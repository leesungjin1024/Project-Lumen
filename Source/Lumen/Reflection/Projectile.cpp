#include "Projectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "DrawDebugHelpers.h"
#include "Reflective.h"

static TAutoConsoleVariable<bool> CVarShowReflectionDebug(
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
		Hit, StartLocation, EndLocation, FQuat::Identity, ECC_Visibility, SphereShape, Params);

	if (bHit)
	{
		HandleHit(Hit);
	}
	else
	{
		SetActorLocation(EndLocation);
	}
}

void AProjectile::HandleHit(const FHitResult& Hit)
{
	AActor* OtherActor = Hit.GetActor();

	if (!OtherActor || !OtherActor->Implements<UReflective>())
	{
		// 반사 대상 아님(벽, 캐릭터 등) → 그 자리에서 정지
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