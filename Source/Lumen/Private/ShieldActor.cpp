#include "ShieldActor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "LumenCollisionChannels.h" 
#include "HealthComponent.h" 
#include "Engine/OverlapResult.h"

AShieldActor::AShieldActor()
{
	PrimaryActorTick.bCanEverTick = false;
	ShieldCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("ShieldCollision"));
	ShieldCollision->SetBoxExtent(ShieldBoxExtent);
	RootComponent = ShieldCollision;
	ShieldCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(RootComponent);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComp->SetVisibility(false);

	// 채널 분리 설정 — 변수명 통일, Block 줄 삭제
	ShieldCollision->SetCollisionObjectType(ECC_Reflective);
	ShieldCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore); // 4주차 LOS 버그 해결
	ShieldCollision->SetCollisionResponseToChannel(ECC_ProjectileTrace, ECR_Block);
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


FVector AShieldActor::ApplyAimAssist(const FVector& ReflectedDirection, const FVector& Origin) const
{
	//UE_LOG(LogTemp, Warning, TEXT("ApplyAimAssist called")); //

	const FVector NormalizedReflected = ReflectedDirection.GetSafeNormal2D();

	AActor* BestTarget = nullptr;
	float BestAngleDeg = AimAssistConeHalfAngleDegrees; // 이 각도보다 좁은 후보만 채택

	TArray<FOverlapResult> Overlaps;
	FCollisionShape SearchSphere = FCollisionShape::MakeSphere(AimAssistSearchRadius);
	FCollisionObjectQueryParams ObjParams(ECC_Pawn);

	GetWorld()->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity, ObjParams, SearchSphere);
	//UE_LOG(LogTemp, Warning, TEXT("Overlaps found: %d"), Overlaps.Num()); //

	for (const FOverlapResult& O : Overlaps)
	{
		AActor* Candidate = O.GetActor();
		if (!Candidate || Candidate == GetOwner())
		{
			continue; // 방패 소유자(플레이어) 자신은 타겟에서 제외
		}

		// "적인가"는 LumenCombatLibrary와 동일 기준(UHealthComponent 보유 여부)으로 판별
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

//
	//UE_LOG(LogTemp, Warning, TEXT("BestTarget: %s, Angle: %.1f"), BestTarget ? *BestTarget->GetName() : TEXT("None"), BestAngleDeg);
	if (!BestTarget)
	{
		return NormalizedReflected; // 원뿔 안에 적이 없으면 원래 계산값 그대로
	}

	const FVector ToBest = (BestTarget->GetActorLocation() - Origin).GetSafeNormal2D();
	return FMath::Lerp(NormalizedReflected, ToBest, AimAssistBlendAlpha).GetSafeNormal2D();
}