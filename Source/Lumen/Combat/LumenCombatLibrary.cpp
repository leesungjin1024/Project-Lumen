// ULumenCombatLibrary 구현 — 자세한 설명은 LumenCombatLibrary.h

#include "LumenCombatLibrary.h"
#include "WeaponDataAsset.h"
#include "Reflective.h"
#include "ShieldActor.h"
#include "HealthComponent.h"
#include "Projectile.h"// CVarShowReflectionDebug extern 선언 재사용
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Pawn.h"
#include "LumenCollisionChannels.h" 
#include "PuzzleSensorActor.h"
#include "LumenBeamSegment.h"
#include "MirrorActor.h"
#include "Sound/SoundBase.h"


void ULumenCombatLibrary::FireReflectiveLineTrace(
	const UObject* WorldContextObject,
	AActor* OwnerActor,
	FVector StartLocation,
	FVector Direction,
	UWeaponDataAsset* WeaponData,
	AController* InitialInstigator,
	AActor* InitialCauser,
	UNiagaraSystem* ReflectionSparkVFX,
	UNiagaraSystem* RangeEndVFX,
	float RangeMultiplier)
{
	if (!WeaponData || !OwnerActor)
	{
		return;
	}

	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}

	Direction = Direction.GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		return;
	}

	// 데미지 귀속(Instigator/Causer)을 발사체처럼 멤버가 아닌 루프 로컬 변수로 추적
	// (라인트레이스는 한 프레임에 끝나는 즉발이라 상태를 들고 다닐 액터가 없음)
	AController* CurrentInstigator = InitialInstigator;
	AActor* CurrentCauser = InitialCauser;

	FVector CurrentStart = StartLocation;
	FVector CurrentDirection = Direction;

	const float MaxTraceRange = WeaponData->TraceRange * FMath::Max(RangeMultiplier, 0.01f);
	float RemainingRange = MaxTraceRange;
	int32 SegmentCount = 0; // 반사 + 관통 통과를 합산한 세그먼트 수 (MaxReflections로 상한)

	AActor* ActorToIgnoreThisSegment = nullptr;

	// 이번 발사에서 이미 데미지를 준 대상 — 관통 후 반사로 같은 적을 다시 지나가도 1회만 데미지
	// (발사체 펠릿의 VolleyId와 같은 규칙)
	TSet<AActor*> DamagedActors;

	while (SegmentCount <= WeaponData->MaxReflections && RemainingRange > 0.f)
	{
		const FVector TraceEnd = CurrentStart + CurrentDirection * RemainingRange;

		FCollisionQueryParams Params;
		Params.bTraceComplex = true;
		if (SegmentCount == 0)
		{
			Params.AddIgnoredActor(OwnerActor); // 첫 세그먼트(총구 직후)에서만 자기 몸 무시 — 반사 후엔 자신도 맞아야 함
		}
		// 거울/방패는 Visibility를 무시하므로 반드시 ProjectileTrace 채널로 검사
		if (ActorToIgnoreThisSegment)
		{
			Params.AddIgnoredActor(ActorToIgnoreThisSegment);
		}


		FHitResult Hit;
		const bool bHit = World->LineTraceSingleByChannel(Hit, CurrentStart, TraceEnd, ECC_ProjectileTrace, Params);

		const FVector SegmentEnd = bHit ? Hit.ImpactPoint : TraceEnd;

		if (CVarShowReflectionDebug.GetValueOnGameThread())
		{
			DrawDebugLine(World, CurrentStart, SegmentEnd, FColor::Green, false, 1.0f, 0, 2.0f);
		}

		// 실제로 보이는 레이저 — 세그먼트(반사/관통 사이 구간)마다 하나씩 스폰.
		// 이 위치 한 곳에서 처리하므로 빗나감/반사/관통/벽 정지/뒷면 정지 전부 자동으로 그려짐
		if (WeaponData->BeamSegmentClass)
		{
			FActorSpawnParameters BeamParams;
			BeamParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			if (ALumenBeamSegment* Beam = World->SpawnActor<ALumenBeamSegment>(
				WeaponData->BeamSegmentClass, CurrentStart, FRotator::ZeroRotator, BeamParams))
			{
				Beam->InitSegment(CurrentStart, SegmentEnd, WeaponData->WeaponColor_TEMP);
			}
		}

		if (!bHit)
		{
			// 사거리 끝까지 아무것도 안 맞음 - 끝 지점에 소멸 이펙트 후 종료
			if (RangeEndVFX)
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, RangeEndVFX, TraceEnd);
			}
			break;
		}

		const float TraveledThisSegment = FVector::Dist(CurrentStart, Hit.ImpactPoint);
		RemainingRange -= TraveledThisSegment;

		AActor* HitActor = Hit.GetActor();
		const bool bIsReflective = HitActor && HitActor->Implements<UReflective>();

		if (bIsReflective)
		{
			IReflective* ReflectiveActor = Cast<IReflective>(HitActor);
			const FVector Normal = ReflectiveActor ? ReflectiveActor->GetReflectionNormal(Hit) : Hit.ImpactNormal;

			// 뒷면 판별: 입사 방향·법선의 내적이 양수면 뒷면에서 맞은 것 - 반사 없이 정지
			if (FVector::DotProduct(CurrentDirection, Normal) > 0.f)
			{
				break;
			}

			// 방패에 맞았으면 이 시점부터 Instigator/Causer를 방패 소유자로 재설정
			// (AMirrorActor는 소유권 개념이 없으므로 그대로 유지)
			if (AShieldActor* Shield = Cast<AShieldActor>(HitActor))
			{
				Shield->PlayBlockSound(Hit.ImpactPoint);
				if (AActor* ShieldOwner = Shield->GetOwner())
				{
					if (APawn* OwnerPawn = Cast<APawn>(ShieldOwner))
					{
						CurrentInstigator = OwnerPawn->GetController();
					}
					CurrentCauser = ShieldOwner;
				}
			}

			if (ReflectionSparkVFX)
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, ReflectionSparkVFX, Hit.ImpactPoint);
			}

			if (WeaponData->ReflectSound)
			{
				UGameplayStatics::PlaySoundAtLocation(World, WeaponData->ReflectSound, Hit.ImpactPoint);
			}

			// 반사 공식은 입사각 구분 없이 동일 적용 (FVector::MirrorByVector)
			CurrentDirection = CurrentDirection.MirrorByVector(Normal).GetSafeNormal();

			// 방패 반사면 근처 적 방향으로 살짝 보정 (에임 어시스트)
			if (AShieldActor* ReflectingShield = Cast<AShieldActor>(HitActor))
			{
				CurrentDirection = ReflectingShield->ApplyAimAssist(CurrentDirection, Hit.ImpactPoint);
			}

			CurrentStart = Hit.ImpactPoint + Normal * 0.5f; // 재충돌 방지 오프셋
			ActorToIgnoreThisSegment = HitActor;

			// 반사 후 사거리 처리 (AProjectile과 동일 규칙)
			// 방패: 남은 사거리 그대로 이어감 / 맵 거울: 사거리 리셋
			if (!Cast<AShieldActor>(HitActor))
			{
				// 맵 거울은 거울마다 정한 배율만큼 사거리를 새로 채움 (AProjectile과 동일 규칙)
				const AMirrorActor* Mirror = Cast<AMirrorActor>(HitActor);
				RemainingRange = MaxTraceRange * (Mirror ? Mirror->GetReflectedRangeMultiplier() : 1.f);
			}

			++SegmentCount;
			continue;
		}

		// 반사 대상이 아님 - 데미지를 받을 수 있는 대상인지 확인 (UHealthComponent 보유 여부)
		UHealthComponent* HealthComp = HitActor ? HitActor->FindComponentByClass<UHealthComponent>() : nullptr;

		if (HealthComp)
		{
			if (!DamagedActors.Contains(HitActor))
			{
				DamagedActors.Add(HitActor);
				UGameplayStatics::ApplyDamage(HitActor, WeaponData->Damage, CurrentInstigator, CurrentCauser, UDamageType::StaticClass());
			}

			if (WeaponData->bPierces)
			{
				// 관통: 정지하지 않고 같은 방향으로 계속 진행.
				// 같은 액터를 이 트레이스 안에서 또 맞히지 않도록 이번 세그먼트 무시 대상으로 등록
				CurrentStart = Hit.ImpactPoint + CurrentDirection * 0.5f;
				ActorToIgnoreThisSegment = HitActor;
				++SegmentCount;
				continue;
			}

			// 관통 아님 - 데미지 주고 정지
			break;
		}

		// 반사 대상도 데미지 대상도 아닌 것(벽, 센서 등) - 정지.
		// 센서는 HealthComponent가 없어 이 분기로 들어옴. 초록은 항상 Green이라 색 조회 불필요
		if (APuzzleSensorActor* Sensor = Cast<APuzzleSensorActor>(HitActor))
		{
			Sensor->NotifyHitByLight(ELumenWeaponColor::Green);
		}

		break;
	}
}