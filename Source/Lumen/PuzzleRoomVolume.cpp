// PuzzleRoomVolume.cpp

#include "PuzzleRoomVolume.h"
#include "Components/BoxComponent.h"
#include "LumenCharacter.h"
#include "LumenDoorActor.h"
#include "PuzzleSensorActor.h"
#include "Kismet/GameplayStatics.h"
#include "LumenCollisionChannels.h" // ECC_ProjectileTrace

APuzzleRoomVolume::APuzzleRoomVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	EntryTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("EntryTrigger"));
	RootComponent = EntryTrigger;
	EntryTrigger->SetCollisionProfileName(TEXT("Trigger"));
	// CombatRoomVolume과 동일한 이유로 명시적 무시 처리 필요
	EntryTrigger->SetCollisionResponseToChannel(ECC_ProjectileTrace, ECR_Ignore);
	EntryTrigger->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
}

void APuzzleRoomVolume::BeginPlay()
{
	Super::BeginPlay();
	EntryTrigger->OnComponentBeginOverlap.AddDynamic(this, &APuzzleRoomVolume::OnEntryOverlap);
	EntryTrigger->OnComponentEndOverlap.AddDynamic(this, &APuzzleRoomVolume::OnEntryEndOverlap);
}

void APuzzleRoomVolume::OnEntryOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (ALumenCharacter* Player = Cast<ALumenCharacter>(OtherActor))
	{
		Player->SetCurrentRoom(this); // "지금 이 퍼즐룸에 있다"는 걸 플레이어에게 알림 (죽음→리셋용)
	}
}

void APuzzleRoomVolume::OnEntryEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (ALumenCharacter* Player = Cast<ALumenCharacter>(OtherActor))
	{
		Player->ClearCurrentRoomIfMatches(this);
	}
}

void APuzzleRoomVolume::ResetRoom()
{
	// SensorGroupID 문자열 매칭이 아니라, 에디터에서 명시적으로 지정한 Sensors 배열만 리셋
	// (그룹 없는 센서가 여러 방에 있을 때 NAME_None끼리 매칭돼 서로 잘못 리셋되는 문제 방지)
	bool bAnyWasActivated = false;

	for (APuzzleSensorActor* Sensor : Sensors)
	{
		if (Sensor)
		{
			if (Sensor->IsActivated())
			{
				bAnyWasActivated = true;
			}
			Sensor->Deactivate();
		}
	}

	// 이미 아무도 안 맞춘 상태였으면(퍼즐 시작 전 죽음) 문을 굳이 다시 닫을 필요 없음
	if (bAnyWasActivated)
	{
		for (ALumenDoorActor* Door : ExitDoors)
		{
			if (Door)
			{
				Door->Close();
			}
		}
	}
}

bool APuzzleRoomVolume::AreAllSensorsActivated() const
{
	if (Sensors.Num() == 0)
	{
		return false; // 센서가 하나도 안 연결돼 있으면 클리어 불가능한 상태이므로 안전하게 false
	}

	for (const APuzzleSensorActor* Sensor : Sensors)
	{
		if (!Sensor || !Sensor->IsActivated())
		{
			return false;
		}
	}
	return true;
}

void APuzzleRoomVolume::NotifySensorActivated()
{
	// 문을 여는 책임은 여기(방)로 통일 — 센서 개별 ExitDoor는 더 이상 사용하지 않음
	if (AreAllSensorsActivated())
	{
		for (ALumenDoorActor* Door : ExitDoors)
		{
			if (Door)
			{
				Door->Open();
			}
		}
	}
}