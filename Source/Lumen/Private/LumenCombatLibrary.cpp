// LumenCombatLibrary.cpp
//
// 기존 ALumenCharacter::FireLineTrace의 while 루프(3주차 §2-3)를 그대로 가져와
// 정적 함수로 옮기고, 관통(bPierces) 분기와 방패 반사 시 Instigator/Causer 재설정
// (3주차 §2-4-1, §5장의 "AI 리팩터링과 함께 처리" 계획)을 추가함.
//
// ※ 프로젝트의 실제 클래스명(IReflective, AShieldActor, UHealthComponent,
//   UWeaponDataAsset, CVarShowReflectionDebug)에 맞춰 include 경로만 조정하면 됨.
 
#include "LumenCombatLibrary.h"
#include "WeaponDataAsset.h"
#include "Lumen/Reflection/Reflective.h"
#include "ShieldActor.h"
#include "HealthComponent.h"
#include "Lumen/Reflection/Projectile.h"// CVarShowReflectionDebug extern 선언 재사용 (2주차 트러블슈팅 표 참고)
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Pawn.h"
#include "LumenCollisionChannels.h" 


void ULumenCombatLibrary::FireReflectiveLineTrace(
	const UObject* WorldContextObject,
	AActor* OwnerActor,
	FVector StartLocation,
	FVector Direction,
	UWeaponDataAsset* WeaponData,
	AController* InitialInstigator,
	AActor* InitialCauser,
	UNiagaraSystem* ReflectionSparkVFX)
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

	// 3주차 §2-4-1과 동일 원칙을 "발사체 멤버 변수" 대신 "루프 로컬 변수"로 구현
	// (라인트레이스는 프레임 생애가 없는 즉발이라 멤버로 들고 다닐 대상이 없음 — 3주차 5장에 이미 이렇게 하기로 정해둔 방식)
	AController* CurrentInstigator = InitialInstigator;
	AActor* CurrentCauser = InitialCauser;

	FVector CurrentStart = StartLocation;
	FVector CurrentDirection = Direction;

	float RemainingRange = WeaponData->TraceRange;
	int32 SegmentCount = 0; // 반사 + 관통 통과를 합산한 세그먼트 수 (MaxReflections로 상한)

	AActor* ActorToIgnoreThisSegment = nullptr;

	while (SegmentCount <= WeaponData->MaxReflections && RemainingRange > 0.f)
	{
		const FVector TraceEnd = CurrentStart + CurrentDirection * RemainingRange;

		FCollisionQueryParams Params;
		Params.bTraceComplex = true;
		if (SegmentCount == 0)
		{
			Params.AddIgnoredActor(OwnerActor); // 첫 세그먼트(총구 직후)에서만 자기 몸 무시
		}
		if (ActorToIgnoreThisSegment)
		{
			Params.AddIgnoredActor(ActorToIgnoreThisSegment);
		}


		FHitResult Hit;
		const bool bHit = World->LineTraceSingleByChannel(Hit, CurrentStart, TraceEnd, ECC_ProjectileTrace, Params);

		if (CVarShowReflectionDebug.GetValueOnGameThread())
		{
			DrawDebugLine(World, CurrentStart, bHit ? Hit.ImpactPoint : TraceEnd, FColor::Green, false, 1.0f, 0, 2.0f);
		}

		if (!bHit)
		{
			// 사거리 끝까지 아무것도 안 맞음 - 종료
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

			// 뒷면 판별 (5-2-1): 입사 방향·법선의 내적이 양수면 뒷면에서 맞은 것 - 반사 없이 정지
			if (FVector::DotProduct(CurrentDirection, Normal) > 0.f)
			{
				break;
			}
			
			// 방패에 맞았으면 이 시점부터 Instigator/Causer를 방패 소유자로 재설정
			// (AMirrorActor는 소유권 개념이 없으므로 그대로 통과 — 3주차 §2-4-1과 동일 원칙)
			if (AShieldActor* Shield = Cast<AShieldActor>(HitActor))
			{
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

			// 반사 공식은 각도 구분 없이 동일 적용 (5-2-1, UE의 FVector::MirrorByVector)
			CurrentDirection = CurrentDirection.MirrorByVector(Normal).GetSafeNormal();

		    // 반사각 보정
			if (AShieldActor* ReflectingShield = Cast<AShieldActor>(HitActor))
			{
				CurrentDirection = ReflectingShield->ApplyAimAssist(CurrentDirection, Hit.ImpactPoint);
			}

			CurrentStart = Hit.ImpactPoint + Normal * 0.5f; // 재충돌 방지 오프셋
			ActorToIgnoreThisSegment = HitActor;

			++SegmentCount;
			continue;
		}

		// 반사 대상이 아님 - 데미지를 받을 수 있는 대상인지 확인 (UHealthComponent 보유 여부)
		UHealthComponent* HealthComp = HitActor ? HitActor->FindComponentByClass<UHealthComponent>() : nullptr;

		if (HealthComp)
		{
			UGameplayStatics::ApplyDamage(HitActor, WeaponData->Damage, CurrentInstigator, CurrentCauser, UDamageType::StaticClass());

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

		// 벽 등, 반사 대상도 데미지 대상도 아닌 것 - 정지
		break;
	}
}
