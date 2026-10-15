#include "CombatRoomVolume.h"
#include "Components/BoxComponent.h"
#include "LumenEnemyCharacter.h"
#include "LumenCharacter.h"
#include "HealthComponent.h"
#include "HeatComponent.h"
#include "LumenDoorActor.h"
#include "LumenCollisionChannels.h" // ECC_ProjectileTrace
#include "LumenMusicSubsystem.h"
#include "Sound/SoundBase.h"

ACombatRoomVolume::ACombatRoomVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	EntryTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("EntryTrigger"));
	RootComponent = EntryTrigger;
	EntryTrigger->SetCollisionProfileName(TEXT("Trigger"));
	// Trigger 프리셋은 커스텀 채널(ProjectileTrace)의 Default Response(Block)를 그대로 물려받음.
	// 방 전체를 덮는 큰 볼륨이라 이대로 두면 안에서 발사체가 전부 막히므로 명시적으로 Ignore
	EntryTrigger->SetCollisionResponseToChannel(ECC_ProjectileTrace, ECR_Ignore);
	EntryTrigger->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore); // AI LOS도 방해 안 하게
}

void ACombatRoomVolume::BeginPlay()
{
	Super::BeginPlay();
	EntryTrigger->OnComponentBeginOverlap.AddDynamic(this, &ACombatRoomVolume::OnEntryOverlap);
	EntryTrigger->OnComponentEndOverlap.AddDynamic(this, &ACombatRoomVolume::OnEntryEndOverlap);
}

void ACombatRoomVolume::OnEntryOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	UE_LOG(LogTemp, Warning, TEXT("[Room] OnEntryOverlap fired on %s (bRoomStarted=%d)"),
		*GetName(), bRoomStarted);
	if (ALumenCharacter* Player = Cast<ALumenCharacter>(OtherActor))
	{
		// CurrentRoom 갱신은 bRoomStarted와 무관하게 매 진입마다 해야 함 — 전투 중 EndOverlap 후
		// 재진입했을 때 bRoomStarted==true라고 건너뛰면 CurrentRoom이 nullptr로 남아 사망해도 방이 리셋 안 됨
		Player->SetCurrentRoom(this);

		ApplyRoomView(Player, EntryTrigger->Bounds.GetBox());

		if (!bRoomStarted)
		{
			StartRoom();
		}
	}
}

void ACombatRoomVolume::StartRoom()
{
	bRoomStarted = true;
	SpawnEnemies();

	// 전투 곡 시작 (비어 있으면 곡 유지)
	if (CombatMusic)
	{
		if (ULumenMusicSubsystem* Music = GetWorld()->GetSubsystem<ULumenMusicSubsystem>())
		{
			Music->PlayMusic(CombatMusic, 1.f);
		}
	}
	// TODO: 여기서 입구 문도 함께 잠그고 싶다면 별도 EntryDoor 참조를 추가해 처리
}

void ACombatRoomVolume::OnEntryEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (ALumenCharacter* Player = Cast<ALumenCharacter>(OtherActor))
	{
		ClearRoomView(Player);
		// 내가 아직 CurrentRoom일 때만 지움 — 인접한 다른 방에 먼저 진입해 CurrentRoom이
		// 이미 바뀐 경우 잘못 지우지 않도록
		Player->ClearCurrentRoomIfMatches(this);
	}
}

void ACombatRoomVolume::SpawnEnemies()
{
	AliveEnemies.Empty();
	AliveEnemyCount = 0;

	const int32 Count = FMath::Min(EnemyClassesToSpawn.Num(), SpawnPoints.Num());
	for (int32 i = 0; i < Count; ++i)
	{
		if (!EnemyClassesToSpawn[i])
		{
			continue;
		}

		ALumenEnemyCharacter* NewEnemy = GetWorld()->SpawnActor<ALumenEnemyCharacter>(
			EnemyClassesToSpawn[i], SpawnPoints[i]);

		if (!NewEnemy)
		{
			continue;
		}

		AliveEnemies.Add(NewEnemy);
		++AliveEnemyCount;

		// 적끼리 발열 사이클이 겹치지 않도록 초기 발열량을 무작위(0~40%)로 부여
		if (UHeatComponent* Heat = NewEnemy->FindComponentByClass<UHeatComponent>())
		{
			const float InitialHeatFraction = FMath::FRandRange(0.f, 0.4f);
			Heat->SetHeatFraction(InitialHeatFraction);
		}

		if (UHealthComponent* Health = NewEnemy->FindComponentByClass<UHealthComponent>())
		{
			// OnDeath가 파라미터 없는 델리게이트이므로 HandleEnemyDeath()도 void() 시그니처로 바인딩
			Health->OnDeath.AddDynamic(this, &ACombatRoomVolume::HandleEnemyDeath);
		}
	}
}

void ACombatRoomVolume::HandleEnemyDeath()
{
	--AliveEnemyCount;

	if (AliveEnemyCount <= 0)
	{
		OpenExitDoor();
	}
}

void ACombatRoomVolume::OpenExitDoor()
{
	// 전멸 → 평소 곡으로 복귀
	if (CombatMusic)
	{
		if (ULumenMusicSubsystem* Music = GetWorld()->GetSubsystem<ULumenMusicSubsystem>())
		{
			Music->ReturnToDefaultMusic();
		}
	}

	for (ALumenDoorActor* Door : ExitDoors)
	{
		if (Door)
		{
			Door->Open();
		}
	}
}

void ACombatRoomVolume::ResetRoom()
{
	UE_LOG(LogTemp, Warning, TEXT("[Room] ResetRoom called on %s (AliveEnemyCount=%d, AliveEnemies.Num()=%d)"),
		*GetName(), AliveEnemyCount, AliveEnemies.Num());

	// 이미 클리어된 방(적이 다 죽어서 문이 열린 상태)이면 아무것도 안 함
	if (AliveEnemyCount <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Room] ResetRoom early-return (already cleared)"));
		return;
	}

	// 전투 중 사망 → 평소 곡으로 복귀 (재입장하면 StartRoom에서 다시 전투 곡)
	if (CombatMusic)
	{
		if (ULumenMusicSubsystem* Music = GetWorld()->GetSubsystem<ULumenMusicSubsystem>())
		{
			Music->ReturnToDefaultMusic(1.f);
		}
	}

	for (ALumenEnemyCharacter* Enemy : AliveEnemies)
	{
		if (Enemy)
		{
			Enemy->Destroy();
		}
	}
	AliveEnemies.Empty();
	AliveEnemyCount = 0;

	// 재입장 시 다시 스폰되도록 되돌림
	bRoomStarted = false;

	// 죽어서 리셋되는 거니 문도 다시 닫음 (평소엔 클리어 후 열어둔 채로 두지만, 리셋은 예외)
	for (ALumenDoorActor* Door : ExitDoors)
	{
		if (Door)
		{
			Door->Close();
		}
	}
}