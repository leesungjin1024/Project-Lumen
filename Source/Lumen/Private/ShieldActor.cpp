#include "ShieldActor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"

AShieldActor::AShieldActor()
{
	PrimaryActorTick.bCanEverTick = false; // 회전은 캐릭터가 호출해서 갱신 (자체 Tick 불필요)

	ShieldCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("ShieldCollision"));
	ShieldCollision->SetBoxExtent(ShieldBoxExtent);
	RootComponent = ShieldCollision;

	// 처음엔 방패 안 든 상태 - 콜리전 꺼둠 (반사 판정에 안 걸리게)
	ShieldCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 임시 시각 표현 - 평평한 판. 실제 메시(에디터에서 큐브/플레인 할당)는 콜리전과 분리돼 있어
	// 나중에 메시만 교체해도 콜리전 크기·두께에 영향 없음 (5-2-1 원칙과 일치)
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(RootComponent);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 시각 전용, 판정은 ShieldCollision만
	MeshComp->SetVisibility(false); // 추가 - 시작 시 안 보이게
	ShieldCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); // 추가 필요
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