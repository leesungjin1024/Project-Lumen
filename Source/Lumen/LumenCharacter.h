
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "HealthComponent.h"
#include "LumenCharacter.generated.h"


class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class AProjectile;
class AMirrorActor;
class UHeatComponent;
class UWeaponDataAsset;
class AShieldActor;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UHeatComponent> HeatComponent;

	// 색 -> 무기 데이터 매핑 (에디터에서 3개 DA_Weapon_*_TEMP 할당)
	UPROPERTY(EditAnywhere, Category = "Weapon")
	TMap<ELumenWeaponColor, UWeaponDataAsset*> WeaponDataByColor;

	// 5-4-1: 방패 든 동안 사격 차단용 — 방패 구현 단계에서 실제로 세팅됨
	UPROPERTY(BlueprintReadOnly, Category = "Shield")
	bool bIsShieldActive = false;

	//체력
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	UHealthComponent* HealthComponent;

	UPROPERTY(EditAnywhere, Category = "Shield")
	TSubclassOf<AShieldActor> ShieldActorClass; // 에디터에서 BP_Shield 할당

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shield")
	TObjectPtr<AShieldActor> ShieldActor;

	// 방패 스태미너 게이지 (5-4-1: 홀드 중 소모, 미사용 시 회복, 완전 소진 시 강제 쿨타임)
	UPROPERTY(EditDefaultsOnly, Category = "Shield")
	float MaxShieldStamina = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Shield")
	float ShieldDrainRate = 25.f; // 초당 소모량 (가안)

	UPROPERTY(EditDefaultsOnly, Category = "Shield")
	float ShieldRecoveryRate = 20.f; // 초당 회복량 (가안)

	UPROPERTY(EditDefaultsOnly, Category = "Shield")
	float ShieldOverchargeForcedCooldown = 1.5f; // 완전 소진 시 강제 쿨타임 (1~2초 범위, 가안)

	UPROPERTY(EditDefaultsOnly, Category = "Shield")
	float ShieldMoveSpeedMultiplier = 0.5f; // 5-2-1: 방패 사용 중 이동 허용(감속)


private:
	FTimerHandle AutoFireTimerHandle;

	UWeaponDataAsset* GetCurrentWeaponData() const;
	void FireOnce(UWeaponDataAsset* Weapon);          // 발열 체크 + 실제 발사 진입점
	void FireProjectileSet(UWeaponDataAsset* Weapon); // 빨강(1발)/파랑(펠릿 다발) 공용
	void FireLineTrace(UWeaponDataAsset* Weapon);     // 초록 — TODO(3주차 후반, 관통 로직은 보류)

	//쉴드
	float CurrentShieldStamina = 0.f;
	bool bShieldOnForcedCooldown = false;
	float ShieldForcedCooldownTimer = 0.f;
	float DefaultMaxWalkSpeed = 0.f; // 방패 해제 시 되돌릴 원래 속도

	void UpdateShieldStamina(float DeltaTime); // Tick에서 호출
};
