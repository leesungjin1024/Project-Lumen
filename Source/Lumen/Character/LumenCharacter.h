// ALumenCharacter — 플레이어 캐릭터. 고정 아이소메트릭 카메라, 커서/패드 조준, 3색 무기 사격, 방패·대쉬, E 상호작용, 사망/낙사 부활.
// 연결: UHealthComponent, UHeatComponent, AShieldActor, ULumenAimLineComponent, ARoomVolume(카메라/리셋), ILumenInteractable, ALumenPlayerState(체크포인트)
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "HealthComponent.h"
#include "WeaponDataAsset.h"
#include "LumenHealthChunksWidget.h"
#include "Components/WidgetComponent.h"
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
class ULumenAimLineComponent;
class ALumenDoorActor;
class ARoomVolume;
class UWidgetComponent;
class ULumenPlayerGaugesWidget;
class USoundBase;
class UAnimMontage;
class UStaticMeshComponent;

UCLASS()
class LUMEN_API ALumenCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ALumenCharacter();

	// ===== 사망/방 연동 =====
	// HealthComponent::OnDeath(파라미터 없는 delegate)에 바인딩. 사망 연출 후 RespawnAtCheckpoint
	UFUNCTION() void HandlePlayerDeath();
	// Kill Z(ALumenKillZVolume)에서 호출 — 사망 연출 없이 즉시 부활
	void HandleFellOutOfWorld();

	// 사망/낙사 시 리셋할 방 — 전투룸/퍼즐룸 진입 시 호출
	// (방의 사거리 배율도 함께 적용하므로 본문은 cpp)
	void SetCurrentRoom(ARoomVolume* Room);

	// 룸 진입/이탈 시 호출(카메라 각도·거리·룸 마스크 전환). ArmLength가 0 이하이면 기본 거리 사용
	void EnterRoomView(AActor* Room, bool bOverridePitch, float Pitch,
		float ArmLength, bool bMask, const FBox& Bounds);
	void ExitRoomView(AActor* Room);

	void SetRoomMask(bool bEnable, const FBox& RoomBounds);

	// ILumenInteractable 구현체(문/회전 거울/리셋 오브젝트)가 자기 InteractionZone 오버랩에서 호출
	void AddInteractable(AActor* Actor);
	void RemoveInteractable(AActor* Actor);

	// E 상호작용 프롬프트 위젯 — WBP_InteractPrompt 할당 (부모 클래스: LumenInteractPromptWidget)
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> InteractPromptWidgetClass;

	// 방 밖으로 나갈 때 호출 — 지금도 CurrentRoom이 그 방일 때만 지움
	// (이미 다른 방에 들어가 CurrentRoom이 바뀐 뒤 옛 방의 EndOverlap이 늦게 와서 새 방 참조를 지우는 것 방지)
	// 본문이 cpp에 있는 이유: TWeakObjectPtr<ARoomVolume> 대입에 완전한 타입이 필요
	void ClearCurrentRoomIfMatches(ARoomVolume* Room);

	// ===== UI =====
	// 화면 HUD 위젯 — WBP_PlayerHUD 할당
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> PlayerHUDWidgetClass;

	// 머리 위 하트 3칸 (ULumenHealthChunksWidget)
	UPROPERTY(VisibleAnywhere, Category = "UI")
	TObjectPtr<UWidgetComponent> HealthChunksComp;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<ULumenHealthChunksWidget> HealthChunksWidgetClass;

	// 캐릭터 주변 게이지 (방패 스태미너/대쉬/발열)
	UPROPERTY(VisibleAnywhere, Category = "UI")
	TObjectPtr<UWidgetComponent> GaugesComp;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<ULumenPlayerGaugesWidget> GaugesWidgetClass;

	// 게이지 위젯이 읽는 값 (private 멤버를 밖으로 노출)
	float GetShieldRatio() const { return MaxShieldStamina > 0.f ? CurrentShieldStamina / MaxShieldStamina : 0.f; }
	// 애니메이션 블루프린트에서 방패 상체 자세 전환용으로 읽음
	UFUNCTION(BlueprintPure, Category = "Shield")
	bool IsShieldActive() const { return bIsShieldActive; }
	bool IsShieldOnForcedCooldown() const { return bShieldOnForcedCooldown; }
	float GetDashReadyRatio() const;          // 0 = 막 씀, 1 = 사용 가능
	FLinearColor GetActiveWeaponColor() const;

	UFUNCTION(BlueprintCallable, Category = "Pause")
	void TogglePause();

	UFUNCTION(BlueprintCallable, Category = "Pause")
	void QuitToTitle();

	// 머리 위 하트 위젯의 높이 (캡슐 꼭대기 기준, cm)
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	float HeartHeightOffset = 30.f;

	// ===== 사운드 =====
	// 이 레벨의 평소 배경음 (퍼즐 곡) — 전투룸 전투 곡이 끝나면 이 곡으로 돌아옴
	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	TObjectPtr<USoundBase> DefaultLevelMusic;

	// 발소리 (Anim Notify가 아닌 타이머 방식)
	// 평소 이동 = 달리기, 방패 든 감속 이동 = 걷기
	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	TObjectPtr<USoundBase> FootstepRunSound;

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	TObjectPtr<USoundBase> FootstepWalkSound;

	UPROPERTY(EditDefaultsOnly, Category = "Audio", meta = (ClampMin = "0.05"))
	float RunStepInterval = 0.32f;

	UPROPERTY(EditDefaultsOnly, Category = "Audio", meta = (ClampMin = "0.05"))
	float WalkStepInterval = 0.5f;

	// ===== 사운드: 전투 동작음 (비워두면 무음) =====
	UPROPERTY(EditDefaultsOnly, Category = "Audio|Combat")
	TObjectPtr<USoundBase> DashSound;

	UPROPERTY(EditDefaultsOnly, Category = "Audio|Combat")
	TObjectPtr<USoundBase> ShieldUpSound;

	UPROPERTY(EditDefaultsOnly, Category = "Audio|Combat")
	TObjectPtr<USoundBase> ShieldDownSound;

	// 방패 게이지 완전 소진 → 강제로 내려갈 때 (ShieldDownSound 대신 재생)
	UPROPERTY(EditDefaultsOnly, Category = "Audio|Combat")
	TObjectPtr<USoundBase> ShieldBreakSound;

	UPROPERTY(EditDefaultsOnly, Category = "Audio|Combat")
	TObjectPtr<USoundBase> ColorSwitchSound;

	// 과열 중 방아쇠를 당겼을 때 "틱" 소리
	UPROPERTY(EditDefaultsOnly, Category = "Audio|Combat")
	TObjectPtr<USoundBase> DryFireSound;

	// 빨강 자동연사가 과열 중 계속 재시도해도 이 간격(초)보다 자주 울리지 않음
	UPROPERTY(EditDefaultsOnly, Category = "Audio|Combat", meta = (ClampMin = "0.0"))
	float DryFireInterval = 0.4f;

	double LastDryFireTime = -1000.0;

	// ===== 무기 메시 / 총구 =====
	// 손에 드는 총 — 메시는 현재 무기 DA의 WeaponMesh로 자동 교체
	UPROPERTY(VisibleAnywhere, Category = "Weapon")
	TObjectPtr<UStaticMeshComponent> WeaponMeshComp;

	// 스켈레톤 RightHand 본에 만든 소켓 이름
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName WeaponSocketName = TEXT("WeaponSocket");

	// 총 스태틱 메시(3종 각각)에 만든 소켓 이름 — 발사체/레이저/조준선이 여기서 시작
	// 총 메시에 없으면 캐릭터 메시의 같은 소켓, 그것도 없으면 캐릭터 중심 + 조준 방향 100cm
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName MuzzleSocketName = TEXT("Muzzle");

	// 현재 총의 총구 월드 위치
	FVector GetMuzzleLocation() const;

	// 커서가 총구에서 이 거리(cm)보다 가까우면 커서 방향 대신 몸 조준 방향 사용
	// (커서가 총구 바로 옆/뒤에 있을 때 방향이 홱 뒤집히는 것 방지)
	UPROPERTY(EditDefaultsOnly, Category = "Weapon", meta = (ClampMin = "0.0"))
	float MinCursorAimDistance = 80.f;

	// 실제 발사/조준선 방향 — 마우스: 총구 → 커서 지점, 게임패드: 몸 조준 방향
	FVector GetFireDirection() const;

private:
	// 마우스 커서가 가리키는 바닥 평면 지점 (UpdateMouseAimDirection에서 갱신)
	FVector CursorAimPoint = FVector::ZeroVector;
	bool bHasCursorAimPoint = false;

public:

	// ===== 애니메이션 =====
	// 사격 — 슬롯을 UpperBody로 지정한 몽타주 (이동 중에도 상체만 재생)
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TObjectPtr<UAnimMontage> FireMontage;

	// 구르기 — DashDuration에 맞춰 자동 배속 재생
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TObjectPtr<UAnimMontage> DashMontage;

	// 사망 — 재생 후 DeathRespawnDelay 뒤 체크포인트에서 부활. 비우면 즉시 부활
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TObjectPtr<UAnimMontage> DeathMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Animation", meta = (ClampMin = "0.0"))
	float DeathRespawnDelay = 1.2f;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// ===== 카메라 (고정 아이소메트릭) =====

	// 캐릭터를 따라다니되 회전은 캐릭터 영향을 받지 않는 붐
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> TopDownCamera;

	// 기본 카메라 각도/거리 — BeginPlay에서 CameraBoom에 적용 (방 진입 시 RoomVolume 설정으로 바뀜)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "-89", ClampMax = "-10"))
	float CameraPitch = -55.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float CameraYaw = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "300"))
	float CameraArmLength = 1200.f;

	// ===== Enhanced Input =====

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	// 게임패드 오른쪽 스틱 전용 (마우스 조준은 IA 바인딩이 아니라 Tick에서 커서 위치로 계산)
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

	// 빨강/파랑 무기가 스폰하는 발사체
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
	void InteractReleased(const FInputActionValue& Value);
	void OnPauseInput(const FInputActionValue& Value);

	// ===== 조준 방향 =====

	// 마우스 커서든 게임패드든, 최종적으로 여기 저장된 방향으로 캐릭터가 회전함
	UPROPERTY(BlueprintReadOnly, Category = "Aim")
	FVector2D CurrentAimDirection = FVector2D(1.f, 0.f);

	// 게임패드 스틱이 이 값보다 크게 움직였을 때만 "게임패드로 조준 중"으로 간주 (마우스와 충돌 방지)
	UPROPERTY(EditAnywhere, Category = "Aim")
	float GamepadAimDeadZone = 0.2f;

	bool bIsUsingGamepadAim = false;
	FVector2D LastGamepadAimInput = FVector2D::ZeroVector;

	// 마우스 레이를 캐릭터 높이의 수평 평면과 교차시켜 조준 방향/CursorAimPoint를 구함
	void UpdateMouseAimDirection();

	// ===== 컴포넌트 / 무기 =====
	// 조준선 (부채꼴 트레이스로 벽에서 잘림, 총구에서 시작)
	UPROPERTY(VisibleAnywhere)
	ULumenAimLineComponent* AimLineComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	UHealthComponent* HealthComponent;

	// 현재 무기 색 (시작은 빨강)
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	ELumenWeaponColor CurrentColor = ELumenWeaponColor::Red;

	// 발열(과열 시 발사 불가)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UHeatComponent> HeatComponent;

	// 색 -> 무기 데이터 매핑 (3개 DA_Weapon_*_TEMP 할당)
	UPROPERTY(EditAnywhere, Category = "Weapon")
	TMap<ELumenWeaponColor, UWeaponDataAsset*> WeaponDataByColor;

	// ===== 방패 =====
	// 방패를 든 동안 true — 사격/대쉬 차단, 이동 감속
	UPROPERTY(BlueprintReadOnly, Category = "Shield")
	bool bIsShieldActive = false;

	UPROPERTY(EditAnywhere, Category = "Shield")
	TSubclassOf<AShieldActor> ShieldActorClass; // BP_Shield 할당

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shield")
	TObjectPtr<AShieldActor> ShieldActor;

	// 방패 스태미너 게이지 (홀드 중 소모, 미사용 시 회복, 완전 소진 시 강제 쿨타임)
	UPROPERTY(EditDefaultsOnly, Category = "Shield")
	float MaxShieldStamina = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Shield")
	float ShieldDrainRate = 25.f; // 초당 소모량

	UPROPERTY(EditDefaultsOnly, Category = "Shield")
	float ShieldRecoveryRate = 20.f; // 초당 회복량 (강제 쿨타임 중에도 회복)

	UPROPERTY(EditDefaultsOnly, Category = "Shield")
	float ShieldOverchargeForcedCooldown = 1.5f; // 완전 소진 시 재사용 불가 시간(초)

	UPROPERTY(EditDefaultsOnly, Category = "Shield")
	float ShieldMoveSpeedMultiplier = 0.5f; // 방패 든 동안 이동 속도 배율

	// 벽 페이드/룸 마스크용 MPC (PlayerLocation, CameraLocation, RoomBoundsMin/Max, RoomMaskAlpha)
	UPROPERTY(EditDefaultsOnly, Category = "Lumen|VFX")
	TObjectPtr<UMaterialParameterCollection> WallFadeMPC;

private:
	// ===== 사격 =====
	FTimerHandle AutoFireTimerHandle;

	UWeaponDataAsset* GetCurrentWeaponData() const;
	void FireOnce(UWeaponDataAsset* Weapon);          // 발열 체크 + 실제 발사 진입점
	void FireProjectileSet(UWeaponDataAsset* Weapon); // 빨강(1발)/파랑(펠릿 다발) 공용
	void FireLineTrace(UWeaponDataAsset* Weapon);     // 초록 — 반사/관통 라인트레이스

	// ===== 방패 상태 =====
	float CurrentShieldStamina = 0.f;
	bool bShieldOnForcedCooldown = false;
	float ShieldForcedCooldownTimer = 0.f;
	float DefaultMaxWalkSpeed = 0.f; // 방패 해제 시 되돌릴 원래 속도

	void UpdateShieldStamina(float DeltaTime); // Tick에서 호출

	// ===== 대쉬(구르기) =====
	void UpdateDash(float DeltaTime);
	void StartDash();
	void EndDash();

	UPROPERTY(EditDefaultsOnly, Category = "Dash")
	float DashDistance = 600.f;

	// 구르기 전체 시간(초) — DashMontage가 이 시간에 맞춰 배속 재생. 무적 시간은 DashInvulnerableDuration
	UPROPERTY(EditDefaultsOnly, Category = "Dash")
	float DashDuration = 0.4f;

	UPROPERTY(EditDefaultsOnly, Category = "Dash")
	float DashCooldown = 1.6f;

	// 구르기 앞부분만 무적 — 일어나는 동안은 맞을 수 있음 (타이밍 맞춘 구르기만 회피)
	// 무적은 콜리전을 유지하고 데미지만 무시 (콜리전을 끄면 발사체가 뚫고 지나감)
	UPROPERTY(EditDefaultsOnly, Category = "Dash", meta = (ClampMin = "0.0"))
	float DashInvulnerableDuration = 0.2f;

	FTimerHandle DashInvulnerableTimerHandle;

	FVector LastMoveDirection = FVector::ForwardVector;
	bool bIsDashing = false;
	bool bDashOnCooldown = false;
	FTimerHandle DashDurationTimerHandle;
	FTimerHandle DashCooldownTimerHandle;

	// ===== 방 / 조준 대상 =====
	// 사망/낙사 시 ResetRoom할 방
	UPROPERTY()
	TObjectPtr<ARoomVolume> CurrentRoom;

	// 현재 방의 사거리 배율 (방 밖 = 1). 발사체/초록/조준선 모두 여기에 맞춤
	float RangeMultiplier = 1.f;
	void ApplyRoomRangeMultiplier();

	// 조준선에 걸린 적 — 이 적의 하트만 표시 (UHealthComponent::SetTargeted)
	UPROPERTY()
	TObjectPtr<AActor> CurrentTargetedActor = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float TargetingTraceRange = 1500.f;

	void UpdateCurrentTarget();

	// ===== 카메라 전환 / 룸 마스크 =====
	UPROPERTY(EditAnywhere, Category = "Camera|Puzzle")
	float CameraBlendSpeed = 4.f;   // 클수록 빠르게 전환

	FRotator DefaultBoomRotation;
	float DefaultArmLength = 0.f;
	FRotator TargetBoomRotation;
	float DesiredArmLength = 0.f;

	void UpdateCameraBlend(float DeltaTime);

	// 지금 카메라를 차지하고 있는 방 — 인접 방의 Enter/Exit 순서가 꼬여도 옛 방의 Exit가 새 방 카메라를 되돌리지 않게
	TWeakObjectPtr<AActor> ActiveViewRoom;

	// 방 범위 밖을 어둡게 하는 마스크 (MPC RoomMaskAlpha로 보간)
	float RoomMaskAlpha = 0.f;
	float TargetRoomMaskAlpha = 0.f;

	// ===== 일시정지 =====
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> PauseMenuWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> PauseMenuWidget;

	// ===== 상호작용 =====
	TArray<TWeakObjectPtr<AActor>> OverlappingInteractables;

	// E를 누른 순간 고른 대상 — 뗄 때 같은 대상에 InteractReleased 전달
	TWeakObjectPtr<AActor> HeldInteractable;

	AActor* FindBestInteractable();
	void UpdateInteractPrompt();

	UPROPERTY()
	TObjectPtr<UWidgetComponent> InteractPromptComp;

	FText LastPromptText;
	TWeakObjectPtr<UUserWidget> LastPromptWidget;

	// ===== 낙사 리셋 =====
	// 방을 나간 직후 FallResetGraceTime 안에 Kill Z에 닿으면 그 방에서 떨어진 것으로 보고 리셋
	TWeakObjectPtr<ARoomVolume> LastExitedRoom;
	double LastRoomExitTime = -1000.0;

	UPROPERTY(EditDefaultsOnly, Category = "Room")
	float FallResetGraceTime = 3.f;

	// ===== 발소리 =====
	float FootstepTimer = 0.f;
	void UpdateFootsteps(float DeltaTime);

	// ===== 사망 연출 =====
	bool bIsDying = false;
	FTimerHandle DeathRespawnTimerHandle;
	void RespawnAtCheckpoint();   // 사망 연출 해제 + 체크포인트 텔레포트 + 방/체력/발열/방패 리셋

	// 현재 무기 DA의 메시를 손에 붙임 (BeginPlay, 색 전환 시)
	void UpdateWeaponMesh();
};