#include "LightEmitterActor.h"
#include "Components/ArrowComponent.h"
#include "WeaponDataAsset.h"
#include "Lumen/Reflection/Projectile.h"

ALightEmitterActor::ALightEmitterActor()
{
	PrimaryActorTick.bCanEverTick = false;

	DirectionArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("DirectionArrow"));
	RootComponent = DirectionArrow;
	DirectionArrow->ArrowSize = 2.f;
	DirectionArrow->SetHiddenInGame(true); // 방향 표시는 에디터 전용 - 게임 중엔 안 보임
}

void ALightEmitterActor::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("LightEmitterActor::BeginPlay - bStartActive=%d WeaponData=%s ProjectileClass=%s"),
		bStartActive,
		WeaponData ? TEXT("OK") : TEXT("NULL"),
		ProjectileClass ? TEXT("OK") : TEXT("NULL"));

	if (bStartActive && WeaponData && ProjectileClass)
	{
		GetWorldTimerManager().SetTimer(FireTimerHandle, this, &ALightEmitterActor::Fire, FireInterval, true, 0.f);
	}
}

void ALightEmitterActor::Fire()
{
	UE_LOG(LogTemp, Warning, TEXT("LightEmitterActor::Fire called. WeaponData=%s ProjectileClass=%s"),
		WeaponData ? TEXT("OK") : TEXT("NULL"),
		ProjectileClass ? TEXT("OK") : TEXT("NULL"));

	if (!WeaponData || !ProjectileClass) return;

	const FVector BaseDirection = GetActorForwardVector();
	const int32 Count = FMath::Max(1, WeaponData->PelletCount);

	for (int32 i = 0; i < Count; ++i)
	{
		FVector Direction = BaseDirection;

		if (Count > 1)
		{
			const float HalfSpread = WeaponData->SpreadAngleDegrees * 0.5f;
			const float Step = WeaponData->SpreadAngleDegrees / FMath::Max(1, Count - 1);
			const float Jitter = FMath::FRandRange(-WeaponData->PelletJitterDegrees, WeaponData->PelletJitterDegrees);
			Direction = BaseDirection.RotateAngleAxis(-HalfSpread + Step * i + Jitter, FVector::UpVector);
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		if (AProjectile* NewProjectile = GetWorld()->SpawnActor<AProjectile>(ProjectileClass, GetActorLocation(), Direction.Rotation(), SpawnParams))
		{
			NewProjectile->InitFromWeaponData(WeaponData, Direction);
		}
	}
}