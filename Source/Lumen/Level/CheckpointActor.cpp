#include "CheckpointActor.h"
#include "Components/BoxComponent.h"
#include "LumenCharacter.h"
#include "LumenPlayerState.h"
#include "LumenCollisionChannels.h" // ECC_ProjectileTrace
#include "LumenGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

ACheckpointActor::ACheckpointActor()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	RootComponent = TriggerVolume;
	TriggerVolume->SetCollisionProfileName(TEXT("Trigger"));
	// Trigger 프리셋은 커스텀 채널(ProjectileTrace)의 Default Response(Block)를 그대로 물려받아
	// 발사체를 막아버리므로 명시적으로 Ignore
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

	// 더 진행된 체크포인트일 때만 갱신 (역행 방지)
	if (CheckpointOrder <= PS->LastCheckpointOrder)
	{
		return;
	}

	PS->LastCheckpointTransform = GetActorTransform();
	PS->LastCheckpointID = CheckpointID;
	PS->LastCheckpointOrder = CheckpointOrder;

	if (CheckpointSound)
	{
		UGameplayStatics::PlaySound2D(this, CheckpointSound);
	}

	// 세이브 파일에 즉시 기록
	if (ULumenGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance<ULumenGameInstance>() : nullptr)
	{
		GI->SaveCurrentProgress(PS);
	}
}