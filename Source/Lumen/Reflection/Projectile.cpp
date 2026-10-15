// AProjectile 구현 — 자세한 설명은 Projectile.h
#include "Projectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "DrawDebugHelpers.h"
#include "WeaponDataAsset.h"
#include "Reflective.h"
#include "Kismet/GameplayStatics.h"
#include "ShieldActor.h"
#include "LumenCollisionChannels.h"
#include "PuzzleSensorActor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundBase.h"
#include "HealthComponent.h"
#include "MirrorActor.h"

TAutoConsoleVariable<bool> CVarShowReflectionDebug(
	TEXT("ShowReflectionDebug"),
	false,
	TEXT("반사 입사선/반사선을 디버그로 표시"),
	ECVF_Cheat
);

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComp"));
	CollisionComp->InitSphereRadius(8.0f);
	CollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 물리 충돌은 안 씀 - 판정은 라인 트레이스로만
	RootComponent = CollisionComp;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(RootComponent);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComp->SetCastShadow(false); // 빛 자체라 그림자를 드리우면 어색함
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		const bool bIsPlayerOwned = OwnerPawn->IsPlayerControlled();
		CollisionComp->SetCollisionObjectType(
			bIsPlayerOwned ? ECC_PlayerProjectile : ECC_EnemyProjectile);
	}
	else
	{
		// ALightEmitterActor처럼 Pawn이 아닌 소유자
		CollisionComp->SetCollisionObjectType(ECC_PlayerProjectile); // 임시 기본값 (미확정)
	}

	// 스폰 직후 소유자 무시는 Tick에서 처리 (반사 전에는 쏜 사람을 항상 무시)

	// 블루프린트에서 메시 스케일을 바꿨을 수 있으므로 생성자가 아닌 BeginPlay에서 캡처
	InitialMeshScale = MeshComp->GetRelativeScale3D();
}

void AProjectile::InitVelocity(const FVector& Direction)
{
	Velocity = Direction.GetSafeNormal() * Speed;
	FaceVelocity();
}

void AProjectile::FaceVelocity()
{
	if (!Velocity.IsNearlyZero())
	{
		SetActorRotation(Velocity.Rotation());
	}
}

void AProjectile::ApplyWeaponColor(const FLinearColor& Color)
{
	if (!MeshMID)
	{
		// 0번 슬롯 머티리얼(M_Projectile)을 기반으로 이 발사체 전용 인스턴스 생성
		MeshMID = MeshComp->CreateDynamicMaterialInstance(0);
	}
	if (MeshMID)
	{
		MeshMID->SetVectorParameterValue(TEXT("Color"), Color);
	}
}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// ── 페이드아웃 중: 이동/판정 없이 작아지기만 하다가 파괴 ──
	if (bFadingOut)
	{
		FadeElapsed += DeltaTime;
		const float Alpha = FMath::Clamp(FadeElapsed / FadeOutDuration, 0.f, 1.f);
		MeshComp->SetRelativeScale3D(InitialMeshScale * (1.f - Alpha));
		if (Alpha >= 1.f)
		{
			Destroy();
		}
		return;
	}

	if (bStopped || Velocity.IsNearlyZero())
	{
		return;
	}

	if (IgnoreLastActorTimer > 0.f)
	{
		IgnoreLastActorTimer -= DeltaTime;
	}

	// ── 사거리 클램프: 이번 틱 이동량이 남은 사거리를 넘으면 정확히 경계까지만 이동 ──
	// (조준선 끝 = 실제 소멸 지점이 되도록. 넘어서 날아가다 사라지면 사거리가 길어 보임)
	const FVector Direction = Velocity.GetSafeNormal();
	float StepDistance = Velocity.Size() * DeltaTime;
	bool bReachesRangeEnd = false;

	if (MaxRange > 0.f)
	{
		const float RemainingRange = MaxRange - DistanceTraveled;
		if (RemainingRange <= KINDA_SMALL_NUMBER)
		{
			BeginFadeOut();
			return;
		}
		if (RemainingRange <= StepDistance)
		{
			StepDistance = RemainingRange;
			bReachesRangeEnd = true;
		}
	}

	const FVector StartLocation = GetActorLocation();
	const FVector EndLocation = StartLocation + Direction * StepDistance;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(ProjectileTrace), false, this);
	Params.AddIgnoredActor(this);

	// 한 번도 반사되기 전에는 쏜 사람 자신(몸·포탑 메시)을 계속 무시 — 펠릿이 쏜 포탑 몸통에 맞지 않도록.
	// 초록 레이저의 "첫 세그먼트만 자기 몸 무시"와 같은 규칙.
	// 반사된 뒤에는 다시 맞을 수 있어야 함 (방패로 되돌려 보낸 탄은 쏜 적에게 그대로 명중)
	if (CurrentReflections == 0 && GetOwner())
	{
		Params.AddIgnoredActor(GetOwner());
	}

	// 방금 반사한 대상은 짧은 시간 동안 무시 - 모서리 등에서 오프셋만으로
	// 완전히 못 빠져나왔을 때 같은 자리에서 매 틱 재충돌하는 것을 방지
	if (IgnoreLastActorTimer > 0.f && LastReflectedActor.IsValid())
	{
		Params.AddIgnoredActor(LastReflectedActor.Get());
	}

	FCollisionShape SphereShape = FCollisionShape::MakeSphere(CollisionComp->GetScaledSphereRadius());
	FHitResult Hit;
	const bool bHit = GetWorld()->SweepSingleByChannel(
		Hit, StartLocation, EndLocation, FQuat::Identity, ECC_ProjectileTrace, SphereShape, Params);

	if (bHit)
	{
		DistanceTraveled += FVector::Dist(StartLocation, Hit.ImpactPoint); // 실제 맞은 지점까지만 누적
		HandleHit(Hit);
	}
	else
	{
		DistanceTraveled += StepDistance;
		SetActorLocation(EndLocation);

		if (bReachesRangeEnd)
		{
			BeginFadeOut();
		}
	}
}

void AProjectile::InitFromWeaponData(UWeaponDataAsset* WeaponData, const FVector& Direction)
{
	if (!WeaponData) return;

	Damage = WeaponData->Damage;
	MaxReflections = WeaponData->MaxReflections;
	MaxRange = WeaponData->FalloffEndRange; // 에디터 표시명 "Max Range" — 조준선 길이와 같은 값
	BaseMaxRange = MaxRange;

	Speed = WeaponData->ProjectileSpeed; // InitVelocity가 Speed를 쓰므로 먼저 갱신
	InitVelocity(Direction);
	ProjectileColor = DeriveWeaponColor(WeaponData);
	ApplyWeaponColor(WeaponData->WeaponColor_TEMP); // 조준선과 같은 색 소스
	ReflectSound = WeaponData->ReflectSound;
}

void AProjectile::BeginFadeOut()
{
	if (bFadingOut)
	{
		return;
	}

	bFadingOut = true;
	bStopped = true;
	Velocity = FVector::ZeroVector;
	FadeElapsed = 0.f;

	if (RangeEndFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), RangeEndFX, GetActorLocation(), FRotator::ZeroRotator);
	}
}

void AProjectile::HandleHit(const FHitResult& Hit)
{
	AActor* OtherActor = Hit.GetActor();
	const bool bIsReflectiveChannel =
		Hit.Component.IsValid() && Hit.Component->GetCollisionObjectType() == ECC_Reflective;

	if (!OtherActor || !bIsReflectiveChannel)
	{
		if (OtherActor)
		{
			// 센서는 데미지 대신 색 알림만 받음. 그 외 대상은 같은 VolleyId의 펠릿이 대상당 1회만 데미지
			if (APuzzleSensorActor* Sensor = Cast<APuzzleSensorActor>(OtherActor))
			{
				Sensor->NotifyHitByLight(ProjectileColor);
			}
			else
			{
				UHealthComponent* HC = OtherActor->FindComponentByClass<UHealthComponent>();
				if (!HC || HC->TryConsumeVolley(VolleyId))
				{
					UGameplayStatics::ApplyDamage(OtherActor, Damage, DamageInstigatorController, this, UDamageType::StaticClass());
				}
			}
		}

		if (ReflectionSparkFX)  // 임팩트 전용 이펙트가 없어 반사 이펙트를 재사용
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ReflectionSparkFX, Hit.ImpactPoint, FRotator::ZeroRotator);
		}

		Destroy();
		return;
	}

	if (CurrentReflections >= MaxReflections)
	{
		// 반사 한도 도달 — 즉시 파괴 대신 맞은 자리에서 페이드 소멸
		SetActorLocation(Hit.Location);
		BeginFadeOut();
		return;
	}

	IReflective* ReflectiveActor = Cast<IReflective>(OtherActor);
	checkf(ReflectiveActor, TEXT("Reflective 채널인데 IReflective 미구현: %s"), *OtherActor->GetName());

	AShieldActor* Shield = Cast<AShieldActor>(OtherActor);

	// 반사는 항상 수평면에서만 — 노멀 Z=0 고정
	FVector SurfaceNormal = ReflectiveActor->GetReflectionNormal(Hit);
	SurfaceNormal.Z = 0.f;
	SurfaceNormal = SurfaceNormal.GetSafeNormal();

	FVector IncidentDir = Velocity.GetSafeNormal();
	if (FVector::DotProduct(IncidentDir, SurfaceNormal) > 0.f)
	{
		// 뒷면 - 반사하지 않고 맞은 자리에서 멈춘 뒤 페이드 소멸
		SetActorLocation(Hit.Location);
		BeginFadeOut();
		return;
	}

	// 방패 반사일 때만 Instigator를 방패 소유자로 재설정 (DamageCauser는 발사체 그대로).
	// 뒷면 판정 뒤에 둬서 반사가 확정된 경우에만 적용
	if (Shield)
	{
		Shield->PlayBlockSound(Hit.ImpactPoint);
		if (APawn* OwningPawn = Cast<APawn>(Shield->GetOwner()))
		{
			DamageInstigatorController = OwningPawn->GetController();
		}
	}

	FVector ReflectedDir = IncidentDir.MirrorByVector(SurfaceNormal);

	// 방패에 맞았으면 근처 적 방향으로 살짝 보정 (에임 어시스트, 거울은 대상에서 자동 제외됨)
	if (Shield)
	{
		ReflectedDir = Shield->ApplyAimAssist(ReflectedDir, Hit.ImpactPoint);
	}

	Velocity = ReflectedDir * Velocity.Size();
	FaceVelocity(); // 반사 후 새 진행 방향으로 메시 회전

	// 재충돌 방지 오프셋
	SetActorLocation(Hit.ImpactPoint + SurfaceNormal * 15.0f);
	CurrentReflections++;

	// 반사 후 이동 거리 처리
	// - 방패: 누적 유지 → 빨리 반사할수록 멀리 돌려보낼 수 있는 긴장감
	// - 맵 거울(방패 외 모든 반사체): 0으로 리셋 + 사거리를 "발사 사거리 × 거울 배율"로 다시 채움
	//   → 퍼즐 설계 시 거리 제약에서 자유롭고, 거울을 거칠수록 멀리 보낼 수 있음
	if (!Shield)
	{
		DistanceTraveled = 0.f;

		if (BaseMaxRange > 0.f) // 0 = 무제한이면 그대로
		{
			const AMirrorActor* Mirror = Cast<AMirrorActor>(OtherActor);
			MaxRange = BaseMaxRange * (Mirror ? Mirror->GetReflectedRangeMultiplier() : 1.f);
		}
	}

	// 모서리 등에서 오프셋으로 완전히 못 빠져나왔을 경우를 대비해
	// 이 액터는 잠시 무시 (0.05초 — 속도 2000이면 약 100cm 이동하는 시간)
	LastReflectedActor = OtherActor;
	IgnoreLastActorTimer = 0.05f;

	if (ReflectionSparkFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ReflectionSparkFX, Hit.ImpactPoint, FRotator::ZeroRotator);
	}

	// 반사음 — 반사가 확정된 경우에만 (뒷면/반사 한도는 위에서 이미 return)
	// 파랑 펠릿 여러 발이 같은 거울에 동시에 맞으면 겹쳐 울림 → 사운드 에셋 Concurrency로 제한
	if (ReflectSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ReflectSound, Hit.ImpactPoint);
	}

	if (CVarShowReflectionDebug.GetValueOnGameThread())
	{
		DrawDebugLine(GetWorld(), Hit.TraceStart, Hit.ImpactPoint, FColor::Red, false, 3.f, 0, 2.f);
		DrawDebugLine(GetWorld(), Hit.ImpactPoint, Hit.ImpactPoint + ReflectedDir * 300.f, FColor::Yellow, false, 3.f, 0, 2.f);
	}
}