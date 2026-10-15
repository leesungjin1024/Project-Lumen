// ALumenBeamSegment — 초록 레이저의 보이는 한 구간(시작점→끝점). 판정은 이미 끝난 뒤 짧게 그렸다가 가늘어지며 사라짐.
// 실제 메시라 Shipping에서도 보임. BP_BeamSegment(메시/머티리얼 지정)를 초록 WeaponDataAsset::BeamSegmentClass에 할당.
// 연결: ULumenCombatLibrary::FireReflectiveLineTrace(반사·관통 세그먼트마다 스폰)
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LumenBeamSegment.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

UCLASS()
class LUMEN_API ALumenBeamSegment : public AActor
{
	GENERATED_BODY()

public:
	ALumenBeamSegment();

	virtual void Tick(float DeltaTime) override;

	// 스폰 직후 호출 — 두 점 사이를 잇도록 위치/회전/길이를 맞추고 색을 적용
	void InitSegment(const FVector& Start, const FVector& End, const FLinearColor& Color);

protected:
	// BP에서 /Engine/BasicShapes/Cylinder + M_Projectile 지정
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComp;

	// ===== 빔 =====
	// 레이저 두께(cm)
	UPROPERTY(EditDefaultsOnly, Category = "Beam", meta = (ClampMin = "0.1"))
	float BeamWidth = 6.f;

	// 화면에 남아 있는 시간(초) — 이 동안 점점 가늘어지며 사라짐
	UPROPERTY(EditDefaultsOnly, Category = "Beam", meta = (ClampMin = "0.01"))
	float LifeTime = 0.15f;

	// 사용하는 메시의 기본 크기(cm). 엔진 Cylinder는 지름 100, 높이 100
	UPROPERTY(EditDefaultsOnly, Category = "Beam", meta = (ClampMin = "1.0"))
	float MeshBaseSize = 100.f;

private:
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> MeshMID;

	float Elapsed = 0.f;
	float SegmentLength = 0.f;

	// 길이는 SegmentLength 고정, 두께만 BeamWidth * WidthScale로 스케일
	void ApplyScale(float WidthScale);
};
