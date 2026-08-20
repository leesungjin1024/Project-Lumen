// MirrorActor.cpp
#include "MirrorActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Lumen/LumenCharacter.h"
#include "LumenCollisionChannels.h" 

AMirrorActor::AMirrorActor()
{
	CollisionComp = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionComp"));
	// 5-2-1: 그레이징 각도 대응 — 시각 메시보다 콜리전을 살짝 두껍게(5~10cm)
	CollisionComp -> InitBoxExtent(FVector(10.f, 100.f, 150.f));

	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComp->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	CollisionComp->SetCollisionObjectType(ECC_Reflective);
	CollisionComp->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore); // AI LOS 트레이스는 그대로 통과
	CollisionComp->SetCollisionResponseToChannel(ECC_ProjectileTrace, ECR_Block);

	RootComponent = CollisionComp;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp -> SetupAttachment(RootComponent);
	MeshComp -> SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 상호작용 범위는 반사 판정 박스보다 넉넉하게 - 캐릭터가 가까이 오면 감지
	InteractionZone = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionZone"));
	InteractionZone->SetupAttachment(RootComponent);
	InteractionZone->SetSphereRadius(250.f);
	InteractionZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionZone->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	InteractionZone->SetCollisionResponseToChannel(ECC_ProjectileTrace, ECR_Ignore); // 추가 — 발사체는 이 구체를 완전히 무시
}

void AMirrorActor::BeginPlay()
{
	Super::BeginPlay();

	if (bIsRotatable)
	{
		InteractionZone->OnComponentBeginOverlap.AddDynamic(this, &AMirrorActor::OnInteractionZoneBeginOverlap);
		InteractionZone->OnComponentEndOverlap.AddDynamic(this, &AMirrorActor::OnInteractionZoneEndOverlap);
	}
}

FVector AMirrorActor::GetReflectionNormal(const FHitResult& Hit) const
{
	FVector Normal = GetActorForwardVector();
	Normal.Z = 0.f;              // 수평 평면으로 강제 고정 (4-1/4-2 설계 원칙)
	return Normal.GetSafeNormal();
}

void AMirrorActor::RotateStep()
{
	if (!bIsRotatable)
	{
		return;
	}

	AddActorWorldRotation(FRotator(0.f, RotationStepDegrees, 0.f));
	// Pitch/Roll은 절대 안 건드림 - 반사가 수평으로만 유지되게 (4-1/4-2 원칙)
}

void AMirrorActor::OnInteractionZoneBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (ALumenCharacter* Character = Cast<ALumenCharacter>(OtherActor))
	{
		Character->SetInteractableMirror(this);
	}
}

void AMirrorActor::OnInteractionZoneEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (ALumenCharacter* Character = Cast<ALumenCharacter>(OtherActor))
	{
		Character->ClearInteractableMirror(this);
	}
}