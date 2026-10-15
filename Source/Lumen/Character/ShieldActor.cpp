// AShieldActor 구현 — 자세한 설명은 ShieldActor.h
#include "ShieldActor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "LumenCollisionChannels.h"
#include "HealthComponent.h"
#include "Engine/OverlapResult.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

AShieldActor::AShieldActor()
{
	PrimaryActorTick.bCanEverTick = false;
	ShieldCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("ShieldCollision"));
	ShieldCollision->SetBoxExtent(ShieldBoxExtent);
	RootComponent = ShieldCollision;
	ShieldCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 들기 전엔 꺼둠 (SetShieldActive)

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(RootComponent);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComp->SetVisibility(false);

	// 반사체 채널. 적 AI의 LOS(Visibility) 트레이스는 방패를 통과해야 하므로 Ignore,
	// 발사체/레이저 판정은 ProjectileTrace 채널로만 막음
	ShieldCollision->SetCollisionObjectType(ECC_Reflective);
	ShieldCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	ShieldCollision->SetCollisionResponseToChannel(ECC_ProjectileTrace, ECR_Block);
}

void AShieldActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ShieldCollision->SetBoxExtent(ShieldBoxExtent);
}

FVector AShieldActor::GetReflectionNormal(const FHitResult& Hit) const
{
	FVector Normal = GetActorForwardVector();
	Normal.Z = 0.f;
	return Normal.GetSafeNormal();
}

void AShieldActor::UpdateFacingDirection(const FVector2D& AimDirection)
{
	const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(AimDirection.Y, AimDirection.X));
	SetActorRotation(FRotator(0.f, Yaw, 0.f));
}

void AShieldActor::SetShieldActive(bool bActive)
{
	ShieldCollision->SetCollisionEnabled(bActive ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	MeshComp->SetVisibility(bActive); // 안 든 상태면 시각적으로도 숨김
}


void AShieldActor::PlayBlockSound(const FVector& Location) const
{
	if (BlockSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, BlockSound, Location);
	}
}

FVector AShieldActor::ApplyAimAssist(const FVector& ReflectedDirection, const FVector& Origin) const
{
	const FVector NormalizedReflected = ReflectedDirection.GetSafeNormal2D();

	AActor* BestTarget = nullptr;
	float BestAngleDeg = AimAssistConeHalfAngleDegrees; // 이 각도보다 좁은 후보만 채택

	TArray<FOverlapResult> Overlaps;
	FCollisionShape SearchSphere = FCollisionShape::MakeSphere(AimAssistSearchRadius);
	FCollisionObjectQueryParams ObjParams(ECC_Pawn);

	GetWorld()->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity, ObjParams, SearchSphere);

	for (const FOverlapResult& O : Overlaps)
	{
		AActor* Candidate = O.GetActor();
		if (!Candidate || Candidate == GetOwner())
		{
			continue; // 방패 소유자(플레이어) 자신은 타겟에서 제외
		}

		// "적인가"는 ULumenCombatLibrary의 데미지 대상과 같은 기준(UHealthComponent 보유 여부)으로 판별
		if (!Candidate->FindComponentByClass<UHealthComponent>())
		{
			continue;
		}

		const FVector ToTarget = (Candidate->GetActorLocation() - Origin).GetSafeNormal2D();
		const float DotProduct = FMath::Clamp(FVector::DotProduct(NormalizedReflected, ToTarget), -1.f, 1.f);
		const float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(DotProduct));

		if (AngleDeg < BestAngleDeg)
		{
			BestAngleDeg = AngleDeg;
			BestTarget = Candidate;
		}
	}

	if (!BestTarget)
	{
		return NormalizedReflected; // 원뿔 안에 적이 없으면 원래 계산값 그대로
	}

	const FVector ToBest = (BestTarget->GetActorLocation() - Origin).GetSafeNormal2D();
	return FMath::Lerp(NormalizedReflected, ToBest, AimAssistBlendAlpha).GetSafeNormal2D();
}