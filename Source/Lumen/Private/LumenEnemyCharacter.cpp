// LumenEnemyCharacter.cpp

#include "LumenEnemyCharacter.h"
#include "WeaponDataAsset.h"
#include "HeatComponent.h"
#include "HealthComponent.h"
#include "Lumen/Reflection/Projectile.h"
#include "LumenCombatLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Lumen/LumenAimLineComponent.h"

ALumenEnemyCharacter::ALumenEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = false; // AI는 BT/Service 틱으로 충분, 캐릭터 자체 틱 불필요 (5-5-3)

	HeatComponent = CreateDefaultSubobject<UHeatComponent>(TEXT("HeatComponent"));
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	TelegraphComp = CreateDefaultSubobject<ULumenAimLineComponent>(TEXT("TelegraphComp")); // ← 추가

	// 5-5-3의 AttackState 흐름과 무관하게, 캐릭터 자체 회전은 조준 방향을 바로 향하도록
	// (아이소메트릭 카메라·명확성 우선 철학, 4-1) — 플레이어와 동일 원칙
	bUseControllerRotationYaw = false;
}

void ALumenEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (HealthComponent)
	{
		HealthComponent->OnDeath.AddDynamic(this, &ALumenEnemyCharacter::HandleDeath);
	}
	if (TelegraphComp && WeaponData)
	{
		TelegraphComp->SetWeaponData(WeaponData);
	}
	if (TelegraphComp)
	{
		TelegraphComp->SetVisible(false); // 평소엔 안 보임 - Telegraph 태스크 중에만 켜짐
	}
}

FVector ALumenEnemyCharacter::GetMuzzleLocation() const
{
	if (GetMesh() && GetMesh()->DoesSocketExist(MuzzleSocketName))
	{
		return GetMesh()->GetSocketLocation(MuzzleSocketName);
	}
	return GetActorLocation();
}

void ALumenEnemyCharacter::FireWeapon(FVector Direction)
{
	if (!WeaponData || !HeatComponent)
	{
		return;
	}

	// 발열 체크는 여기서도 한 번 더 방어적으로 확인 (BT 쪽에서 bOverheated로 이미 막았어야 하지만,
	// Task 순서가 어긋나는 실수를 대비한 이중 방어 — 5-5-1의 "발열=확실한 취약 구간" 설계 의도를
	// 코드에서도 깨지지 않게 보장)
	if (!HeatComponent->TryConsumeHeat(WeaponData))
	{
		return;
	}

	Direction = Direction.GetSafeNormal();

	switch (WeaponData->FireMode)
	{
	case EWeaponFireMode::Projectile:
		FireProjectileSet(Direction);
		break;
	case EWeaponFireMode::LineTrace:
		FireLineTraceWeapon(Direction);
		break;
	default:
		break;
	}
}

void ALumenEnemyCharacter::FireProjectileSet(FVector Direction)
{
	UWorld* World = GetWorld();
	if (!World || !ProjectileClass)
	{
		return;
	}

	const FVector Muzzle = GetMuzzleLocation();

	// 파랑(샷건)이면 PelletCount만큼, 그 외(빨강)는 1발
	const int32 PelletCount = FMath::Max(1, WeaponData->PelletCount);

	for (int32 i = 0; i < PelletCount; ++i)
	{
		FVector PelletDirection = Direction;

		if (PelletCount > 1)
		{
			// 균등 분산 + 미세 jitter (3주차 §2-1/§2-3와 동일한 원칙)
			const float Half = WeaponData->SpreadAngleDegrees * 0.5f;
			const float Step = WeaponData->SpreadAngleDegrees / FMath::Max(1, PelletCount - 1);
			const float BaseAngle = -Half + Step * i;
			const float Jitter = FMath::FRandRange(-WeaponData->PelletJitterDegrees, WeaponData->PelletJitterDegrees);
			const float FinalAngle = BaseAngle + Jitter;

			PelletDirection = Direction.RotateAngleAxis(FinalAngle, FVector::UpVector);
		}

		// 사전 트레이스로 스폰 위치 보정 (거울/벽에 붙어 쏘는 경우, 3주차 §2-3와 동일 패턴)
		FVector SpawnLocation = Muzzle;
		FHitResult PreHit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);
		if (World->LineTraceSingleByChannel(PreHit, Muzzle, Muzzle + PelletDirection * 50.f, ECC_Visibility, Params))
		{
			SpawnLocation = PreHit.ImpactPoint - PelletDirection * 5.f;
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = this;

		AProjectile* Projectile = World->SpawnActor<AProjectile>(
			ProjectileClass,
			SpawnLocation,
			PelletDirection.Rotation(),
			SpawnParams);

		if (Projectile)
		{
			Projectile->InitFromWeaponData(WeaponData, PelletDirection);
		}
	}
}

void ALumenEnemyCharacter::FireLineTraceWeapon(FVector Direction)
{
	ULumenCombatLibrary::FireReflectiveLineTrace(
		this,
		this,
		GetMuzzleLocation(),
		Direction,
		WeaponData,
		GetController(),
		this,
		ReflectionSparkVFX);
}

void ALumenEnemyCharacter::HandleDeath()
{
	if (AAIController* AICon = Cast<AAIController>(GetController()))
	{
		if (UBrainComponent* Brain = AICon->GetBrainComponent())
		{
			Brain->StopLogic(TEXT("Died"));
		}
	}

	if (HeatComponent)
	{
		HeatComponent->SetComponentTickEnabled(false);
	}
}

void ALumenEnemyCharacter::ShowTelegraph_Implementation(float Duration, float WidthDegrees, FVector AimDirection)
{
	// 텔레그래프 시작 시점에 타겟 방향으로 1회 스냅 회전 (틱 없이도 충분 - 5-5-3)
	FRotator NewRot = AimDirection.Rotation();
	NewRot.Pitch = 0.f;
	NewRot.Roll = 0.f;
	SetActorRotation(NewRot);

	if (TelegraphComp)
	{
		TelegraphComp->SetAimDirection(AimDirection);
		TelegraphComp->SetVisible(true);
	}

	UE_LOG(LogTemp, Verbose, TEXT("[%s] Telegraph 표시 (Duration=%.2f, Width=%.1f)"), *GetName(), Duration, WidthDegrees);
}

void ALumenEnemyCharacter::HideTelegraph_Implementation()
{
	if (TelegraphComp)
	{
		TelegraphComp->SetVisible(false);
	}
	UE_LOG(LogTemp, Verbose, TEXT("[%s] Telegraph 해제"), *GetName());
}
