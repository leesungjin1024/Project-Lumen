// LumenCharacter.cpp

#include "LumenCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

ALumenCharacter::ALumenCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// ===== 카메라 (4-1: 고정 아이소메트릭) =====
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = CameraArmLength;
	CameraBoom->SetRelativeRotation(FRotator(CameraPitch, CameraYaw, 0.f));

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
}

void ALumenCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
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
		EnhancedInput->BindAction(DashAction, ETriggerEvent::Triggered, this, &ALumenCharacter::Dash);
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

	// 최종 조준 방향으로 캐릭터를 회전 (4-2: 이동 방향이 아니라 조준 방향을 보고 있어야 함)
	// FVector2D는 Rotation() 멤버가 없으므로 atan2로 직접 Yaw 각도를 구함
	const float TargetYaw = FMath::RadiansToDegrees(FMath::Atan2(CurrentAimDirection.Y, CurrentAimDirection.X));
	const FRotator TargetRotation = FRotator(0.f, TargetYaw, 0.f);
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, 15.f));
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
	const FVector2D MoveInput = Value.Get<FVector2D>();

	// 카메라가 고정 45도 회전이라, 월드 축 그대로 넣으면 화면상 이동 방향과 어긋날 수 있음
	// 4-1에서 카메라 각도를 실험적으로 확정할 때 이 부분도 같이 체감 확인 필요
	AddMovementInput(FVector::ForwardVector, MoveInput.Y);
	AddMovementInput(FVector::RightVector, MoveInput.X);
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

// ===== 아래는 스텁 — 실제 로직은 각 참고 섹션 구현 시점(6-1)에 채움 =====

void ALumenCharacter::StartFire(const FInputActionValue& Value)
{
	// TODO(2주차, 5-1/5-2): 발사 시작 + 발열 누적 시작
	UE_LOG(LogTemp, Log, TEXT("StartFire (color=%d)"), static_cast<uint8>(CurrentColor));
}

void ALumenCharacter::StopFire(const FInputActionValue& Value)
{
	// TODO(2주차, 5-1-1): 발사 중단 -> 발열 냉각 시작
	UE_LOG(LogTemp, Log, TEXT("StopFire"));
}

void ALumenCharacter::StartShield(const FInputActionValue& Value)
{
	// TODO(3주차, 5-4/5-4-1): 방패 들기 시작, 게이지 소모 시작
	UE_LOG(LogTemp, Log, TEXT("StartShield"));
}

void ALumenCharacter::StopShield(const FInputActionValue& Value)
{
	// TODO(3주차, 5-4-1): 방패 내리기, 게이지 회복 시작
	UE_LOG(LogTemp, Log, TEXT("StopShield"));
}

void ALumenCharacter::Dash(const FInputActionValue& Value)
{
	// TODO(3주차, 4-2/5-8): 대쉬 구현 (우선순위 가장 낮은 항목, 6-1 리스크 참고)
	UE_LOG(LogTemp, Log, TEXT("Dash"));
}

void ALumenCharacter::SelectRed(const FInputActionValue& Value) { SelectColor(ELumenWeaponColor::Red); }
void ALumenCharacter::SelectBlue(const FInputActionValue& Value) { SelectColor(ELumenWeaponColor::Blue); }
void ALumenCharacter::SelectGreen(const FInputActionValue& Value) { SelectColor(ELumenWeaponColor::Green); }

void ALumenCharacter::SelectColor(ELumenWeaponColor NewColor)
{
	// TODO(3주차, 5-7): 색 코어 보유 여부 확인 후에만 전환 허용 (지금은 무조건 전환)
	CurrentColor = NewColor;
	UE_LOG(LogTemp, Log, TEXT("SelectColor: %d"), static_cast<uint8>(CurrentColor));
}

void ALumenCharacter::Interact(const FInputActionValue& Value)
{
	// TODO: 상호작용 대상 정의 필요 (문서에 아직 없음 - 필요 시 5-6 퍼즐 오브젝트와 연동)
	UE_LOG(LogTemp, Log, TEXT("Interact"));
}

void ALumenCharacter::TogglePause(const FInputActionValue& Value)
{
	// TODO(4주차, 11-1): 일시정지 위젯 생성/제거
	UE_LOG(LogTemp, Log, TEXT("TogglePause"));
}