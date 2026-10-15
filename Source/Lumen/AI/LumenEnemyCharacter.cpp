// ALumenEnemyCharacter 구현 — 자세한 설명은 LumenEnemyCharacter.h

#include "LumenEnemyCharacter.h"
#include "WeaponDataAsset.h"
#include "HeatComponent.h"
#include "HealthComponent.h"
#include "Projectile.h"
#include "LumenCombatLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "LumenCollisionChannels.h"

#include "Components/WidgetComponent.h"
#include "Components/CapsuleComponent.h"   // GetCapsuleComponent() 사용에 완전한 타입 필요
#include "LumenAimLineComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/CharacterMovementComponent.h"

ALumenEnemyCharacter::ALumenEnemyCharacter()
{
	// 포탑 회전(플레이어를 일정 속도로 따라 돎)과 텔레그래프 시작점 갱신에 Tick 사용
	PrimaryActorTick.bCanEverTick = true;

	HeatComponent = CreateDefaultSubobject<UHeatComponent>(TEXT("HeatComponent"));

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));

	// 하트 위젯. BeginPlay에서 null 체크 없이 쓰므로 반드시 여기서 생성
	HealthChunksComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthChunksComp"));
	HealthChunksComp->SetupAttachment(GetCapsuleComponent());
	HealthChunksComp->SetWidgetSpace(EWidgetSpace::Screen);
	HealthChunksComp->SetDrawAtDesiredSize(true);          // 하트 개수에 맞춰 자동 크기
	HealthChunksComp->SetPivot(FVector2D(0.5f, 1.f));      // 아래쪽 가운데 기준 → 머리 위에 얹힘
	HealthChunksComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	TelegraphComp = CreateDefaultSubobject<ULumenAimLineComponent>(TEXT("TelegraphComp"));

	// 손에 드는 총 — 콜리전 없음
	WeaponMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMeshComp"));
	WeaponMeshComp->SetupAttachment(GetMesh(), WeaponSocketName);
	WeaponMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMeshComp->SetGenerateOverlapEvents(false);

	// 컨트롤러 회전을 따르지 않음 — 회전은 Tick(포탑 회전)과 ShowTelegraph(방향 스냅)에서 직접 처리
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
		TelegraphComp->SetVisible(false); // 평소엔 숨김, 텔레그래프 중에만 표시
	}

	// 무기 DA에 지정된 총 메시를 손 소켓에 장착
	if (WeaponMeshComp && GetMesh())
	{
		WeaponMeshComp->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, WeaponSocketName);
		WeaponMeshComp->SetStaticMesh(WeaponData ? WeaponData->WeaponMesh.Get() : nullptr);
		WeaponMeshComp->SetRelativeTransform(WeaponData ? WeaponData->WeaponMeshOffset : FTransform::Identity);
	}

	// 하트 위치: 캡슐 윗면 + HeartHeightOffset, 피벗은 아래쪽 가운데 → 머리 위에 얹힘
	const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	HealthChunksComp->SetRelativeLocation(FVector(0.f, 0.f, HalfHeight + HeartHeightOffset));

	if (HealthChunksWidgetClass)
	{
		HealthChunksComp->SetWidgetClass(HealthChunksWidgetClass);
		HealthChunksComp->InitWidget();
		if (auto* W = Cast<ULumenHealthChunksWidget>(HealthChunksComp->GetUserWidgetObject()))
		{
			W->Init(HealthComponent, /*bOnlyWhenTargeted=*/ true);    // 적은 조준 중일 때만 표시
		}
	}

}

// 포탑 메시(BP에서 추가한 TurretMesh 등) → 손에 든 총 → 스켈레탈 메시 순으로
// MuzzleSocketName 소켓을 찾음. 아무 데도 없으면 액터 중심.
// 포탑을 먼저 보는 이유: 총 메시(DA의 WeaponMesh)에도 Muzzle 소켓이 있으면, 포탑 BP에서 보이지 않는
// 손 위치의 총구가 잡혀 엉뚱한 곳에서 발사됨. 숨겨진 메시는 건너뜀
FVector ALumenEnemyCharacter::GetMuzzleLocation() const
{
	auto IsUsable = [this](const UStaticMeshComponent* Comp)
	{
		return Comp && Comp->GetStaticMesh() && Comp->IsVisible() && !Comp->bHiddenInGame
			&& Comp->DoesSocketExist(MuzzleSocketName);
	};

	TInlineComponentArray<UStaticMeshComponent*> StaticMeshes(this);
	for (const UStaticMeshComponent* Comp : StaticMeshes)
	{
		if (Comp != WeaponMeshComp && IsUsable(Comp))
		{
			return Comp->GetSocketLocation(MuzzleSocketName);
		}
	}

	if (IsUsable(WeaponMeshComp))
	{
		return WeaponMeshComp->GetSocketLocation(MuzzleSocketName);
	}

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

	// BT가 bOverheated로 이미 막지만, 태스크 구성 실수에 대비한 이중 방어
	// → "과열 = 확실한 반격 구간"이 코드 차원에서도 보장됨
	if (!HeatComponent->TryConsumeHeat(WeaponData))
	{
		return;
	}

	// 텔레그래프로 보여준 방향이 있으면 그쪽으로 발사 (태스크가 넘긴 "현재 플레이어 방향"은 무시)
	if (bHasLockedAim)
	{
		Direction = LockedAimDirection;
		bHasLockedAim = false;   // 한 번 쏘면 해제 → 다음 텔레그래프에서 새로 고정
	}
	Direction = Direction.GetSafeNormal();

	// 발사음 — 펠릿 수와 무관하게 발사 1회당 1번
	if (WeaponData->FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, WeaponData->FireSound, GetMuzzleLocation());
	}

	// 사격 모션 (UpperBody 슬롯)
	if (FireMontage)
	{
		PlayAnimMontage(FireMontage);
	}

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
	const FVector Center = GetActorLocation();

	// 파랑(샷건)이면 PelletCount만큼, 그 외(빨강)는 1발
	const int32 PelletCount = FMath::Max(1, WeaponData->PelletCount);
	const int32 ThisVolley = AProjectile::NewVolleyId();   // 같은 발사의 펠릿은 대상당 1회만 데미지

	for (int32 i = 0; i < PelletCount; ++i)
	{
		FVector PelletDirection = Direction;

		if (PelletCount > 1)
		{
			// SpreadAngleDegrees 안에 균등 분산 + 펠릿별 미세 jitter
			const float Half = WeaponData->SpreadAngleDegrees * 0.5f;
			const float Step = WeaponData->SpreadAngleDegrees / FMath::Max(1, PelletCount - 1);
			const float BaseAngle = -Half + Step * i;
			const float Jitter = FMath::FRandRange(-WeaponData->PelletJitterDegrees, WeaponData->PelletJitterDegrees);
			const float FinalAngle = BaseAngle + Jitter;

			PelletDirection = Direction.RotateAngleAxis(FinalAngle, FVector::UpVector);
		}

		// 사전 트레이스로 스폰 위치 보정 — 몸 중심 → 총구 사이에 벽/거울이 있으면 그 앞에서 스폰
		FVector SpawnLocation = Muzzle;
		FHitResult PreHit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);
		if (World->LineTraceSingleByChannel(PreHit, Center, Muzzle, ECC_ProjectileTrace, Params))
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
			Projectile->VolleyId = ThisVolley;
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

	HideTelegraph_Implementation();

	// Screen Space 위젯은 SetActorHiddenInGame만으로 안 숨겨질 수 있어 직접 숨김
	if (HealthChunksComp)
	{
		HealthChunksComp->SetVisibility(false);
	}

	bHasLockedAim = false;
	bIsDead = true;          // Tick의 포탑 회전/텔레그래프 갱신 정지
	TrackTarget.Reset();

	// 콜리전은 즉시 끔 (쓰러지는 몸에 발사체가 막히지 않게).
	// 콜리전을 끄면 캐릭터가 바닥을 뚫고 떨어지므로 이동부터 정지
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->DisableMovement();
	}
	SetActorEnableCollision(false);

	// 쓰러지는 모션 → DeathHideDelay 뒤 사라짐. 몽타주가 없거나 지연이 0이면 즉시 숨김
	if (DeathMontage && DeathHideDelay > 0.f)
	{
		StopAnimMontage();   // 사격 모션 중이었으면 끊고
		PlayAnimMontage(DeathMontage);
		GetWorldTimerManager().SetTimer(DeathHideTimerHandle, this, &ALumenEnemyCharacter::HideAfterDeath,
			DeathHideDelay, false);
	}
	else
	{
		HideAfterDeath();
	}
}

void ALumenEnemyCharacter::HideAfterDeath()
{
	// 몸/총 숨김 (하트 위젯은 HandleDeath에서 이미 숨김)
	SetActorHiddenInGame(true);
}

void ALumenEnemyCharacter::ShowTelegraph_Implementation(float Duration, float WidthDegrees, FVector AimDirection)
{
	// 태스크가 넘긴 방향(액터 중심 기준)을 "총구 → 플레이어"로 다시 계산 — 총구가 중심에서 벗어나 있어도 정확히 플레이어를 향함
	if (const AActor* Target = TrackTarget.Get())
	{
		const FVector FromMuzzle = (Target->GetActorLocation() - GetMuzzleLocation()).GetSafeNormal2D();
		if (!FromMuzzle.IsNearlyZero())
		{
			AimDirection = FromMuzzle;
		}
	}

	LockedAimDirection = AimDirection.GetSafeNormal2D();
	bHasLockedAim = !LockedAimDirection.IsNearlyZero();

	// 공격 예고음 — 화면 밖 적도 소리 방향으로 위치를 알 수 있게 (리스너 = 캐릭터 위치)
	if (WeaponData && WeaponData->TelegraphSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, WeaponData->TelegraphSound, GetActorLocation());
	}
	// 조준 방향으로 즉시 스냅. 이후 FireWeapon까지는 bHasLockedAim 때문에 Tick 회전이 멈춤
	FRotator NewRot = AimDirection.Rotation();
	NewRot.Pitch = 0.f;
	NewRot.Roll = 0.f;
	SetActorRotation(NewRot);

	if (TelegraphComp)
	{
		TelegraphComp->SetOriginLocation(GetMuzzleLocation());
		TelegraphComp->SetAimDirection(AimDirection);
		// 페이드인 → 채움 → 번쩍 연출 시작. Duration은 BTTask_LumenTelegraph가 넘겨준
		// WeaponData->TelegraphDuration 그대로 — BT 대기 시간과 번쩍 타이밍이 같은 값에서 나옴
		TelegraphComp->StartTelegraph(Duration);
	}

	UE_LOG(LogTemp, Verbose, TEXT("[%s] Telegraph 표시 (Duration=%.2f, Width=%.1f)"), *GetName(), Duration, WidthDegrees);
}

void ALumenEnemyCharacter::HideTelegraph_Implementation()
{
	if (TelegraphComp)
	{
		TelegraphComp->StopTelegraph();
	}
	UE_LOG(LogTemp, Verbose, TEXT("[%s] Telegraph 해제"), *GetName());
}

void ALumenEnemyCharacter::CancelTelegraph()
{
	HideTelegraph();
	bHasLockedAim = false;
}
// ===== 포탑 =====

void ALumenEnemyCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 텔레그래프 시작점을 총구에 맞춤 (사격 모션으로 총구가 움직여도 따라감)
	if (TelegraphComp && !bIsDead)
	{
		TelegraphComp->SetOriginLocation(GetMuzzleLocation());
	}

	// 죽었거나, 텔레그래프로 방향을 고정한 상태(ShowTelegraph ~ FireWeapon)에선 회전하지 않음
	// → "조준선이 보인 곳으로 쏜다"는 예측형 가드 원칙 유지
	if (bIsDead || bHasLockedAim)
	{
		return;
	}
	if (!bTrackWhileOverheated && HeatComponent && HeatComponent->IsOverheated())
	{
		return;
	}

	const AActor* Target = TrackTarget.Get();
	if (!Target)
	{
		return;
	}

	const FVector ToTarget = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	if (ToTarget.IsNearlyZero())
	{
		return;
	}

	// 초당 TurnRateDegrees로 일정하게 회전 (FixedTurn은 -180/180 경계도 최단 방향으로 처리)
	const float NewYaw = FMath::FixedTurn(GetActorRotation().Yaw, ToTarget.Rotation().Yaw, TurnRateDegrees * DeltaSeconds);
	SetActorRotation(FRotator(0.f, NewYaw, 0.f));
}

bool ALumenEnemyCharacter::IsAimedAt(const AActor* Target) const
{
	if (!Target)
	{
		return false;
	}

	const FVector ToTarget = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
	if (ToTarget.IsNearlyZero())
	{
		return true;
	}

	const float CosTolerance = FMath::Cos(FMath::DegreesToRadians(AimToleranceDegrees));
	return FVector::DotProduct(Forward, ToTarget) >= CosTolerance;
}
