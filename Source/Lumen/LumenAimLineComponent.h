// LumenAimLineComponent.h
// 5-1/5-5 조준선을 실제 WeaponDataAsset 값(사거리/감쇠/폭/색)에서 직접 읽어와 표시.
// ALumenCharacter에 부착.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProceduralMeshComponent.h"
#include "LumenAimLineComponent.generated.h"

class UWeaponDataAsset;

/**
 * 캐릭터 루트에 부착되는 조준선 표시 컴포넌트.
 * 색상별 하드코딩 값을 갖지 않고, 현재 장착된 UWeaponDataAsset에서
 * 사거리(TraceRange 또는 FalloffEndRange)/폭(TelegraphWidthDegrees)/색(WeaponColor_TEMP)을
 * 그대로 읽어와 실제 판정과 항상 일치하도록 함. (5-2-1: 초록=하드 캡, 빨강/파랑=감쇠 종료 지점)
 */
UCLASS(ClassGroup = (Lumen), meta = (BlueprintSpawnableComponent))
class LUMEN_API ULumenAimLineComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULumenAimLineComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** 5-7 색 전환 시 호출 — 새 무기 데이터에서 사거리/폭/색을 다시 읽어와 조준선을 갱신 */
	UFUNCTION(BlueprintCallable, Category = "Lumen|AimLine")
	void SetWeaponData(UWeaponDataAsset* NewWeapon);

	/** ALumenCharacter::CurrentAimDirection을 매 틱 여기로 넘겨줄 것 */
	UFUNCTION(BlueprintCallable, Category = "Lumen|AimLine")
	void SetAimDirection(const FVector& WorldAimDirection);

	/** 방패를 든 동안 조준선을 숨기고 싶을 때 사용 (5-4-1) */
	UFUNCTION(BlueprintCallable, Category = "Lumen|AimLine")
	void SetVisible(bool bVisible);

protected:
	// 반투명 언릿 머티리얼 권장 (버텍스 컬러를 곱해서 쓰는 형태)
	UPROPERTY(EditDefaultsOnly, Category = "Lumen|AimLine")
	UMaterialInterface* BaseMaterial = nullptr;

	// 바닥 z-fighting 방지용 높이 오프셋
	UPROPERTY(EditDefaultsOnly, Category = "Lumen|AimLine")
	float HeightOffset = 5.f;

	// 무기 데이터가 아직 없을 때(초기화 전) 쓰는 대체값
	UPROPERTY(EditDefaultsOnly, Category = "Lumen|AimLine|Fallback")
	float FallbackLength = 300.f;
	UPROPERTY(EditDefaultsOnly, Category = "Lumen|AimLine|Fallback")
	float FallbackHalfWidth = 20.f;
	UPROPERTY(EditDefaultsOnly, Category = "Lumen|AimLine|Fallback")
	FLinearColor FallbackColor = FLinearColor::White;

private:
	UPROPERTY()
	TObjectPtr<UProceduralMeshComponent> MeshComp = nullptr;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> DynamicMat = nullptr;

	UPROPERTY()
	TObjectPtr<UWeaponDataAsset> CurrentWeapon = nullptr;

	FVector AimDirection = FVector::ForwardVector;

	void RebuildMesh();
	void GetShapeFromWeapon(float& OutLength, float& OutHalfWidth, FLinearColor& OutColor) const;
};