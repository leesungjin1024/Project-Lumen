// LumenCharacter.cpp

#include "LumenCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Lumen/Reflection/MirrorActor.h"
#include "Lumen/Reflection/Projectile.h"
#include "Lumen/Public/WeaponDataAsset.h"
#include "Lumen/Public/HeatComponent.h"
#include "HealthComponent.h"
#include "DrawDebugHelpers.h"
#include "LumenCollisionChannels.h" 
#include "Kismet/GameplayStatics.h"
#include "ShieldActor.h"
#include "LumenCombatLibrary.h"
#include "WeaponDataAsset.h"
#include "LumenAimLineComponent.h"
#include "LumenPlayerState.h"
#include "CombatRoomVolume.h"
#include "GameFramework/PlayerStart.h"
#include "RoomVolume.h"
#include "LumenDoorActor.h"
#include "Blueprint/UserWidget.h" 
ALumenCharacter::ALumenCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// ===== 카메라 (4-1: 고정 아이소메트릭) =====
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);

	// 캐릭터가 회전해도 카메라 각도는 절대 안 바뀌게 고정 (고정 아이소메트릭의 핵심)
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bUsePawnControlRotation = false;

	// 4-1-1: 오클루전은 우선 레벨 디자인으로 회피하는 방향으로 결정됐으므로,
	// 카메라 충돌로 인한 자동 줌인은 일단 끔 (필요해지면 true로 바꾸고 4-1-1 로직 추가)
	CameraBoom->bDoCollisionTest = false;

	TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCamera->bUsePawnControlRotation = false;

	// ===== 이동/회전 (4-2: 캐릭터는 이동 방향이 아니라 조준 방향을 보고 회전) =====
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 720.f, 0.f); // 조준 방향으로 빠르게 스냅

	//과열시스템
	HeatComponent = CreateDefaultSubobject<UHeatComponent>(TEXT("HeatComponent"));
	//체력
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	//조준선
	AimLineComp = CreateDefaultSubobject<ULumenAimLineComponent>(TEXT("AimLineComp"));
}

void ALumenCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 카메라 세팅 적용 — BeginPlay에서 해야 블루프린트 디테일 패널 값 수정이 실제로 반영됨
	CameraBoom->TargetArmLength = CameraArmLength;
	CameraBoom->SetRelativeRotation(FRotator(CameraPitch, CameraYaw, 0.f));


	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->bShowMouseCursor = true;
		PC->DefaultMouseCursor = EMouseCursor::Crosshairs; // 기본 화살표 대신 십자선

		FInputModeGameOnly InputMode;
		InputMode.SetConsumeCaptureMouseDown(false); // 캡처 클릭이 게임 입력을 잡아먹지 않게
		PC->SetInputMode(InputMode);

		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("LumenCharacter: DefaultMappingContext가 비어있습니다. 에디터에서 IMC_Default를 할당하세요."));
			}
		}
	}

	CurrentShieldStamina = MaxShieldStamina;
	DefaultMaxWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	
	if (ShieldActorClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		ShieldActor = GetWorld()->SpawnActor<AShieldActor>(ShieldActorClass, GetActorLocation(), GetActorRotation(), SpawnParams);
		if (ShieldActor)
		{
			ShieldActor->AttachToActor(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			ShieldActor->SetActorRelativeLocation(FVector(60.f, 0.f, 0.f)); // 캐릭터 앞쪽 (가안 오프셋)
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("BeginPlay End - bIsShieldActive=%d MaxWalkSpeed=%.1f DefaultMaxWalkSpeed=%.1f"),
		bIsShieldActive, GetCharacterMovement()->MaxWalkSpeed, DefaultMaxWalkSpeed);

	AimLineComp->SetWeaponData(GetCurrentWeaponData());

	if (HealthComponent)
	{
	   HealthComponent->OnDeath.AddDynamic(this, &ALumenCharacter::HandlePlayerDeath);
	}

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (PlayerHUDWidgetClass)
		{
			if (UUserWidget* HUD = CreateWidget<UUserWidget>(PC, PlayerHUDWidgetClass))
			{
				HUD->AddToViewport();
			}
		}
	}

	// 세이브 로드 위치 적용 - PlayerState가 Pawn보다 먼저 생성되므로
	// 이 시점엔 이미 LumenPlayerState::BeginPlay()에서 값이 채워져 있음
	if (ALumenPlayerState* PS = GetPlayerState<ALumenPlayerState>())
	{
		if (PS->LastCheckpointOrder >= 0) // 기본값 -1 = 로드된 세이브 없음(새 게임)
		{
			SetActorLocationAndRotation(
				PS->LastCheckpointTransform.GetLocation(),
				PS->LastCheckpointTransform.GetRotation());
		}
	}
}

void ALumenCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);

	// ===== 진단용 임시 로그 - 각 액션 포인터가 실제로 유효한지 직접 확인 =====
	UE_LOG(LogTemp, Warning, TEXT("=== Input Binding Check ==="));
	UE_LOG(LogTemp, Warning, TEXT("MoveAction: %s"), MoveAction ? TEXT("OK") : TEXT("NULL"));
	UE_LOG(LogTemp, Warning, TEXT("AimAction: %s"), AimAction ? TEXT("OK") : TEXT("NULL"));
	UE_LOG(LogTemp, Warning, TEXT("FireAction: %s"), FireAction ? TEXT("OK") : TEXT("NULL"));
	UE_LOG(LogTemp, Warning, TEXT("ShieldAction: %s"), ShieldAction ? TEXT("OK") : TEXT("NULL"));
	UE_LOG(LogTemp, Warning, TEXT("DashAction: %s"), DashAction ? TEXT("OK") : TEXT("NULL"));
	UE_LOG(LogTemp, Warning, TEXT("SelectRedAction: %s"), SelectRedAction ? TEXT("OK") : TEXT("NULL"));
	UE_LOG(LogTemp, Warning, TEXT("InteractAction: %s"), InteractAction ? TEXT("OK") : TEXT("NULL"));
	UE_LOG(LogTemp, Warning, TEXT("PauseAction: %s"), PauseAction ? TEXT("OK") : TEXT("NULL"));
	UE_LOG(LogTemp, Warning, TEXT("DefaultMappingContext: %s"), DefaultMappingContext ? TEXT("OK") : TEXT("NULL"));
	UE_LOG(LogTemp, Warning, TEXT("=========================="));

	if (MoveAction)
	{
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ALumenCharacter::Move);
	}
	if (AimAction)
	{
		EnhancedInput->BindAction(AimAction, ETriggerEvent::Triggered, this, &ALumenCharacter::AimGamepad);
	}
	if (FireAction)
	{
		EnhancedInput->BindAction(FireAction, ETriggerEvent::Started, this, &ALumenCharacter::StartFire);
		EnhancedInput->BindAction(FireAction, ETriggerEvent::Completed, this, &ALumenCharacter::StopFire);
	}
	if (ShieldAction)
	{
		EnhancedInput->BindAction(ShieldAction, ETriggerEvent::Started, this, &ALumenCharacter::StartShield);
		EnhancedInput->BindAction(ShieldAction, ETriggerEvent::Completed, this, &ALumenCharacter::StopShield);
	}
	if (DashAction)
	{
		EnhancedInput->BindAction(DashAction, ETriggerEvent::Started, this, &ALumenCharacter::Dash);
	}
	if (SelectRedAction)
	{
		EnhancedInput->BindAction(SelectRedAction, ETriggerEvent::Triggered, this, &ALumenCharacter::SelectRed);
	}
	if (SelectBlueAction)
	{
		EnhancedInput->BindAction(SelectBlueAction, ETriggerEvent::Triggered, this, &ALumenCharacter::SelectBlue);
	}
	if (SelectGreenAction)
	{
		EnhancedInput->BindAction(SelectGreenAction, ETriggerEvent::Triggered, this, &ALumenCharacter::SelectGreen);
	}
	if (InteractAction)
	{
		EnhancedInput->BindAction(InteractAction, ETriggerEvent::Triggered, this, &ALumenCharacter::Interact);
	}
	if (PauseAction)
	{
		EnhancedInput->BindAction(PauseAction, ETriggerEvent::Triggered, this, &ALumenCharacter::TogglePause);
	}
}

void ALumenCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 게임패드로 조준 중이 아닐 때만 마우스 커서 기준으로 조준 방향 갱신
	// (마우스와 패드를 동시에 만지는 경우는 드물지만, 패드 입력이 있으면 그쪽을 우선시)
	if (!bIsUsingGamepadAim)
	{
		UpdateMouseAimDirection();
	}

	if (bIsDashing)
	{
		UpdateDash(DeltaTime);
	}
	const float TargetYaw = FMath::RadiansToDegrees(FMath::Atan2(CurrentAimDirection.Y, CurrentAimDirection.X));
	const FRotator TargetRotation = FRotator(0.f, TargetYaw, 0.f);

	AimLineComp->SetAimDirection(FVector(CurrentAimDirection, 0.f));
	UpdateCurrentTarget();

	if (bIsShieldActive)
	{
		// 5-4-1: 예측형 가드는 정밀도가 생명 — 지연 없이 즉시 스냅
		SetActorRotation(TargetRotation);
		if (ShieldActor)
		{
			ShieldActor->UpdateFacingDirection(CurrentAimDirection);
		}
	}
	else
	{
		// 사격 중엔 발사 방향이 CurrentAimDirection을 직접 쓰므로, 몸통 회전은 부드러워도 정밀도에 영향 없음
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, 15.f));
	}

	UpdateShieldStamina(DeltaTime);
}

void ALumenCharacter::UpdateMouseAimDirection()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	FVector WorldLocation, WorldDirection;
	if (!PC->DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
	{
		return;
	}

	// 캐릭터 높이의 수평 평면과 마우스 레이의 교차점을 구함 (4-1/4-2: 반사가 수평 축으로만 이뤄지므로 평면 고정)
	const float PlaneZ = GetActorLocation().Z;
	if (FMath::IsNearlyZero(WorldDirection.Z))
	{
		return; // 카메라가 완전 수평을 보는 경우는 없지만 0 나누기 방지
	}

	const float T = (PlaneZ - WorldLocation.Z) / WorldDirection.Z;
	const FVector HitPoint = WorldLocation + WorldDirection * T;

	const FVector2D Direction2D = FVector2D(HitPoint - GetActorLocation());
	if (!Direction2D.IsNearlyZero())
	{
		CurrentAimDirection = Direction2D.GetSafeNormal();
	}
}

void ALumenCharacter::Move(const FInputActionValue& Value)
{
	if (bIsDashing)
	{
		return; // 대쉬 중엔 CharacterMovementComponent가 별도로 속도를 계산하지 못하게 완전히 차단
	}
	const FVector2D MoveInput = Value.Get<FVector2D>();

	// 테스트 로그 추가
 /*
	UE_LOG(LogTemp, Warning, TEXT("Move called - Input=(%.2f,%.2f) MaxWalkSpeed=%.1f bIsShieldActive=%d"),
		MoveInput.X, MoveInput.Y, GetCharacterMovement()->MaxWalkSpeed, bIsShieldActive);
*/
	// 카메라가 고정 45도 회전이라, 월드 축 그대로 넣으면 화면상 이동 방향과 어긋날 수 있음
	// 4-1에서 카메라 각도를 실험적으로 확정할 때 이 부분도 같이 체감 확인 필요
	AddMovementInput(FVector::ForwardVector, MoveInput.Y);
	AddMovementInput(FVector::RightVector, MoveInput.X);
	if (!MoveInput.IsNearlyZero())
	{
		LastMoveDirection = (FVector::ForwardVector * MoveInput.Y + FVector::RightVector * MoveInput.X).GetSafeNormal();
	}
}

void ALumenCharacter::AimGamepad(const FInputActionValue& Value)
{
	LastGamepadAimInput = Value.Get<FVector2D>();
	bIsUsingGamepadAim = LastGamepadAimInput.SizeSquared() > FMath::Square(GamepadAimDeadZone);

	if (bIsUsingGamepadAim)
	{
		CurrentAimDirection = LastGamepadAimInput.GetSafeNormal();
	}
}

//
UWeaponDataAsset* ALumenCharacter::GetCurrentWeaponData() const
{
	if (const UWeaponDataAsset* const* Found = WeaponDataByColor.Find(CurrentColor))
	{
		return const_cast<UWeaponDataAsset*>(*Found);
	}
	return nullptr;
}


// ===== 아래는 스텁 — 실제 로직은 각 참고 섹션 구현 시점(6-1)에 채움 =====

void ALumenCharacter::StartFire(const FInputActionValue& Value)
{

	//UE_LOG(LogTemp, Log, TEXT("StartFire"));

	if (bIsShieldActive) return; // 5-4-1: 방패 든 동안 사격 차단

	UWeaponDataAsset* Weapon = GetCurrentWeaponData();
	if (!Weapon) return;

	if (CurrentColor == ELumenWeaponColor::Red)
	{
		// 자동 연사: 즉시 1발 쏘고, 이후 FireRate 간격으로 반복
		FireOnce(Weapon);
		const float Interval = Weapon->FireRate > 0.f ? 1.f / Weapon->FireRate : 0.1f;
		GetWorldTimerManager().SetTimer(AutoFireTimerHandle, [this, Weapon]()
			{
				// 과열되면 TryConsumeHeat이 실패 → FireOnce 내부에서 걸러지고 타이머는 그대로 두되 매번 재시도
				// (과열 풀리면 자동으로 다시 발사되는 것이 자연스러움 — 트리거를 계속 누르고 있는 상태이므로)
				FireOnce(Weapon);
			}, Interval, true);
	}
	else
	{
		// 파랑/초록: 반자동, 트리거 1회 = 발사 1세트
		FireOnce(Weapon);
	}
}

void ALumenCharacter::StopFire(const FInputActionValue& Value)
{
	// TODO(2주차, 5-1-1): 발사 중단 -> 발열 냉각 시작
	//UE_LOG(LogTemp, Log, TEXT("StopFire"));
	// 자동연사(빨강) 중이었다면 타이머 정지. 반자동 무기는 애초에 타이머가 없으므로 안전하게 무시됨
	GetWorldTimerManager().ClearTimer(AutoFireTimerHandle);

	// 발열 냉각은 HeatComponent::TickComponent가 발사 여부와 무관하게 항상 진행 중이므로
	// 여기서 별도로 "냉각 시작"을 트리거할 필요 없음 (TODO 해소)
}

void ALumenCharacter::FireOnce(UWeaponDataAsset* Weapon)
{
	if (!HeatComponent || !HeatComponent->TryConsumeHeat(Weapon))
	{
		return; // 과열 중 — 발사 안 됨
	}

	switch (Weapon->FireMode)
	{
	case EWeaponFireMode::Projectile:
		FireProjectileSet(Weapon);
		break;
	case EWeaponFireMode::LineTrace:
		FireLineTrace(Weapon);
		break;
	}
}

void ALumenCharacter::FireProjectileSet(UWeaponDataAsset* Weapon)
{
	if (!ProjectileClass) return;

	const int32 Count = FMath::Max(1, Weapon->PelletCount);
	const FVector BaseDirection = FVector(CurrentAimDirection, 0.f);

	for (int32 i = 0; i < Count; ++i)
	{
		FVector Direction = BaseDirection;

		if (Count > 1)
		{
			const float HalfSpread = Weapon->SpreadAngleDegrees * 0.5f;
			const float Step = Weapon->SpreadAngleDegrees / FMath::Max(1, Count - 1);
			const float Jitter = FMath::FRandRange(-Weapon->PelletJitterDegrees, Weapon->PelletJitterDegrees);
			Direction = BaseDirection.RotateAngleAxis(-HalfSpread + Step * i + Jitter, FVector::UpVector);
		}

		const FVector DesiredOffset = Direction * 100.f;
		FVector SpawnLocation = GetActorLocation() + DesiredOffset;

		FCollisionQueryParams Params(SCENE_QUERY_STAT(MuzzleTrace), false, this);
		FHitResult PreHit;
		if (GetWorld()->LineTraceSingleByChannel(PreHit, GetActorLocation(), SpawnLocation, ECC_ProjectileTrace, Params))
		{
			SpawnLocation = PreHit.ImpactPoint - Direction * 2.f;
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		if (AProjectile* NewProjectile = GetWorld()->SpawnActor<AProjectile>(ProjectileClass, SpawnLocation, Direction.Rotation(), SpawnParams))
		{
			NewProjectile->InitFromWeaponData(Weapon, Direction); // InitVelocity(Direction) 대신
		}
	}
}

void ALumenCharacter::FireLineTrace(UWeaponDataAsset* Weapon)
{
	FVector Direction = FVector(CurrentAimDirection, 0.f).GetSafeNormal();
	const FVector StartLocation = GetActorLocation() + Direction * 100.f;

	ULumenCombatLibrary::FireReflectiveLineTrace(
		this,
		this,
		StartLocation,
		Direction,
		Weapon,
		GetController(),
		this,
		nullptr); // 반사 스파크 VFX — 필요하면 아래처럼 멤버 추가해서 에디터에서 할당}
}

void ALumenCharacter::UpdateShieldStamina(float DeltaTime)
{
	if (bIsShieldActive)
	{
		CurrentShieldStamina = FMath::Max(0.f, CurrentShieldStamina - ShieldDrainRate * DeltaTime);

		if (CurrentShieldStamina <= 0.f && !bShieldOnForcedCooldown)
		{
			// 완전 소진 - 강제로 방패 내림 + 강제 쿨타임 시작 (5-4-1)
			bShieldOnForcedCooldown = true;
			ShieldForcedCooldownTimer = ShieldOverchargeForcedCooldown;
			bIsShieldActive = false;

			if (ShieldActor)
			{
				ShieldActor->SetShieldActive(false);
			}
			GetCharacterMovement()->MaxWalkSpeed = DefaultMaxWalkSpeed;
		}
	}
	else if (bShieldOnForcedCooldown)
	{
		// 강제 쿨타임 중엔 재사용 자체가 막혀있어야 함 (StartShield에서 체크)
		ShieldForcedCooldownTimer -= DeltaTime;
		if (ShieldForcedCooldownTimer <= 0.f)
		{
			bShieldOnForcedCooldown = false;
		}
		// 쿨타임 중에도 게이지는 회복 (HeatComponent와 동일 원칙 - 재사용 시점만 강제로 늦춤)
		CurrentShieldStamina = FMath::Min(MaxShieldStamina, CurrentShieldStamina + ShieldRecoveryRate * DeltaTime);
	}
	else
	{
		// 평상시 - 안 쓰는 동안 자연 회복
		CurrentShieldStamina = FMath::Min(MaxShieldStamina, CurrentShieldStamina + ShieldRecoveryRate * DeltaTime);
	}
}

void ALumenCharacter::StartShield(const FInputActionValue& Value)
{
	// TODO(3주차, 5-4/5-4-1): 방패 들기 시작, 게이지 소모 시작
	UE_LOG(LogTemp, Log, TEXT("StartShield"));
	// 강제 쿨타임 중이거나 게이지가 없으면 못 듦
	if (bShieldOnForcedCooldown || CurrentShieldStamina <= 0.f)
	{
		return;
	}

	// 4-2: 방패 든 동안 사격 차단 - 자동연사 중이었다면 정지
	GetWorldTimerManager().ClearTimer(AutoFireTimerHandle);

	bIsShieldActive = true;

	if (ShieldActor)
	{
		ShieldActor->SetShieldActive(true);
	}
	//조준선 비활성화
	AimLineComp->SetVisible(false);

	GetCharacterMovement()->MaxWalkSpeed = DefaultMaxWalkSpeed * ShieldMoveSpeedMultiplier; // 5-2-1: 이동 허용(감속)
}

void ALumenCharacter::StopShield(const FInputActionValue& Value)
{
	// TODO(3주차, 5-4-1): 방패 내리기, 게이지 회복 시작
	UE_LOG(LogTemp, Log, TEXT("StopShield"));

	bIsShieldActive = false;

	if (ShieldActor)
	{
		ShieldActor->SetShieldActive(false);
	}
	//조준선 활성화
	AimLineComp->SetVisible(true);

	GetCharacterMovement()->MaxWalkSpeed = DefaultMaxWalkSpeed;
}

void ALumenCharacter::Dash(const FInputActionValue& Value)
{
	// TODO(3주차, 4-2/5-8): 대쉬 구현 (우선순위 가장 낮은 항목, 6-1 리스크 참고)
	UE_LOG(LogTemp, Log, TEXT("Dash"));
	StartDash();

}
void ALumenCharacter::UpdateDash(float DeltaTime)
{
	const FVector Step = LastMoveDirection * (DashDistance / DashDuration) * DeltaTime;
	FHitResult Hit;
	AddActorWorldOffset(Step, /*bSweep=*/true, &Hit); // 스윕 켜서 벽 뚫고 나가지 않게
}


void ALumenCharacter::StartDash()
{
	if (bIsDashing || bDashOnCooldown || bIsShieldActive)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Dash] Invulnerable ON at %.2f"), GetWorld()->GetTimeSeconds());

	bIsDashing = true;
	GetCharacterMovement()->Velocity = FVector::ZeroVector; // ← 추가: 대쉬 진입 시 관성 제거

	if (HealthComponent)
	{
		HealthComponent->SetInvulnerable(true);
	}

	GetWorldTimerManager().SetTimer(DashDurationTimerHandle, this, &ALumenCharacter::EndDash, DashDuration, false);
}

void ALumenCharacter::EndDash()
{
	UE_LOG(LogTemp, Warning, TEXT("[Dash] Invulnerable OFF at %.2f"), GetWorld()->GetTimeSeconds());

	bIsDashing = false;
	bDashOnCooldown = true;

	if (HealthComponent)
	{
		HealthComponent->SetInvulnerable(false);
	}

	GetWorldTimerManager().SetTimer(DashCooldownTimerHandle, [this]() { bDashOnCooldown = false; }, DashCooldown, false);
}
void ALumenCharacter::SelectRed(const FInputActionValue& Value) { SelectColor(ELumenWeaponColor::Red); }
void ALumenCharacter::SelectBlue(const FInputActionValue& Value) { SelectColor(ELumenWeaponColor::Blue); }
void ALumenCharacter::SelectGreen(const FInputActionValue& Value) { SelectColor(ELumenWeaponColor::Green); }

void ALumenCharacter::SelectColor(ELumenWeaponColor NewColor)
{
	// TODO(3주차, 5-7): 색 코어 보유 여부 확인 후에만 전환 허용 (지금은 무조건 전환)
	CurrentColor = NewColor;
	AimLineComp->SetWeaponData(GetCurrentWeaponData()); // ← SetWeaponColor에서 변경
	UE_LOG(LogTemp, Log, TEXT("SelectColor: %d"), static_cast<uint8>(CurrentColor));
}

void ALumenCharacter::TogglePause(const FInputActionValue& Value)
{
	// TODO(4주차, 11-1): 일시정지 위젯 생성/제거
	UE_LOG(LogTemp, Log, TEXT("TogglePause"));
}

void ALumenCharacter::SetInteractableMirror(AMirrorActor* Mirror)
{
	CurrentInteractableMirror = Mirror;
}

void ALumenCharacter::ClearInteractableMirror(AMirrorActor* Mirror)
{
	if (CurrentInteractableMirror == Mirror)
	{
		CurrentInteractableMirror = nullptr;
	}
}

void ALumenCharacter::Interact(const FInputActionValue& Value)
{
	UE_LOG(LogTemp, Log, TEXT("Interact"));

	if (CurrentInteractableMirror)
	{
		CurrentInteractableMirror->RotateStep();
		return;
	}

	ALumenDoorActor* NearestDoor = nullptr;
	float NearestDistSq = TNumericLimits<float>::Max();

	for (ALumenDoorActor* Door : OverlappingInteractableDoors)
	{
		if (!Door)
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(GetActorLocation(), Door->GetActorLocation());
		if (DistSq < NearestDistSq)
		{
			NearestDistSq = DistSq;
			NearestDoor = Door;
		}
	}

	if (NearestDoor)
	{
		NearestDoor->ToggleInteract();
	}
}

void ALumenCharacter::HandlePlayerDeath()
{
	// 텔레포트(SetActorLocationAndRotation)가 방 트리거 박스 밖으로 나가는 걸로 처리되면서
	// OnEntryEndOverlap → ClearCurrentRoomIfMatches()가 동기적으로 발동해 CurrentRoom이
	// 지워지는 문제가 실제로 발생함 — 그래서 텔레포트 전에 미리 복사해서 안전하게 보관
	ARoomVolume* RoomToReset = CurrentRoom;

	// APawn::GetPlayerState()는 템플릿이 아니라 APlayerState*를 반환 — Cast로 다운캐스팅 필요
	ALumenPlayerState* PS = Cast<ALumenPlayerState>(GetPlayerState());
	if (!PS)
	{
		return;
	}

	FTransform RespawnTransform = PS->LastCheckpointTransform;

	if (PS->LastCheckpointID.IsNone())
	{
		if (AActor* Start = UGameplayStatics::GetActorOfClass(GetWorld(), APlayerStart::StaticClass()))
		{
			RespawnTransform = Start->GetActorTransform();
		}
		else
		{
			RespawnTransform = GetActorTransform();
		}
	}

	SetActorLocationAndRotation(RespawnTransform.GetLocation(), RespawnTransform.GetRotation());

	// CurrentRoom이 아니라 미리 복사해둔 RoomToReset을 사용 — 텔레포트로 인해 CurrentRoom이
	// 지워졌더라도(방 밖으로 나가는 걸로 처리됐더라도) 리셋은 정확히 실행됨
	if (RoomToReset)
	{
		RoomToReset->ResetRoom();
	}

	if (UHealthComponent* Health = FindComponentByClass<UHealthComponent>())
	{
		Health->ResetHealth();
	}
	if (UHeatComponent* Heat = FindComponentByClass<UHeatComponent>())
	{
		Heat->ResetHeat();
	}
}

void ALumenCharacter::HandleFellOutOfWorld()
{
	// Kill Z는 데미지 판정 자체를 건너뛰고 곧장 사망 처리와 동일한 경로를 재사용
	HandlePlayerDeath();
}

void ALumenCharacter::UpdateCurrentTarget()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector Direction = FVector(CurrentAimDirection, 0.f).GetSafeNormal();
	// FireLineTrace/FireProjectileSet과 동일한 100cm 오프셋 컨벤션 재사용
	// — 캐릭터 앞 60cm에 항상 붙어있는 ShieldActor를 자연히 건너뛰게 됨
	const FVector Start = GetActorLocation() + Direction * 100.f;
	const FVector End = Start + Direction * TargetingTraceRange;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(TargetingTrace), false, this);

	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, ECC_ProjectileTrace, Params);

	AActor* NewTarget = nullptr;
	if (bHit && Hit.GetActor() && Hit.GetActor()->FindComponentByClass<UHealthComponent>())
	{
		NewTarget = Hit.GetActor();
	}

	if (NewTarget != CurrentTargetedActor)
	{
		if (UHealthComponent* PrevHealth = CurrentTargetedActor ? CurrentTargetedActor->FindComponentByClass<UHealthComponent>() : nullptr)
		{
			PrevHealth->SetTargeted(false);
		}

		if (UHealthComponent* NewHealth = NewTarget ? NewTarget->FindComponentByClass<UHealthComponent>() : nullptr)
		{
			NewHealth->SetTargeted(true);
		}

		CurrentTargetedActor = NewTarget;
	}
}