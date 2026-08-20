#include "ShieldActor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "LumenCollisionChannels.h" 

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