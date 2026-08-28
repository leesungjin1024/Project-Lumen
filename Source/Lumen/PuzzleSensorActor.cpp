// PuzzleSensorActor.cpp

#include "PuzzleSensorActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "LumenCollisionChannels.h" // ECC_ProjectileTrace
#include "PuzzleRoomVolume.h"

APuzzleSensorActor::APuzzleSensorActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SensorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SensorMesh"));
	RootComponent = SensorMesh;

	SensorCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("SensorCollision"));
	SensorCollision->SetupAttachment(SensorMesh);

	// Reflective가 아닌 일반 월드 오브젝트로 등록 — 발사체/레이저가 반사 없이 여기서 막히도록
	// (ProjectileTrace 채널만 Block, Visibility는 Ignore로 둬서 AI LOS는 방해하지 않음)
	SensorCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly); // 명시적으로 켜둠 (기본값 의존 금지)
	SensorCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	SensorCollision->SetCollisionResponseToChannel(ECC_ProjectileTrace, ECR_Block);
	SensorCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
}

void APuzzleSensorActor::NotifyHitByLight(ELumenWeaponColor HitColor)
{
	if (bActivated)
	{
		return;
	}

	if (HitColor == RequiredColor)
	{
		Activate();
	}
}

void APuzzleSensorActor::Activate()
{
	bActivated = true;

	// TODO: 시각/사운드 피드백 (머티리얼 색 변경 등)

	// 문을 여는 판단/실행은 방(PuzzleRoomVolume)에 위임 — 센서가 직접 문을 열지 않음
	if (OwningRoom)
	{
		OwningRoom->NotifySensorActivated();
	}
}