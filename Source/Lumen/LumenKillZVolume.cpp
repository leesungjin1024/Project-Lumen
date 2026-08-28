// LumenKillZVolume.cpp

#include "LumenKillZVolume.h"
#include "Components/BoxComponent.h"
#include "LumenCharacter.h"
#include "LumenCollisionChannels.h" // ECC_ProjectileTrace

ALumenKillZVolume::ALumenKillZVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	RootComponent = TriggerVolume;
	TriggerVolume->SetCollisionProfileName(TEXT("Trigger"));
	// Trigger 프리셋은 ProjectileTrace 채널의 Default Response(Block)를 그대로 물려받아
	// 발사체를 막아버림 (CombatRoomVolume/PuzzleRoomVolume/CheckpointActor와 동일한 문제) — 명시적으로 무시
	TriggerVolume->SetCollisionResponseToChannel(ECC_ProjectileTrace, ECR_Ignore);
	TriggerVolume->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
}

void ALumenKillZVolume::BeginPlay()
{
	Super::BeginPlay();
	TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &ALumenKillZVolume::OnOverlapBegin);
}

void ALumenKillZVolume::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (ALumenCharacter* Player = Cast<ALumenCharacter>(OtherActor))
	{
		// 데미지 파이프라인(ApplyDamage 등)을 타지 않고 곧장 리스폰 처리
		// — 낙사는 반사/전투 데미지 판정과 무관하므로 별도 경로가 더 단순함
		Player->HandleFellOutOfWorld();
	}
}