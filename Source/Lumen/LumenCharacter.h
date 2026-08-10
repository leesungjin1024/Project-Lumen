// LumenCharacter.h
// 4-1(고정 아이소메트릭 카메라) + 4-2/4-2-1(Enhanced Input) 1주차 범위 구현
//
// 주의: 아래 LUMEN_API 매크로는 플레이스홀더입니다.
// 본인 프로젝트가 UE 템플릿으로 이미 만들어둔 캐릭터 클래스(예: ProjectLumenCharacter.h)를 열어보면
// 맨 위에 있는 실제 매크로 이름(예: PROJECTLUMEN_API)을 확인할 수 있습니다. 그 이름으로 바꿔서 쓰세요.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "LumenCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class AProjectile;
class AMirrorActor;

/**
 * 색 전환에 쓰이는 색상 enum (5-7 참고)
 * 실제 무기 데이터(5-1)는 아직 구현 전이라, 지금은 "어떤 색을 선택했는가" 상태만 들고 있음
 */
UENUM(BlueprintType)
enum class ELumenWeaponColor : uint8
{
	Red,
	Blue,
	Green
};

UCLASS()
class LUMEN_API ALumenCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ALumenCharacter();
	void SetInteractableMirror(AMirrorActor* Mirror);
	void ClearInteractableMirror(AMirrorActor* Mirror);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// ===== 카메라 (4-1: 고정 아이소메트릭) =====

	/** 캐릭터를 따라다니되 회전은 캐릭터 영향을 받지 않는 붐 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> TopDownCamera;

	/** 4-1 "프로토타입 확인 필요 사항" - 그레이박스에서 실험적으로 조정할 값들 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "-89", ClampMax = "-10"))
	float CameraPitch = -55.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float CameraYaw = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "300"))
	float CameraArmLength = 1200.f;

	// ===== Enhanced Input (4-2-1) =====

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	/** 게임패드 오른쪽 스틱 전용 (마우스 조준은 IA 바인딩이 아니라 Tick에서 커서 위치로 계산, INPUT_SETUP.md 4단계 참고) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> AimAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> ShieldAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> DashAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> SelectRedAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> SelectBlueAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> SelectGreenAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> PauseAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TSubclassOf<AProjectile> ProjectileClass;

	// ===== 입력 핸들러 =====

	void Move(const FInputActionValue& Value);
	void AimGamepad(const FInputActionValue& Value);

	void StartFire(const FInputActionValue& Value);
	void StopFire(const FInputActionValue& Value);

	void StartShield(const FInputActionValue& Value);
	void StopShield(const FInputActionValue& Value);

	void Dash(const FInputActionValue& Value);

	void SelectRed(const FInputActionValue& Value);
	void SelectBlue(const FInputActionValue& Value);
	void SelectGreen(const FInputActionValue& Value);
	void SelectColor(ELumenWeaponColor NewColor);

	void Interact(const FInputActionValue& Value);
	void TogglePause(const FInputActionValue& Value);

	// ===== 조준 방향 (4-2) =====

	/** 마우스 커서든 게임패드든, 최종적으로 여기 저장된 방향으로 캐릭터가 회전함 */
	UPROPERTY(BlueprintReadOnly, Category = "Aim")
	FVector2D CurrentAimDirection = FVector2D(1.f, 0.f);

	/** 게임패드 스틱이 이 값보다 크게 움직였을 때만 "게임패드로 조준 중"으로 간주 (마우스와 충돌 방지) */
	UPROPERTY(EditAnywhere, Category = "Aim")
	float GamepadAimDeadZone = 0.2f;

	bool bIsUsingGamepadAim = false;
	FVector2D LastGamepadAimInput = FVector2D::ZeroVector;

	/** 마우스 커서가 가리키는 월드 좌표를 캐릭터 높이의 평면과 교차시켜 조준 방향을 구함 */
	void UpdateMouseAimDirection();

	//무기 빨간색
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	ELumenWeaponColor CurrentColor = ELumenWeaponColor::Red;

	//거울 회전
	UPROPERTY()
	TObjectPtr<AMirrorActor> CurrentInteractableMirror;
};
