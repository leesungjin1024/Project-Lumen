// ALumenCharacter 구현 — 자세한 설명은 LumenCharacter.h

#include "LumenCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "MirrorActor.h"
#include "Projectile.h"
#include "WeaponDataAsset.h"
#include "HeatComponent.h"
#include "HealthComponent.h"
#include "DrawDebugHelpers.h"
#include "LumenCollisionChannels.h" 
#include "Kismet/GameplayStatics.h"
#include "ShieldActor.h"
#include "LumenCombatLibrary.h"
#include "LumenAimLineComponent.h"
#include "LumenPlayerState.h"
#include "GameFramework/PlayerStart.h"
#include "RoomVolume.h"
#include "LumenDoorActor.h"
#include "Blueprint/UserWidget.h" 
#include "Kismet/KismetMaterialLibrary.h"
#include "LumenGameInstance.h"  
#include "LumenInteractable.h"
#include "LumenInteractPromptWidget.h"
#include "Components/WidgetComponent.h"
#include "Components/CapsuleComponent.h" // GetCapsuleComponent() 사용 — 완전한 타입 필요
#include "LumenPlayerGaugesWidget.h"
#include "LumenMusicSubsystem.h"
#include "Sound/SoundBase.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Animation/AnimMontage.h"
#include "Camera/PlayerCameraManager.h"       // 사망 페이드

ALumenCharacter::ALumenCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// ===== 카메라 (고정 아이소메트릭) =====
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);

	// 캐릭터가 회전해도 카메라 각도는 절대 안 바뀌게 고정 (고정 아이소메트릭의 핵심)
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bUsePawnControlRotation = false;

	// 오클루전은 레벨 디자인 + 벽 페이드(WallFadeMPC)로 처리하므로 카메라 충돌 자동 줌인은 끔
	CameraBoom->bDoCollisionTest = false;

	TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCamera->bUsePawnControlRotation = false;

	// ===== 이동/회전 (캐릭터는 이동 방향이 아니라 조준 방향을 보고 회전 — Tick에서 처리) =====
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 720.f, 0.f);

	HeatComponent = CreateDefaultSubobject<UHeatComponent>(TEXT("HeatComponent"));
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));

	// 하트 위젯 — BeginPlay에서 이 컴포넌트를 바로 쓰므로 생성이 빠지면 nullptr 접근으로 크래시
	// 위치/피벗은 BeginPlay에서 머리 위로 다시 설정
	HealthChunksComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthChunksComp"));
	HealthChunksComp->SetupAttachment(GetCapsuleComponent());
	HealthChunksComp->SetWidgetSpace(EWidgetSpace::Screen);
	HealthChunksComp->SetDrawAtDesiredSize(true);          // 하트 개수에 맞춰 자동 크기
	HealthChunksComp->SetPivot(FVector2D(0.5f, 1.f));      // 아래쪽 가운데 기준 → 머리 위에 올라앉음 (위치는 BeginPlay에서 캡슐 높이로 설정)
	HealthChunksComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 캐릭터 주변 게이지 — 캐릭터 중심에 두고 위젯 안에서 좌/우/아래로 배치
	GaugesComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("GaugesComp"));
	GaugesComp->SetupAttachment(GetCapsuleComponent());
	GaugesComp->SetWidgetSpace(EWidgetSpace::Screen);
	GaugesComp->SetDrawAtDesiredSize(true);
	GaugesComp->SetPivot(FVector2D(0.5f, 0.5f));   // 가운데 기준 → 캐릭터를 감쌈
	GaugesComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	AimLineComp = CreateDefaultSubobject<ULumenAimLineComponent>(TEXT("AimLineComp"));

	// 손에 드는 총 — 콜리전 없음 (ProjectileTrace를 막으면 발사체가 자기 총에 걸림)
	WeaponMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMeshComp"));
	WeaponMeshComp->SetupAttachment(GetMesh(), WeaponSocketName);
	WeaponMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMeshComp->SetGenerateOverlapEvents(false);
}

void ALumenCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 카메라 세팅 적용 — BeginPlay에서 해야 블루프린트 디테일 패널 값 수정이 실제로 반영됨
	CameraBoom->TargetArmLength = CameraArmLength;
	CameraBoom->SetRelativeRotation(FRotator(CameraPitch, CameraYaw, 0.f));
	// 기본 카메라 상태 기억 (퍼즐룸에서 나올 때 여기로 복귀)
	DefaultBoomRotation = CameraBoom->GetRelativeRotation();
	DefaultArmLength = CameraBoom->TargetArmLength;
	TargetBoomRotation = DefaultBoomRotation;
	DesiredArmLength = DefaultArmLength;

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->bShowMouseCursor = true;
		PC->DefaultMouseCursor = EMouseCursor::Crosshairs; // 기본 화살표 대신 십자선

		FInputModeGameOnly InputMode;
		InputMode.SetConsumeCaptureMouseDown(false); // 뷰포트 캡처용 첫 클릭이 소모되지 않고 사격 입력으로 들어가게
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
			ShieldActor->SetActorRelativeLocation(FVector(60.f, 0.f, 0.f)); // 캐릭터 앞 60cm
		}
	}

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

	// 하트 위젯 위치: 캡슐 꼭대기 + HeartHeightOffset (머리 위)
	const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	HealthChunksComp->SetRelativeLocation(FVector(0.f, 0.f, HalfHeight + HeartHeightOffset));

	if (HealthChunksWidgetClass)
	{
		HealthChunksComp->SetWidgetClass(HealthChunksWidgetClass);
		HealthChunksComp->InitWidget();
		if (auto* W = Cast<ULumenHealthChunksWidget>(HealthChunksComp->GetUserWidgetObject()))
		{
			W->Init(HealthComponent, /*bOnlyWhenTargeted=*/ false);   // 플레이어는 항상 표시 (적은 true)
		}
	}

	if (GaugesWidgetClass)
	{
		GaugesComp->SetWidgetClass(GaugesWidgetClass);
		GaugesComp->InitWidget();
		if (auto* G = Cast<ULumenPlayerGaugesWidget>(GaugesComp->GetUserWidgetObject()))
		{
			G->Init(this);
		}
	}

	// 총 메시 — BP에서 소켓 이름을 바꿨을 수 있으므로 BeginPlay에서 다시 붙임
	if (WeaponMeshComp && GetMesh())
	{
		WeaponMeshComp->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, WeaponSocketName);
		UpdateWeaponMesh();
	}

	// 평소 배경음 시작
	if (ULumenMusicSubsystem* Music = GetWorld()->GetSubsystem<ULumenMusicSubsystem>())
	{
		Music->SetDefaultMusic(DefaultLevelMusic);
	}

	// 소리를 듣는 위치를 카메라(멀리 위쪽)가 아니라 캐릭터 위치로.
	// CameraBoom에 붙이는 이유: 위치는 캐릭터와 같고, 회전은 카메라 방향으로 고정이라
	// 캐릭터가 조준하며 돌아도 "화면 왼쪽 소리 = 왼쪽 귀"가 유지됨
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetAudioListenerOverride(CameraBoom, FVector::ZeroVector, FRotator::ZeroRotator);
	}

	// E 상호작용 프롬프트 — Screen Space 위젯 컴포넌트.
	// 캐릭터를 따라다니지 않고(AbsoluteLocation) 매 틱 "가장 가까운 상호작용 대상" 위로 옮겨짐 (UpdateInteractPrompt)
	if (InteractPromptWidgetClass)
	{
		InteractPromptComp = NewObject<UWidgetComponent>(this, TEXT("InteractPrompt"));
		InteractPromptComp->SetWidgetSpace(EWidgetSpace::Screen);
		InteractPromptComp->SetWidgetClass(InteractPromptWidgetClass);
		InteractPromptComp->SetDrawAtDesiredSize(true);
		InteractPromptComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		InteractPromptComp->SetupAttachment(GetRootComponent());
		InteractPromptComp->RegisterComponent();
		InteractPromptComp->SetUsingAbsoluteLocation(true);
		InteractPromptComp->SetVisibility(false);
	}

	// 세이브 로드 위치 적용 — PlayerState가 Pawn보다 먼저 생성되므로 위치 적용은 여기서.
	// 이 시점엔 이미 LumenPlayerState::BeginPlay()에서 값이 채워져 있음
	// SetActorLocationAndRotation 사용: 트랜스폼 통째로 넣으면 체크포인트 Scale이 전이됨
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
		// Started = 누른 순간 1회, Completed = 뗀 순간 (거울: 짧게=정방향, 길게=역방향 판정용)
		EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, this, &ALumenCharacter::Interact);
		EnhancedInput->BindAction(InteractAction, ETriggerEvent::Completed, this, &ALumenCharacter::InteractReleased);
	}
	if (PauseAction)
	{
		EnhancedInput->BindAction(PauseAction, ETriggerEvent::Started, this, &ALumenCharacter::OnPauseInput);
	}
}

void ALumenCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 사망 연출 중엔 조준/회전/발소리 등 전부 멈춤 (카메라 보간만 유지)
	if (bIsDying)
	{
		UpdateCameraBlend(DeltaTime);
		return;
	}

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

	// 조준선은 실제 발사와 같은 방향/시작점(총구)을 사용
	AimLineComp->SetAimDirection(GetFireDirection());
	AimLineComp->SetOriginLocation(GetMuzzleLocation());
	UpdateCurrentTarget();

	if (bIsDashing)
	{
		// 구르는 동안은 구르는 방향을 봄 (StartDash에서 스냅) — 끝나면 아래 보간으로 조준 방향 복귀
	}
	else if (bIsShieldActive)
	{
		// 예측형 가드는 정밀도가 생명 — 지연 없이 즉시 스냅
		SetActorRotation(TargetRotation);
		if (ShieldActor)
		{
			ShieldActor->UpdateFacingDirection(CurrentAimDirection);
		}
	}
	else
	{
		// 발사 방향은 몸 회전이 아니라 GetFireDirection()으로 따로 계산하므로, 몸통 회전은 부드러워도 정밀도에 영향 없음
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, 15.f));
	}

	UpdateShieldStamina(DeltaTime);
	UpdateCameraBlend(DeltaTime);

	UpdateInteractPrompt();
	UpdateFootsteps(DeltaTime);

	// 벽 페이드 머티리얼이 캐릭터-카메라 사이 벽을 투명하게 만들 때 쓰는 위치
	if (WallFadeMPC)
	{
		UKismetMaterialLibrary::SetVectorParameterValue(
			this,
			WallFadeMPC,
			FName("PlayerLocation"),
			FLinearColor(GetActorLocation())          // MPC 벡터 파라미터는 FLinearColor로 전달
		);

		if (TopDownCamera)
		{
			UKismetMaterialLibrary::SetVectorParameterValue(
				this,
				WallFadeMPC,
				FName("CameraLocation"),
				FLinearColor(TopDownCamera->GetComponentLocation())
			);
		}
	}
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

	// 캐릭터 높이의 수평 평면과 마우스 레이의 교차점을 구함 (사격/반사가 수평면에서만 이뤄지므로 평면 고정)
	const float PlaneZ = GetActorLocation().Z;
	if (FMath::IsNearlyZero(WorldDirection.Z))
	{
		return; // 카메라가 완전 수평을 보는 경우는 없지만 0 나누기 방지
	}

	const float T = (PlaneZ - WorldLocation.Z) / WorldDirection.Z;
	const FVector HitPoint = WorldLocation + WorldDirection * T;
	CursorAimPoint = HitPoint;          // GetFireDirection()에서 총구→커서 방향 계산에 사용
	bHasCursorAimPoint = true;

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
		return; // 대쉬 이동은 UpdateDash가 직접 처리 — 이동 입력이 섞이지 않게 차단
	}
	const FVector2D MoveInput = Value.Get<FVector2D>();

// 주의: 입력을 월드 X/Y 축 그대로 넣음 — 카메라 Yaw(기본 45도)만큼 화면상 방향과 어긋날 수 있음
// 화면 기준 이동이 필요하면 CameraYaw로 입력을 회전시켜야 함
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

UWeaponDataAsset* ALumenCharacter::GetCurrentWeaponData() const
{
	if (const UWeaponDataAsset* const* Found = WeaponDataByColor.Find(CurrentColor))
	{
		return const_cast<UWeaponDataAsset*>(*Found);
	}
	return nullptr;
}


// ===== 사격 =====

void ALumenCharacter::StartFire(const FInputActionValue& Value)
{
	if (bIsShieldActive) return; // 방패 든 동안 사격 차단

	UWeaponDataAsset* Weapon = GetCurrentWeaponData();
	if (!Weapon) return;

	if (CurrentColor == ELumenWeaponColor::Red)
	{
		// 자동 연사: 즉시 1발 쏘고, 이후 FireRate 간격으로 반복
		FireOnce(Weapon);
		const float Interval = Weapon->FireRate > 0.f ? 1.f / Weapon->FireRate : 0.1f;
		GetWorldTimerManager().SetTimer(AutoFireTimerHandle, [this, Weapon]()
			{
				// 과열 중엔 FireOnce 안에서 걸러지지만 타이머는 유지해 매번 재시도
				// (트리거를 계속 누르고 있으므로 과열이 풀리면 자동으로 다시 발사되는 게 자연스러움)
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
	// 자동연사(빨강) 타이머 정지. 반자동 무기는 타이머가 없으므로 그냥 무시됨
	GetWorldTimerManager().ClearTimer(AutoFireTimerHandle);

	// 발열 냉각은 HeatComponent::TickComponent가 발사 여부와 무관하게 항상 진행하므로
	// 여기서 별도로 "냉각 시작"을 트리거할 필요 없음
}

void ALumenCharacter::FireOnce(UWeaponDataAsset* Weapon)
{
	if (!HeatComponent || !HeatComponent->TryConsumeHeat(Weapon))
	{
		// 과열 중 방아쇠 — 빈 격발음 (자동연사 재시도마다 울리지 않게 간격 제한)
		const double Now = GetWorld()->GetTimeSeconds();
		if (DryFireSound && Now - LastDryFireTime >= DryFireInterval)
		{
			LastDryFireTime = Now;
			UGameplayStatics::PlaySoundAtLocation(this, DryFireSound, GetMuzzleLocation());
		}
		return; // 과열 중 — 발사 안 됨
	}

	// 발사음 — 펠릿 수와 무관하게 발사 1회당 1번
	if (Weapon->FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Weapon->FireSound, GetMuzzleLocation());
	}

	// 사격 모션 (UpperBody 슬롯 — 이동 애니메이션 위에 상체만 덮어씀)
	if (FireMontage)
	{
		PlayAnimMontage(FireMontage);
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
	const FVector BaseDirection = GetFireDirection();
	const int32 ThisVolley = AProjectile::NewVolleyId();   // 파랑 펠릿: 같은 VolleyId는 대상당 1회만 데미지
	const FVector Muzzle = GetMuzzleLocation();

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

		// 몸 중심 → 총구 사이에 벽/거울이 끼어 있으면(벽에 붙어 쏠 때) 그 앞에서 스폰
		// 거울/방패는 Visibility를 무시하므로 ProjectileTrace 채널로 검사, 내 방패는 제외
		FVector SpawnLocation = Muzzle;

		FCollisionQueryParams Params(SCENE_QUERY_STAT(MuzzleTrace), false, this);
		if (ShieldActor) Params.AddIgnoredActor(ShieldActor);
		FHitResult PreHit;
		if (GetWorld()->LineTraceSingleByChannel(PreHit, GetActorLocation(), SpawnLocation, ECC_ProjectileTrace, Params))
		{
			SpawnLocation = PreHit.ImpactPoint - Direction * 2.f;
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		if (AProjectile* NewProjectile = GetWorld()->SpawnActor<AProjectile>(ProjectileClass, SpawnLocation, Direction.Rotation(), SpawnParams))
		{
			NewProjectile->VolleyId = ThisVolley;
			NewProjectile->InitFromWeaponData(Weapon, Direction);
			NewProjectile->ScaleMaxRange(RangeMultiplier); // 퍼즐룸 등 방별 사거리 배율
		}
	}
}

void ALumenCharacter::FireLineTrace(UWeaponDataAsset* Weapon)
{
	FVector Direction = GetFireDirection();
	FVector StartLocation = GetMuzzleLocation();

	// 벽에 붙어 쏠 때 총구가 벽 안에 들어가 있으면 벽 앞에서 시작 (FireProjectileSet과 동일)
	FCollisionQueryParams PreParams(SCENE_QUERY_STAT(MuzzleTrace), false, this);
	if (ShieldActor) PreParams.AddIgnoredActor(ShieldActor);
	FHitResult PreHit;
	if (GetWorld()->LineTraceSingleByChannel(PreHit, GetActorLocation(), StartLocation, ECC_ProjectileTrace, PreParams))
	{
		StartLocation = PreHit.ImpactPoint - Direction * 2.f;
	}

	ULumenCombatLibrary::FireReflectiveLineTrace(
		this,
		this,
		StartLocation,
		Direction,
		Weapon,
		GetController(),
		this,
		nullptr,          // 반사 스파크 VFX 없음 — 필요하면 멤버로 추가해 에디터에서 할당 후 전달
		nullptr,          // 사거리 끝 VFX
		RangeMultiplier); // 퍼즐룸 등 방별 사거리 배율
}

void ALumenCharacter::UpdateShieldStamina(float DeltaTime)
{
	if (bIsShieldActive)
	{
		CurrentShieldStamina = FMath::Max(0.f, CurrentShieldStamina - ShieldDrainRate * DeltaTime);

		if (CurrentShieldStamina <= 0.f && !bShieldOnForcedCooldown)
		{
			// 완전 소진 — 강제로 방패 내림 + 강제 쿨타임 시작
			bShieldOnForcedCooldown = true;
			ShieldForcedCooldownTimer = ShieldOverchargeForcedCooldown;
			bIsShieldActive = false;

			if (ShieldActor)
			{
				ShieldActor->SetShieldActive(false);
			}
			GetCharacterMovement()->MaxWalkSpeed = DefaultMaxWalkSpeed;

			// 방패 깨짐 소리 (비어 있으면 일반 내림 소리)
			if (USoundBase* BreakSnd = ShieldBreakSound ? ShieldBreakSound.Get() : ShieldDownSound.Get())
			{
				UGameplayStatics::PlaySoundAtLocation(this, BreakSnd, GetActorLocation());
			}
		}
	}
	else if (bShieldOnForcedCooldown)
	{
		// 강제 쿨타임 중엔 재사용 불가 (StartShield에서 체크)
		ShieldForcedCooldownTimer -= DeltaTime;
		if (ShieldForcedCooldownTimer <= 0.f)
		{
			bShieldOnForcedCooldown = false;
		}
		// 쿨타임 중에도 게이지는 회복 (HeatComponent와 동일 원칙 — 재사용 시점만 강제로 늦춤)
		CurrentShieldStamina = FMath::Min(MaxShieldStamina, CurrentShieldStamina + ShieldRecoveryRate * DeltaTime);
	}
	else
	{
		// 평상시 — 안 쓰는 동안 자연 회복
		CurrentShieldStamina = FMath::Min(MaxShieldStamina, CurrentShieldStamina + ShieldRecoveryRate * DeltaTime);
	}
}

void ALumenCharacter::StartShield(const FInputActionValue& Value)
{
	// 강제 쿨타임 중이거나 게이지가 없으면 못 듦
	if (bShieldOnForcedCooldown || CurrentShieldStamina <= 0.f)
	{
		return;
	}

	// 방패 든 동안 사격 차단 — 자동연사 중이었다면 정지
	GetWorldTimerManager().ClearTimer(AutoFireTimerHandle);

	bIsShieldActive = true;

	if (ShieldActor)
	{
		ShieldActor->SetShieldActive(true);
	}
	if (ShieldUpSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ShieldUpSound, GetActorLocation());
	}
	AimLineComp->SetVisible(false);

	GetCharacterMovement()->MaxWalkSpeed = DefaultMaxWalkSpeed * ShieldMoveSpeedMultiplier; // 방패 든 채 이동 허용(감속)
}

void ALumenCharacter::StopShield(const FInputActionValue& Value)
{
	// 내림 소리는 실제로 들고 있었을 때만 (게이지 소진으로 이미 내려간 뒤 버튼을 떼는 경우 제외)
	if (bIsShieldActive && ShieldDownSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ShieldDownSound, GetActorLocation());
	}

	bIsShieldActive = false;

	if (ShieldActor)
	{
		ShieldActor->SetShieldActive(false);
	}
	AimLineComp->SetVisible(true);

	GetCharacterMovement()->MaxWalkSpeed = DefaultMaxWalkSpeed;
}

void ALumenCharacter::Dash(const FInputActionValue& Value)
{
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

	bIsDashing = true;
	GetCharacterMovement()->Velocity = FVector::ZeroVector; // 대쉬 진입 시 관성 제거

	if (DashSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, DashSound, GetActorLocation());
	}

	// 구르기 애니메이션이 앞으로 구르므로 몸을 구르는 방향으로 즉시 돌림
	SetActorRotation(FRotator(0.f, LastMoveDirection.Rotation().Yaw, 0.f));

	// 구르기 몽타주 — DashDuration에 맞춰 배속 재생
	if (DashMontage)
	{
		const float Length = DashMontage->GetPlayLength();
		const float Rate = (DashDuration > 0.f && Length > 0.f) ? Length / DashDuration : 1.f;
		PlayAnimMontage(DashMontage, Rate);
	}

	if (HealthComponent)
	{
		// 무적 = 콜리전은 유지하고 데미지만 무시 (콜리전을 끄면 발사체가 뚫고 지나감)
		HealthComponent->SetInvulnerable(true);

		// 무적은 앞부분만 (DashInvulnerableDuration) — 구르기 전체가 아님
		const float InvulTime = FMath::Min(DashInvulnerableDuration, DashDuration);
		GetWorldTimerManager().SetTimer(DashInvulnerableTimerHandle, [this]()
			{
				if (HealthComponent) HealthComponent->SetInvulnerable(false);
			}, FMath::Max(InvulTime, KINDA_SMALL_NUMBER), false);
	}

	GetWorldTimerManager().SetTimer(DashDurationTimerHandle, this, &ALumenCharacter::EndDash, DashDuration, false);
}

void ALumenCharacter::EndDash()
{
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
	// TODO: 색 코어 보유 여부 확인 후에만 전환 허용 (지금은 무조건 전환)
	if (NewColor != CurrentColor && ColorSwitchSound)   // 같은 색 다시 누르면 무음
	{
		UGameplayStatics::PlaySound2D(this, ColorSwitchSound);
	}
	CurrentColor = NewColor;
	AimLineComp->SetWeaponData(GetCurrentWeaponData());
	UpdateWeaponMesh();   // 손에 든 총 교체
}

void ALumenCharacter::OnPauseInput(const FInputActionValue& Value)
{
	TogglePause();
}

AActor* ALumenCharacter::FindBestInteractable()
{
	// 범위 안의 상호작용 대상(문/회전 거울/리셋 오브젝트 등) 중 지금 쓸 수 있고 가장 가까운 것
	OverlappingInteractables.RemoveAll([](const TWeakObjectPtr<AActor>& W) { return !W.IsValid(); });

	AActor* Best = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();
	for (const TWeakObjectPtr<AActor>& W : OverlappingInteractables)
	{
		ILumenInteractable* I = Cast<ILumenInteractable>(W.Get());
		if (!I || !I->CanInteract()) continue;

		const float DistSq = FVector::DistSquared(W->GetActorLocation(), GetActorLocation());
		if (DistSq < BestDistSq) { BestDistSq = DistSq; Best = W.Get(); }
	}
	return Best;
}

void ALumenCharacter::Interact(const FInputActionValue& Value)
{
	// 프롬프트가 떠 있는 대상 = 여기서 고르는 대상 (같은 함수 사용)
	if (AActor* Best = FindBestInteractable())
	{
		HeldInteractable = Best; // 뗄 때 같은 대상에 InteractReleased를 보내기 위해 기억
		Cast<ILumenInteractable>(Best)->Interact(this);
	}
}

void ALumenCharacter::UpdateInteractPrompt()
{
	if (!InteractPromptComp) return;

	AActor* Best = FindBestInteractable();
	ILumenInteractable* I = Best ? Cast<ILumenInteractable>(Best) : nullptr;

	if (!I)
	{
		if (InteractPromptComp->IsVisible())
		{
			InteractPromptComp->SetVisibility(false);
		}
		return;
	}

	InteractPromptComp->SetWorldLocation(I->GetPromptLocation());

	// 문은 열림/닫힘에 따라 문구가 바뀌므로 매 틱 확인하되, 바뀐 경우에만 SetText
	UUserWidget* RawWidget = InteractPromptComp->GetUserWidgetObject();
	ULumenInteractPromptWidget* Widget = Cast<ULumenInteractPromptWidget>(RawWidget);

	if (RawWidget && !Widget)
	{
		// WBP의 부모 클래스가 LumenInteractPromptWidget이 아니면 문구를 바꿀 수 없음 → 디자이너 기본 문구만 보임
		static bool bWarned = false;
		if (!bWarned)
		{
			UE_LOG(LogTemp, Warning, TEXT("[InteractPrompt] %s 의 부모 클래스가 LumenInteractPromptWidget이 아님 — 프롬프트 문구가 바뀌지 않음"),
				*RawWidget->GetClass()->GetName());
			bWarned = true;
		}
	}

	const FText NewText = I->GetInteractPrompt();
	// 위젯 인스턴스가 새로 만들어진 경우에도(컴포넌트 재등록 등) 다시 문구를 넣도록 위젯 포인터도 비교
	if (Widget && (Widget != LastPromptWidget.Get() || !NewText.EqualTo(LastPromptText)))
	{
		Widget->SetPromptText(NewText);
		LastPromptText = NewText;
		LastPromptWidget = Widget;
	}

	if (!InteractPromptComp->IsVisible())
	{
		InteractPromptComp->SetVisibility(true);
	}
}

void ALumenCharacter::HandlePlayerDeath()
{
	// 사망 연출: 쓰러지는 모션 → 화면 서서히 어두워짐 → DeathRespawnDelay 뒤 부활
	// (HP가 적어 금방 죽으므로 "무엇에 맞아 죽었는지" 볼 시간을 줌. 몽타주가 없으면 즉시 부활)
	if (bIsDying)
	{
		return;
	}
	if (!DeathMontage || DeathRespawnDelay <= 0.f)
	{
		RespawnAtCheckpoint();
		return;
	}

	bIsDying = true;

	// 누르고 있던 사격/방패/구르기 정리 — 부활 후 혼자 계속 쏘는 것 방지 (일시정지와 같은 이유)
	StopFire(FInputActionValue());
	StopShield(FInputActionValue());
	if (bIsDashing)
	{
		GetWorldTimerManager().ClearTimer(DashDurationTimerHandle);
		EndDash();
	}
	AimLineComp->SetVisible(false);
	GetCharacterMovement()->StopMovementImmediately();

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		DisableInput(PC);
		if (PC->PlayerCameraManager)
		{
			// 사망 모션 동안 서서히 검게 → 부활 시 다시 밝아짐 (RespawnAtCheckpoint)
			PC->PlayerCameraManager->StartCameraFade(0.f, 1.f, DeathRespawnDelay, FLinearColor::Black, false, true);
		}
	}

	PlayAnimMontage(DeathMontage);
	GetWorldTimerManager().SetTimer(DeathRespawnTimerHandle, this, &ALumenCharacter::RespawnAtCheckpoint,
		DeathRespawnDelay, false);
}

void ALumenCharacter::RespawnAtCheckpoint()
{
	GetWorldTimerManager().ClearTimer(DeathRespawnTimerHandle);

	// 사망 연출 종료 — 모션 정지, 입력/조준선 복구, 화면 다시 밝게
	// (아래 PlayerState 확인에서 일찍 return해도 입력이 잠긴 채 남지 않도록 맨 앞에서 처리)
	if (bIsDying)
	{
		bIsDying = false;
		StopAnimMontage(DeathMontage);
		AimLineComp->SetVisible(true);

		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			EnableInput(PC);
			if (PC->PlayerCameraManager)
			{
				PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, 0.4f, FLinearColor::Black);
			}
		}
	}

	// 텔레포트 순간 방 트리거 박스를 나가는 것으로 처리되어 EndOverlap → ClearCurrentRoomIfMatches()가
	// 동기적으로 발생해 CurrentRoom이 지워짐 — 그래서 텔레포트 전에 RoomToReset으로 복사해 둠
	ARoomVolume* RoomToReset = CurrentRoom;
	UE_LOG(LogTemp, Log, TEXT("[Death] 사망 — 리셋할 방: %s"), RoomToReset ? *RoomToReset->GetName() : TEXT("없음"));

	// APawn::GetPlayerState()는 템플릿이 아니라 APlayerState*를 반환 — Cast로 다운캐스팅 필요
	ALumenPlayerState* PS = Cast<ALumenPlayerState>(GetPlayerState());
	if (!PS)
	{
		return;
	}

	FTransform RespawnTransform = PS->LastCheckpointTransform;

	// 체크포인트를 아직 안 밟았으면 PlayerStart, 그것도 없으면 제자리
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

	// SetActorTransform 대신 위치/회전만 적용 (체크포인트 Scale 전이 방지)
	SetActorLocationAndRotation(RespawnTransform.GetLocation(), RespawnTransform.GetRotation());

	// 텔레포트로 CurrentRoom이 지워졌을 수 있으므로 미리 복사해 둔 RoomToReset 사용
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
	CurrentShieldStamina = MaxShieldStamina;
	bShieldOnForcedCooldown = false;
}

void ALumenCharacter::SetCurrentRoom(ARoomVolume* Room)
{
	CurrentRoom = Room;
	ApplyRoomRangeMultiplier();
}

void ALumenCharacter::ApplyRoomRangeMultiplier()
{
	const float NewMultiplier = CurrentRoom ? CurrentRoom->GetPlayerRangeMultiplier() : 1.f;
	if (FMath::IsNearlyEqual(NewMultiplier, RangeMultiplier))
	{
		return;
	}
	RangeMultiplier = NewMultiplier;
	if (AimLineComp)
	{
		AimLineComp->SetRangeMultiplier(RangeMultiplier); // 조준선 길이 = 실제 사거리 유지
	}
}

void ALumenCharacter::ClearCurrentRoomIfMatches(ARoomVolume* Room)
{
	if (CurrentRoom == Room)
	{
		CurrentRoom = nullptr;
		ApplyRoomRangeMultiplier();
		// 낙사 대비 — 방 박스 아래로 떨어지면 Kill Z에 닿기 전에 이미 방을 "나간" 것으로 처리되므로
		// 방금 나간 방을 잠깐 기억해 둠 (HandleFellOutOfWorld에서 사용)
		LastExitedRoom = Room;
		LastRoomExitTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	}
}

void ALumenCharacter::HandleFellOutOfWorld()
{
	// 낙사는 Kill Z에 닿기 전에 방 트리거 박스 아래로 빠져나가면서 CurrentRoom이 이미 지워진 상태.
	// 방금(FallResetGraceTime 이내) 나간 방이 있으면 그 방에서 떨어진 것으로 보고 리셋 대상으로 복원
	if (!CurrentRoom && LastExitedRoom.IsValid()
		&& GetWorld()->GetTimeSeconds() - LastRoomExitTime <= FallResetGraceTime)
	{
		CurrentRoom = LastExitedRoom.Get();
	}
	LastExitedRoom.Reset();

	// Kill Z는 데미지 판정을 건너뛰고 곧장 부활 — 낙사는 사망 모션 없이 즉시
	RespawnAtCheckpoint(); // 체력·발열·방패 게이지 리셋 포함
}

void ALumenCharacter::UpdateCurrentTarget()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 조준선/발사와 같은 방향·시작점(총구) — 하트 표시 대상이 조준선 위의 적과 일치하도록
	const FVector Direction = GetFireDirection();
	const FVector Start = GetMuzzleLocation();
	const FVector End = Start + Direction * TargetingTraceRange;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(TargetingTrace), false, this);
	if (ShieldActor) Params.AddIgnoredActor(ShieldActor);   // 방패 든 상태에서도 내 방패에 막히지 않게

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

void ALumenCharacter::EnterRoomView(AActor* Room, bool bOverridePitch, float Pitch,
	float ArmLength, bool bMask, const FBox& Bounds)
{
	ActiveViewRoom = Room;

	const float NewPitch = bOverridePitch ? Pitch : DefaultBoomRotation.Pitch;
	TargetBoomRotation = FRotator(NewPitch, DefaultBoomRotation.Yaw, 0.f);
	DesiredArmLength = (ArmLength > 0.f) ? ArmLength : DefaultArmLength;

	SetRoomMask(bMask, Bounds);
}

void ALumenCharacter::ExitRoomView(AActor* Room)
{
	// 이미 다른 방이 카메라를 차지했다면 무시 (인접 방의 Enter/Exit 순서 꼬임 방지)
	if (ActiveViewRoom.Get() != Room) return;

	ActiveViewRoom = nullptr;
	TargetBoomRotation = DefaultBoomRotation;
	DesiredArmLength = DefaultArmLength;
	SetRoomMask(false, FBox(ForceInit));
}

void ALumenCharacter::UpdateCameraBlend(float DeltaTime)
{
	if (!CameraBoom) return;

	const FRotator Current = CameraBoom->GetRelativeRotation();
	if (!Current.Equals(TargetBoomRotation, 0.01f))
	{
		CameraBoom->SetRelativeRotation(
			FMath::RInterpTo(Current, TargetBoomRotation, DeltaTime, CameraBlendSpeed));
	}

	CameraBoom->TargetArmLength =
		FMath::FInterpTo(CameraBoom->TargetArmLength, DesiredArmLength, DeltaTime, CameraBlendSpeed);

	RoomMaskAlpha = FMath::FInterpTo(RoomMaskAlpha, TargetRoomMaskAlpha, DeltaTime, CameraBlendSpeed);
	if (WallFadeMPC)
	{
		UKismetMaterialLibrary::SetScalarParameterValue(this, WallFadeMPC,
			TEXT("RoomMaskAlpha"), RoomMaskAlpha);
	}
}

void ALumenCharacter::SetRoomMask(bool bEnable, const FBox& RoomBounds)
{
	if (bEnable && WallFadeMPC)
	{
		// 범위는 켤 때만 갱신 → 끌 때는 같은 방 기준으로 서서히 사라짐
		UKismetMaterialLibrary::SetVectorParameterValue(this, WallFadeMPC,
			TEXT("RoomBoundsMin"), FLinearColor(RoomBounds.Min));
		UKismetMaterialLibrary::SetVectorParameterValue(this, WallFadeMPC,
			TEXT("RoomBoundsMax"), FLinearColor(RoomBounds.Max));
	}
	TargetRoomMaskAlpha = bEnable ? 1.f : 0.f;
}

void ALumenCharacter::TogglePause()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	const bool bPause = !UGameplayStatics::IsGamePaused(this);

	if (bPause)
	{
		// 멈추기 전에 사격/방패를 꺼둠 (버튼 떼는 입력을 놓쳐서 계속 쏘는 것 방지)
		StopFire(FInputActionValue());
		StopShield(FInputActionValue());
		// E 누른 채 일시정지 → 재개 후 엉뚱한 회전 방지
		if (ILumenInteractable* Held = Cast<ILumenInteractable>(HeldInteractable.Get()))
		{
			Held->InteractCanceled();
		}
		HeldInteractable.Reset();

		if (!PauseMenuWidget && PauseMenuWidgetClass)
		{
			PauseMenuWidget = CreateWidget<UUserWidget>(PC, PauseMenuWidgetClass);
		}
		if (PauseMenuWidget)
		{
			PauseMenuWidget->AddToViewport(10);   // HUD보다 위에 표시
		}

		UGameplayStatics::SetGamePaused(this, true);

		FInputModeGameAndUI Mode;   // UI 클릭 + P키 둘 다 받기
		if (PauseMenuWidget) Mode.SetWidgetToFocus(PauseMenuWidget->TakeWidget());
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Mode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(Mode);
		PC->bShowMouseCursor = true;
	}
	else
	{
		if (PauseMenuWidget && !PauseMenuWidget->IsVisible()) return;
		if (PauseMenuWidget) PauseMenuWidget->RemoveFromParent();

		UGameplayStatics::SetGamePaused(this, false);

		// SetConsumeCaptureMouseDown(false): 재개 후 첫 클릭이 뷰포트 캡처에 소모되지 않게
		FInputModeGameOnly Mode;
		Mode.SetConsumeCaptureMouseDown(false);
		PC->SetInputMode(Mode);
	}
}

void ALumenCharacter::QuitToTitle()
{
	UGameplayStatics::SetGamePaused(this, false);

	// 로드 대기 플래그 초기화 — 이전 이어하기 요청이 타이틀 이후까지 남지 않게
	if (ULumenGameInstance* GI = GetGameInstance<ULumenGameInstance>())
	{
		GI->ClearPendingLoad();
	}

	UGameplayStatics::OpenLevel(this, FName("L_Title"));
}

void ALumenCharacter::AddInteractable(AActor* Actor)
{
	OverlappingInteractables.AddUnique(Actor);
}

void ALumenCharacter::RemoveInteractable(AActor* Actor)
{
	OverlappingInteractables.Remove(Actor);
}

void ALumenCharacter::InteractReleased(const FInputActionValue& Value)
{
	// 누를 때 고른 대상에게만 전달 (문/리셋은 아무것도 안 함, 거울은 "짧게 누름 = 정방향" 판정)
	if (ILumenInteractable* Held = Cast<ILumenInteractable>(HeldInteractable.Get()))
	{
		Held->InteractReleased(this);
	}
	HeldInteractable.Reset();
}

float ALumenCharacter::GetDashReadyRatio() const
{
	if (bIsDashing) return 0.f;
	if (!bDashOnCooldown || DashCooldown <= 0.f) return 1.f;

	const float Remaining = GetWorldTimerManager().GetTimerRemaining(DashCooldownTimerHandle);
	if (Remaining < 0.f) return 1.f;   // 타이머 없음
	return FMath::Clamp(1.f - Remaining / DashCooldown, 0.f, 1.f);
}

FLinearColor ALumenCharacter::GetActiveWeaponColor() const
{
	const UWeaponDataAsset* Weapon = GetCurrentWeaponData();
	return Weapon ? Weapon->WeaponColor_TEMP : FLinearColor::White;
}

// 발소리 — Anim Notify가 아닌 타이머 방식
void ALumenCharacter::UpdateFootsteps(float DeltaTime)
{
	const UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	const bool bMoving = MoveComp && MoveComp->IsMovingOnGround()
		&& GetVelocity().Size2D() > 10.f && !bIsDashing;

	if (!bMoving)
	{
		FootstepTimer = 0.f;   // 멈추면 리셋 → 다시 움직일 때 첫 발이 바로 들림
		return;
	}

	FootstepTimer -= DeltaTime;
	if (FootstepTimer > 0.f)
	{
		return;
	}

	const bool bWalking = bIsShieldActive;   // 방패 든 감속 이동 = 걷기
	if (USoundBase* Step = bWalking ? FootstepWalkSound : FootstepRunSound)
	{
		const FVector Feet = GetActorLocation()
			- FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
		UGameplayStatics::PlaySoundAtLocation(this, Step, Feet);
	}
	FootstepTimer = bWalking ? WalkStepInterval : RunStepInterval;
}

// 총 메시의 Muzzle 소켓 → (없으면) 캐릭터 메시의 Muzzle 소켓 → (없으면) 기존 방식
FVector ALumenCharacter::GetMuzzleLocation() const
{
	if (WeaponMeshComp && WeaponMeshComp->GetStaticMesh() && WeaponMeshComp->DoesSocketExist(MuzzleSocketName))
	{
		return WeaponMeshComp->GetSocketLocation(MuzzleSocketName);
	}
	if (GetMesh() && GetMesh()->DoesSocketExist(MuzzleSocketName))
	{
		return GetMesh()->GetSocketLocation(MuzzleSocketName);
	}
	return GetActorLocation() + FVector(CurrentAimDirection, 0.f).GetSafeNormal() * 100.f;
}

// 몸 방향(CurrentAimDirection)은 캐릭터 회전용으로 그대로 두고,
// 발사 방향만 총구에서 커서 지점을 향하게 보정 — 총구가 오른손에 치우쳐 있어도 커서에 정확히 맞음
FVector ALumenCharacter::GetFireDirection() const
{
	const FVector BodyDir = FVector(CurrentAimDirection, 0.f).GetSafeNormal();

	if (bIsUsingGamepadAim || !bHasCursorAimPoint)
	{
		return BodyDir;
	}

	FVector ToCursor = CursorAimPoint - GetMuzzleLocation();
	ToCursor.Z = 0.f;

	// 커서가 총구에 너무 가깝거나 총구 뒤쪽이면 몸 방향 사용
	if (ToCursor.SizeSquared() < FMath::Square(MinCursorAimDistance) ||
		FVector::DotProduct(ToCursor, BodyDir) <= 0.f)
	{
		return BodyDir;
	}
	return ToCursor.GetSafeNormal();
}

// 메시 없는 DA면 빈 손(nullptr), 오프셋은 DA의 WeaponMeshOffset
void ALumenCharacter::UpdateWeaponMesh()
{
	if (!WeaponMeshComp)
	{
		return;
	}

	const UWeaponDataAsset* Weapon = GetCurrentWeaponData();
	WeaponMeshComp->SetStaticMesh(Weapon ? Weapon->WeaponMesh.Get() : nullptr);
	WeaponMeshComp->SetRelativeTransform(Weapon ? Weapon->WeaponMeshOffset : FTransform::Identity);
}
