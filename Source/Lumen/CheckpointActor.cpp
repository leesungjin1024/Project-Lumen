// CheckpointActor.cpp

#include "CheckpointActor.h"
#include "Components/BoxComponent.h"
#include "LumenCharacter.h"
#include "LumenPlayerState.h"
#include "LumenCollisionChannels.h" // ECC_ProjectileTrace
#include "LumenGameInstance.h"

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

	ALumenPlayerState* PS = Cast<ALumenPlayerState>(Player->GetPlayerState());
	if (!PS)
	{
		return;
	}

	// 5-10: 더 진행된 체크포인트일 때만 갱신 (역행 방지) — 기존에 빠져있던 로직
	if (CheckpointOrder <= PS->LastCheckpointOrder)
	{
		return;
	}

	PS->LastCheckpointTransform = GetActorTransform();
	PS->LastCheckpointID = CheckpointID;
	PS->LastCheckpointOrder = CheckpointOrder; // ← 기존에 아예 빠져있던 대입

	// 세이브 파일에 즉시 기록
	if (ULumenGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance<ULumenGameInstance>() : nullptr)
	{
		GI->SaveCurrentProgress(PS);
	}
}