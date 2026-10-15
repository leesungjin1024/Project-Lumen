// AShieldActor — 플레이어 방패. 든 동안만 콜리전이 켜져 탄/레이저를 반사하고, 반사 방향을 근처 적 쪽으로 살짝 보정(에임 어시스트).
// 연결: ALumenCharacter(스폰·부착·조준 갱신), IReflective / AProjectile, ULumenCombatLibrary(반사·데미지 귀속 재설정)
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Reflective.h"
#include "ShieldActor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class USoundBase;

UCLASS()
class LUMEN_API AShieldActor : public AActor, public IReflective
{
	GENERATED_BODY()

public:
	AShieldActor();

	// 블루프린트에서 바꾼 ShieldBoxExtent를 박스에 적용 (생성자 시점엔 BP 기본값이 아직 반영 전)
	virtual void OnConstruction(const FTransform& Transform) override;

	// 방패 정면(forward)을 Z=0으로 평탄화한 노멀 — 조준 방향 그대로 반사면이 됨
	virtual FVector GetReflectionNormal(const FHitResult& Hit) const override;

	// 캐릭터가 매 프레임 호출 — 마우스/스틱 조준 방향으로 방패 Yaw 갱신
	void UpdateFacingDirection(const FVector2D& AimDirection);

	// 방패 들기/내리기 — 콜리전과 메시 표시를 켜고 끔 (안 든 상태면 반사 판정에 아예 안 걸림)
	void SetShieldActive(bool bActive);

	// 방패에 탄/레이저가 막혀 튕겨날 때 — AProjectile / ULumenCombatLibrary에서 호출
	void PlayBlockSound(const FVector& Location) const;

	/** 반사 직후 계산된 방향을, 근처 적 방향으로 살짝 당겨서 보정 (에임 어시스트) */
	UFUNCTION(BlueprintCallable, Category = "Shield|AimAssist")
	FVector ApplyAimAssist(const FVector& ReflectedDirection, const FVector& Origin) const;

protected:
	// ===== 컴포넌트 =====
	// 얕은 각도에서도 맞도록 시각 메시는 평평해도 콜리전은 두께 있는 박스로 별도 적용
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> ShieldCollision;

	// 방패 시각 표현 (임시 메시). 방패를 든 동안만 보임
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComp;

	// ===== 사운드 =====
	// 방패에 막히는 소리 (무기 반사음과 함께 재생됨). 비워두면 무음
	UPROPERTY(EditDefaultsOnly, Category = "Shield|Sound")
	TObjectPtr<USoundBase> BlockSound;

	// ===== 방패 =====
	// 콜리전 박스 반경. X=두께(반경 5 → 실제 10cm), Y/Z=판 크기
	UPROPERTY(EditDefaultsOnly, Category = "Shield")
	FVector ShieldBoxExtent = FVector(5.f, 40.f, 60.f);

	// ===== 에임 어시스트 =====
	// 반사 계산은 그대로 하고, 결과 방향만 원뿔 안의 가장 가까운 각도의 적 쪽으로 살짝 당김
	// 반사 방향 기준 이 각도(도) 안에 있는 적만 후보로 고려
	UPROPERTY(EditDefaultsOnly, Category = "Shield|AimAssist")
	float AimAssistConeHalfAngleDegrees = 20.f;

	// 후보 적(UHealthComponent 보유 Pawn)을 찾는 반경(cm)
	UPROPERTY(EditDefaultsOnly, Category = "Shield|AimAssist")
	float AimAssistSearchRadius = 2000.f;

	// 보정 강도. 0=반사 계산값 그대로, 1=완전히 적 쪽으로 스냅
	UPROPERTY(EditDefaultsOnly, Category = "Shield|AimAssist", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AimAssistBlendAlpha = 0.4f;
};