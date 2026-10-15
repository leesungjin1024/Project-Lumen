#include "PuzzleSensorActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "LumenCollisionChannels.h" // ECC_ProjectileTrace
#include "PuzzleRoomVolume.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

namespace
{
	const TCHAR* ColorName(ELumenWeaponColor C)
	{
		switch (C)
		{
		case ELumenWeaponColor::Red:   return TEXT("Red");
		case ELumenWeaponColor::Blue:  return TEXT("Blue");
		case ELumenWeaponColor::Green: return TEXT("Green");
		default:                       return TEXT("?");
		}
	}

	FLinearColor SensorGlowColor(ELumenWeaponColor C)
	{
		switch (C)
		{
		case ELumenWeaponColor::Red:   return FLinearColor(1.f, 0.05f, 0.05f);
		case ELumenWeaponColor::Blue:  return FLinearColor(0.1f, 0.3f, 1.f);
		case ELumenWeaponColor::Green: return FLinearColor(0.1f, 1.f, 0.2f);
		default:                       return FLinearColor::White;
		}
	}
}

APuzzleSensorActor::APuzzleSensorActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SensorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SensorMesh"));
	RootComponent = SensorMesh;

	SensorCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("SensorCollision"));
	SensorCollision->SetupAttachment(SensorMesh);

	// Reflective가 아닌 일반 월드 오브젝트로 등록 — 발사체/레이저가 반사 없이 여기서 막히도록
	// (ProjectileTrace 채널만 Block, Visibility는 Ignore로 둬서 AI LOS는 방해하지 않음)
	SensorCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SensorCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	SensorCollision->SetCollisionResponseToChannel(ECC_ProjectileTrace, ECR_Block);
	SensorCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
}

void APuzzleSensorActor::BeginPlay()
{
	Super::BeginPlay();

	OriginalMaterial = SensorMesh->GetMaterial(0);

	// 자동 연결/검증은 APuzzleRoomVolume::BeginPlay 담당. 여기서는 "방을 지정했는데 그 방의 Sensors/DoorRules 어디에도 없는" 경우만 경고
	if (OwningRoom && !OwningRoom->ContainsSensor(this))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Sensor] %s: OwningRoom(%s)의 Sensors/DoorRules 어디에도 이 센서가 없음 — 판정에 포함되지 않음"),
			*GetName(), *OwningRoom->GetName());
	}
}

void APuzzleSensorActor::NotifyHitByLight(ELumenWeaponColor HitColor)
{
	if (bActivated)
	{
		return;
	}

	if (HitColor != RequiredColor)
	{
		UE_LOG(LogTemp, Log, TEXT("[Sensor] %s: 색 불일치 — 맞은 색 %s / 필요한 색 %s"),
			*GetName(), ColorName(HitColor), ColorName(RequiredColor));

		const double Now = GetWorld()->GetTimeSeconds();
		if (WrongColorSound && Now - LastWrongColorSoundTime > 0.3)
		{
			LastWrongColorSoundTime = Now;
			UGameplayStatics::PlaySoundAtLocation(this, WrongColorSound, GetActorLocation());
		}
		return;
	}

	// 순서형: 아직 차례가 아닌 센서는 맞아도 켜지지 않음
	if (OwningRoom && !OwningRoom->CanSensorActivate(this))
	{
		UE_LOG(LogTemp, Log, TEXT("[Sensor] %s: 아직 차례가 아님 (앞 단계 미완료)"), *GetName());
		return;
	}

	Activate();
}

void APuzzleSensorActor::Activate()
{
	bActivated = true;

	if (ActivateSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ActivateSound, GetActorLocation());
	}

	if (ActivatedMaterial)
	{
		if (!ActivatedMID)
		{
			ActivatedMID = UMaterialInstanceDynamic::Create(ActivatedMaterial, this);
			ActivatedMID->SetVectorParameterValue(TEXT("Color"), SensorGlowColor(RequiredColor));
		}
		SensorMesh->SetMaterial(0, ActivatedMID);
	}

	if (!OwningRoom)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Sensor] %s: 활성화됐지만 OwningRoom이 없음 — 방의 Sensors 배열에 이 센서를 넣을 것"), *GetName());
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[Sensor] %s: 활성화 (%s)"), *GetName(), ColorName(RequiredColor));

	// 문을 열지는 방이 판단 — 센서가 직접 문을 열지 않음
	OwningRoom->NotifySensorActivated();
}

void APuzzleSensorActor::Deactivate()
{
	bActivated = false;

	if (OriginalMaterial)
	{
		SensorMesh->SetMaterial(0, OriginalMaterial);
	}
}