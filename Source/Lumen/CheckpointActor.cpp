// CheckpointActor.cpp

#include "CheckpointActor.h"
#include "Components/BoxComponent.h"
#include "LumenCharacter.h"
#include "LumenPlayerState.h"
#include "LumenCollisionChannels.h" // ECC_ProjectileTrace

ACheckpointActor::ACheckpointActor()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	RootComponent = TriggerVolume;
	TriggerVolume->SetCollisionProfileName(TEXT("Trigger"));
	// Trigger 프리셋은 ProjectileTrace 채널의 Default Response(Block)를 그대로 물려받아
	// 발사체를 막아버림 (CombatRoomVolume/PuzzleRoomVolume과 동일한 문제) — 명시적으로 무시
	TriggerVolume->SetCollisionResponseToChannel(ECC_ProjectileTrace, ECR_Ignore);
	TriggerVolume->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
}

void ACheckpointActor::BeginPlay()
{
	Super::BeginPlay();
	TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &ACheckpointActor::OnOverlapBegin);
}

void ACheckpointActor::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	ALumenCharacter* Player = Cast<ALumenCharacter>(OtherActor);
	if (!Player)
	{
		return;
	}

	// APawn::GetPlayerState()는 템플릿이 아니라 APlayerState*를 반환하므로 Cast 필요
	if (ALumenPlayerState* PS = Cast<ALumenPlayerState>(Player->GetPlayerState()))
	{
		PS->LastCheckpointTransform = GetActorTransform();
		PS->LastCheckpointID = CheckpointID;
	}
}