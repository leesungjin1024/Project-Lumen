// ALumenEnemyCharacter — 고정 포탑형 적. 감지된 플레이어 쪽으로 회전하다 정면에 들어오면 텔레그래프 후 그 방향으로 발사.
// 적 1체 = 색상 1개: 인스턴스별 WeaponData만 바꿔 빨강/파랑/초록 3종을 이 클래스 하나로 구현 (무기/발열/체력은 플레이어와 공용 컴포넌트).
// 연결: ALumenEnemyAIController + BT 노드(BTService_LumenPerception, BTTask_LumenTelegraph/FireWeapon), UHeatComponent, UHealthComponent, ULumenAimLineComponent, ULumenCombatLibrary

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Components/WidgetComponent.h"

#include "LumenHealthChunksWidget.h"
#include "LumenEnemyCharacter.generated.h"

class UWeaponDataAsset;
class UHeatComponent;
class UHealthComponent;
class UNiagaraSystem;
class AProjectile;
class ULumenAimLineComponent;
class UAnimMontage;
class UStaticMeshComponent;

UCLASS()
class LUMEN_API ALumenEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ALumenEnemyCharacter();

	// ===== 무기 / 컴포넌트 =====
	// 이 적이 쓰는 무기(색상) 하나. 레벨 배치 시 인스턴스별로 지정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lumen|Weapon")
	TObjectPtr<UWeaponDataAsset> WeaponData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lumen|Combat")
	TObjectPtr<UHeatComponent> HeatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lumen|Combat")
	TObjectPtr<UHealthComponent> HealthComponent;

	// 텔레그래프(공격 예고선) 비주얼. 플레이어 조준선과 같은 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lumen|Telegraph")
	TObjectPtr<ULumenAimLineComponent> TelegraphComp;

	// 초록(라인트레이스) 반사 지점 스파크 VFX. 플레이어/발사체와 같은 에셋을 지정해 시각적 일관성 유지
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lumen|VFX")
	TObjectPtr<UNiagaraSystem> ReflectionSparkVFX;

	// 빨강/파랑 발사체 클래스. ALumenCharacter/ALightEmitterActor와 같은 클래스를 지정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lumen|Weapon")
	TSubclassOf<AProjectile> ProjectileClass;

	// 총구 소켓 이름. GetMuzzleLocation이 포탑 메시 → 손에 든 총 → 스켈레탈 메시 순으로 찾음
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lumen|Weapon")
	FName MuzzleSocketName = TEXT("Muzzle");

	// ===== 발사 =====
	// BTTask_LumenFireWeapon이 호출. WeaponData->FireMode에 따라 발사체/라인트레이스로 분기
	// 텔레그래프로 고정한 방향이 있으면 Direction 대신 그 방향으로 발사
	UFUNCTION(BlueprintCallable, Category = "Lumen|Weapon")
	void FireWeapon(FVector Direction);

	UFUNCTION(BlueprintCallable, Category = "Lumen|Weapon")
	FVector GetMuzzleLocation() const;

	// ===== 텔레그래프 =====
	// 텔레그래프 표시/해제. BTTask_LumenTelegraph가 Show, BTTask_LumenHideTelegraph가 발사 후 Hide
	// C++ 구현이 발사 방향 고정 + 예고음 + TelegraphComp 연출까지 처리. BP 오버라이드 시 Parent 호출 필요
	UFUNCTION(BlueprintNativeEvent, Category = "Lumen|Telegraph")
	void ShowTelegraph(float Duration, float WidthDegrees, FVector AimDirection);
	virtual void ShowTelegraph_Implementation(float Duration, float WidthDegrees, FVector AimDirection);

	UFUNCTION(BlueprintNativeEvent, Category = "Lumen|Telegraph")
	void HideTelegraph();
	virtual void HideTelegraph_Implementation();

	// 발사 전에 공격이 중단됐을 때(BT Abort 등) — 예고선을 끄고 방향 고정도 풀어 포탑이 다시 회전하게 함
	void CancelTelegraph();

	// ===== 하트 UI =====
	// 머리 위 하트 위젯 (Screen Space). 플레이어가 조준 중일 때만 표시
	UPROPERTY(VisibleAnywhere, Category = "UI")
	TObjectPtr<UWidgetComponent> HealthChunksComp;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<ULumenHealthChunksWidget> HealthChunksWidgetClass;

	// 캡슐 윗면에서 하트 위젯까지의 높이
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	float HeartHeightOffset = 30.f;

	// ===== 모션 =====
	// 플레이어와 같은 베이스 메시/애니메이션 재사용
	// 손에 드는 총. BeginPlay에서 WeaponData의 WeaponMesh/WeaponMeshOffset으로 자동 설정
	UPROPERTY(VisibleAnywhere, Category = "Lumen|Weapon")
	TObjectPtr<UStaticMeshComponent> WeaponMeshComp;

	UPROPERTY(EditDefaultsOnly, Category = "Lumen|Weapon")
	FName WeaponSocketName = TEXT("WeaponSocket");

	UPROPERTY(EditDefaultsOnly, Category = "Lumen|Animation")
	TObjectPtr<UAnimMontage> FireMontage;

	// 사망 모션. 재생 후 DeathHideDelay 뒤 사라짐. 비우면 즉시 사라짐
	UPROPERTY(EditDefaultsOnly, Category = "Lumen|Animation")
	TObjectPtr<UAnimMontage> DeathMontage;

	// 사망 모션 시작부터 액터를 숨길 때까지의 시간(초). 0이면 즉시 숨김
	UPROPERTY(EditDefaultsOnly, Category = "Lumen|Animation", meta = (ClampMin = "0.0"))
	float DeathHideDelay = 1.5f;

	// ===== 포탑 =====
	// 고정형: 감지 범위 안의 플레이어를 향해 회전하고, 조준이 맞으면 공격
	// 초당 회전 각도. 플레이어가 이보다 빨리 옆으로 돌아 들어가면 조준을 피할 수 있음 (난이도 조절값)
	UPROPERTY(EditAnywhere, Category = "Lumen|Turret", meta = (ClampMin = "0.0"))
	float TurnRateDegrees = 120.f;

	// 정면에서 이 각도 안에 플레이어가 들어와야 텔레그래프 시작
	UPROPERTY(EditAnywhere, Category = "Lumen|Turret", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float AimToleranceDegrees = 8.f;

	// 과열(냉각) 중에도 계속 따라 돌지 여부. false면 과열 중엔 멈춤 → 돌아 들어갈 반격 기회
	UPROPERTY(EditAnywhere, Category = "Lumen|Turret")
	bool bTrackWhileOverheated = false;

	// BTService_LumenPerception이 호출. 보이면 대상, 안 보이면 nullptr
	void SetTrackTarget(AActor* NewTarget) { TrackTarget = NewTarget; }

	// 포탑 정면(액터 +X)이 대상 쪽으로 AimToleranceDegrees 안에 들어왔는지 (수평 기준)
	bool IsAimedAt(const AActor* Target) const;

	virtual void Tick(float DeltaSeconds) override;

private:
	TWeakObjectPtr<AActor> TrackTarget;
	bool bIsDead = false;

	FTimerHandle DeathHideTimerHandle;
	void HideAfterDeath();

	// 빨강(1발) / 파랑(PelletCount발 부채꼴) 발사체 발사
	void FireProjectileSet(FVector Direction);

	// 초록 라인트레이스 발사. 플레이어와 같은 ULumenCombatLibrary::FireReflectiveLineTrace 사용
	void FireLineTraceWeapon(FVector Direction);

	// ShowTelegraph에서 고정한 발사 방향. 다음 FireWeapon에서 한 번 쓰고 해제되며, 고정 중엔 포탑 회전도 멈춤
	FVector LockedAimDirection = FVector::ForwardVector;
	bool bHasLockedAim = false;

protected:

	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleDeath();
};