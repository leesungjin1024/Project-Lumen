#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Lumen/Reflection/Reflective.h"
#include "ShieldActor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

UCLASS()
class LUMEN_API AShieldActor : public AActor, public IReflective
{
	GENERATED_BODY()

public:
	AShieldActor();

	virtual FVector GetReflectionNormal(const FHitResult& Hit) const override;

	// 캐릭터가 매 프레임 호출 — 마우스/스틱 방향으로 방패 각도 갱신 (4-2)
	void UpdateFacingDirection(const FVector2D& AimDirection);

	// 방패 들기/내리기 — 콜리전을 켜고 끔 (안 든 상태면 반사 판정에 아예 안 걸림)
	void SetShieldActive(bool bActive);

	/** 반사 직후 계산된 방향을, 근처 적 방향으로 살짝 당겨서 보정 (에임 어시스트) */
	UFUNCTION(BlueprintCallable, Category = "Shield|AimAssist")
	FVector ApplyAimAssist(const FVector& ReflectedDirection, const FVector& Origin) const;

protected:
	// 5-2-1: 얕은 각도 대응 — 시각 메시는 평평해도 콜리전은 5~10cm 두께 박스로 별도 적용
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> ShieldCollision;

	// 임시 시각 표현 - 11주차 VFX 단계에서 실제 이펙트/메시로 교체 예정
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComp;

	UPROPERTY(EditDefaultsOnly, Category = "Shield")
	FVector ShieldBoxExtent = FVector(5.f, 40.f, 60.f); // X=두께(5cm 반경→10cm), Y/Z=판 크기 (가안)
	
	// --- 에임 어시스트 (5-4-2 가안): 계산은 정직하게, 실행 부담만 완화 ---
	UPROPERTY(EditDefaultsOnly, Category = "Shield|AimAssist")
	float AimAssistConeHalfAngleDegrees = 20.f; // 이 각도 안에 있는 적만 후보로 고려

	UPROPERTY(EditDefaultsOnly, Category = "Shield|AimAssist")
	float AimAssistSearchRadius = 2000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Shield|AimAssist", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AimAssistBlendAlpha = 0.4f; // 0=순수 계산값 그대로, 1=완전히 적 쪽으로 스냅
};