#include "Projectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "DrawDebugHelpers.h"
#include "WeaponDataAsset.h"
#include "Reflective.h"
#include "Kismet/GameplayStatics.h" // 신규 추가
#include "ShieldActor.h"
#include "LumenCollisionChannels.h"  

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
		// ALightEmitterActor처럼 Pawn이 아닌 소유자 — 4주차 문서에 남아있던 미결정 사항과 동일 이슈
		CollisionComp->SetCollisionObjectType(ECC_PlayerProjectile); // 임시 기본값, 결정 필요
	}

	// 기존 로직 유지 — 채널 분리가 이걸 대체하지 않음
	IgnoreLastActorTimer = 0.05f;
	LastReflectedActor = GetOwner();
}

void AProjectile::InitVelocity(const FVector& Direction)
{
	Velocity = Direction.GetSafeNormal() * Speed;
}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bStopped || Velocity.IsNearlyZero())
	{
		return;
	}
	//모서리 
	if (IgnoreLastActorTimer > 0.f)
	{
		IgnoreLastActorTimer -= DeltaTime;
	}

	const FVector StartLocation = GetActorLocation();
	const FVector EndLocation = StartLocation + Velocity * DeltaTime;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(ProjectileTrace), false, this);
	Params.AddIgnoredActor(this);

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
		DistanceTraveled += FVector::Dist(StartLocation, EndLocation);
		SetActorLocation(EndLocation);
	}

}

void AProjectile::InitFromWeaponData(UWeaponDataAsset* WeaponData, const FVector& Direction)
{
	if (!WeaponData) return;

	Damage = WeaponData->Damage;
	MaxReflections = WeaponData->MaxReflections;
	FalloffStartRange = WeaponData->FalloffStartRange;
	FalloffEndRange = WeaponData->FalloffEndRange;
	MinDamageMultiplier = WeaponData->MinDamageMultiplier;

	Speed = WeaponData->ProjectileSpeed; // ← 먼저 Speed 멤버 자체를 갱신
	InitVelocity(Direction);            // ← 순수 방향만 넘기고, 내부에서 갱신된 Speed를 사용하게 함
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
			const float FinalDamage = Damage * CalculateFalloffMultiplier();
			UGameplayStatics::ApplyDamage(OtherActor, FinalDamage, DamageInstigatorController, this, nullptr);
		}

		SetActorLocation(Hit.ImpactPoint);
		Velocity = FVector::ZeroVector;
		bStopped = true;
		return;
	}

	if (CurrentReflections >= MaxReflections)
	{
		Destroy();
		return;
	}

	IReflective* ReflectiveActor = Cast<IReflective>(OtherActor);
	checkf(ReflectiveActor, TEXT("Reflective 채널인데 IReflective 미구현: %s"), *OtherActor->GetName());
	
	if (AShieldActor* Shield = Cast<AShieldActor>(OtherActor))   // HitActor 아니라 OtherActor
	{
		if (APawn* OwningPawn = Cast<APawn>(Shield->GetOwner()))
		{
			DamageInstigatorController = OwningPawn->GetController();
		}
	}

	FVector SurfaceNormal = ReflectiveActor->GetReflectionNormal(Hit);
	SurfaceNormal.Z = 0.f;
	SurfaceNormal = SurfaceNormal.GetSafeNormal();

	FVector IncidentDir = Velocity.GetSafeNormal();
	if (FVector::DotProduct(IncidentDir, SurfaceNormal) > 0.f)
	{
		// 뒷면 - 반사하지 않고 그대로 계속 진행
		//SetActorLocation(Hit.ImpactPoint + IncidentDir * 15.0f);
		//return;

		// 뒷면 - 반사는 안 하지만 그냥 통과시키지 않고 멈춤
		SetActorLocation(Hit.ImpactPoint + IncidentDir * 15.0f);   // 뒷면 통과 후
		Velocity = FVector::ZeroVector;
		bStopped = true;
		return;

	}

	FVector ReflectedDir = IncidentDir.MirrorByVector(SurfaceNormal);

	// 방패에 맞았으면 근처 적 방향으로 살짝 보정 (에임 어시스트, 거울은 대상에서 자동 제외됨)
	if (AShieldActor* Shield = Cast<AShieldActor>(OtherActor))
	{
		ReflectedDir = Shield->ApplyAimAssist(ReflectedDir, Hit.ImpactPoint);
	}

	Velocity = ReflectedDir * Velocity.Size();

	// 재충돌 방지 오프셋
	SetActorLocation(Hit.ImpactPoint + SurfaceNormal * 15.0f);
	CurrentReflections++;

	// 모서리 등에서 오프셋으로 완전히 못 빠져나왔을 경우를 대비해
	// 이 액터는 잠시 무시 (약 0.05초, 초당 2000속력이면 약 100cm 이동하는 시간)
	LastReflectedActor = OtherActor;
	IgnoreLastActorTimer = 0.05f;

	if (ReflectionSparkFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ReflectionSparkFX, Hit.ImpactPoint, FRotator::ZeroRotator);
	}

	if (CVarShowReflectionDebug.GetValueOnGameThread())
	{
		DrawDebugLine(GetWorld(), Hit.TraceStart, Hit.ImpactPoint, FColor::Red, false, 3.f, 0, 2.f);
		DrawDebugLine(GetWorld(), Hit.ImpactPoint, Hit.ImpactPoint + ReflectedDir * 300.f, FColor::Yellow, false, 3.f, 0, 2.f);
	}

	UE_LOG(LogTemp, Warning, TEXT("Reflected! Incident=%s Normal=%s NewDir=%s"),
		*IncidentDir.ToString(), *SurfaceNormal.ToString(), *ReflectedDir.ToString());
}

float AProjectile::CalculateFalloffMultiplier() const
{
	// FalloffEndRange <= FalloffStartRange면 감쇄 미설정 상태(빨강/초록 기본값 0/0) → 항상 100%
	if (FalloffEndRange <= FalloffStartRange)
	{
		return 1.f;
	}

	if (DistanceTraveled <= FalloffStartRange)
	{
		return 1.f;
	}

	if (DistanceTraveled >= FalloffEndRange)
	{
		return MinDamageMultiplier;
	}

	const float Alpha = (DistanceTraveled - FalloffStartRange) / (FalloffEndRange - FalloffStartRange);
	return FMath::Lerp(1.f, MinDamageMultiplier, Alpha);
}