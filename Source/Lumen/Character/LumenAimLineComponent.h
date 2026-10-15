// ULumenAimLineComponent — 부채꼴 조준선. WeaponDataAsset의 사거리/폭/색을 그대로 읽어 표시하고, 광선 트레이스로 벽에서 잘림.
// 연결: ALumenCharacter(플레이어 조준선), ALumenEnemyCharacter(텔레그래프), UWeaponDataAsset
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
 * 그대로 읽어와 실제 판정과 항상 일치하도록 함.
 *
 * 형태: 삼각형 1장이 아니라 "부채꼴(팬)" — 콘 폭을 여러 광선(Ray)으로 나눠 매 틱 라인트레이스하고,
 *       광선마다 벽/거울/문에 막힌 지점까지만 그림 → 벽 너머로 조준선이 새지 않음.
 *       캐릭터(Pawn)는 무시 — 적이 사거리 안에 있는지 조준선 위에서 바로 보이도록.
 *
 * 메시 섹션
 *   0번: 콘 전체 (플레이어 조준선 / 텔레그래프 배경)
 *   1번: 채움 콘 (텔레그래프 모드에서만 — 적 쪽에서 바깥으로 차오름, 역시 벽에서 잘림)
 *
 * UV (머티리얼 그라데이션용)
 *   U = 시작점에서의 거리 / 최대 사거리 (0~1, 벽에 잘려도 절대 거리 기준이라 무늬가 늘어나지 않음)
 *   V = 콘 폭 방향 위치 (0 = 왼쪽 끝, 0.5 = 중심선, 1 = 오른쪽 끝)
 *
 * 텔레그래프 모드 (StartTelegraph ~ StopTelegraph)
 *   ① 페이드인 → ② 채움 → ③ 발사 직전 번쩍
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

	/** 색 전환 시 호출 — 새 무기 데이터에서 사거리/폭/색을 다시 읽어와 조준선을 갱신 */
	UFUNCTION(BlueprintCallable, Category = "Lumen|AimLine")
	void SetWeaponData(UWeaponDataAsset* NewWeapon);

	/** 조준(발사) 방향 — 매 틱 넘겨줄 것. Z는 무시하고 수평 방향만 사용 */
	UFUNCTION(BlueprintCallable, Category = "Lumen|AimLine")
	void SetAimDirection(const FVector& WorldAimDirection);

	/** 조준선 시작점(총구) — 매 틱 넘겨줄 것. 한 번도 호출 안 하면 액터 중심에서 시작 */
	UFUNCTION(BlueprintCallable, Category = "Lumen|AimLine")
	void SetOriginLocation(const FVector& WorldLocation);

	/** 방별 사거리 배율 (퍼즐룸 등) — 조준선 길이를 실제 사거리와 맞춤. 바뀔 때만 메시 재생성 */
	UFUNCTION(BlueprintCallable, Category = "Lumen|AimLine")
	void SetRangeMultiplier(float Multiplier);

	/** 조준선 표시/숨김 (방패를 든 동안 등) */
	UFUNCTION(BlueprintCallable, Category = "Lumen|AimLine")
	void SetVisible(bool bVisible);

	/** 적 텔레그래프 시작 — Duration은 BT 태스크와 같은 WeaponData->TelegraphDuration을 넘길 것 */
	UFUNCTION(BlueprintCallable, Category = "Lumen|Telegraph")
	void StartTelegraph(float Duration);

	/** 텔레그래프 종료 + 숨김 */
	UFUNCTION(BlueprintCallable, Category = "Lumen|Telegraph")
	void StopTelegraph();

protected:
	// ===== 표시 =====
	// M_AimLine (Translucent/Unlit/Two Sided, UV 그라데이션 + 버텍스 컬러 + "Intensity")
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

	// ===== 벽 가림 =====

	// 벽/거울/문에 막히면 거기까지만 그림. 끄면 항상 최대 사거리까지 그림
	UPROPERTY(EditDefaultsOnly, Category = "Lumen|AimLine|Occlusion")
	bool bClipByWorld = true;

	// 광선 사이 간격(도). 작을수록 벽 모서리가 정확하지만 트레이스 수 증가
	UPROPERTY(EditDefaultsOnly, Category = "Lumen|AimLine|Occlusion", meta = (ClampMin = "0.5"))
	float RayAngleStepDegrees = 3.f;

	// 광선 수 상한 (파랑 40도 → 3도 간격이면 15개)
	UPROPERTY(EditDefaultsOnly, Category = "Lumen|AimLine|Occlusion", meta = (ClampMin = "2", ClampMax = "64"))
	int32 MaxRayCount = 24;

	// ===== 텔레그래프 연출 (적 전용, 플레이어 조준선엔 영향 없음) =====

	// 전체 텔레그래프 시간 중 페이드인에 쓰는 비율
	UPROPERTY(EditDefaultsOnly, Category = "Lumen|Telegraph", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float FadeInRatio = 0.15f;

	// 발사 직전 번쩍이는 시간(초). 전체의 25%를 넘지 않게 자동 제한
	UPROPERTY(EditDefaultsOnly, Category = "Lumen|Telegraph", meta = (ClampMin = "0.0"))
	float FlashTime = 0.1f;

	// 배경 콘(섹션 0)의 불투명도
	UPROPERTY(EditDefaultsOnly, Category = "Lumen|Telegraph", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BackgroundAlpha = 0.35f;

	// 채움 콘(섹션 1)의 불투명도 — 차오르는 동안 Start → End로 변함
	UPROPERTY(EditDefaultsOnly, Category = "Lumen|Telegraph", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FillAlphaStart = 0.6f;
	UPROPERTY(EditDefaultsOnly, Category = "Lumen|Telegraph", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FillAlphaEnd = 1.0f;

	// 번쩍일 때 흰색 쪽으로 섞는 비율 — 너무 높이면 3색 구분이 사라짐
	UPROPERTY(EditDefaultsOnly, Category = "Lumen|Telegraph", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FlashWhiten = 0.6f;

	// 번쩍일 때 머티리얼 "Intensity" 파라미터 값 (평소 1)
	UPROPERTY(EditDefaultsOnly, Category = "Lumen|Telegraph", meta = (ClampMin = "1.0"))
	float FlashIntensity = 3.f;

private:
	UPROPERTY()
	TObjectPtr<UProceduralMeshComponent> MeshComp = nullptr;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> DynamicMat = nullptr;

	UPROPERTY()
	TObjectPtr<UWeaponDataAsset> CurrentWeapon = nullptr;

	FVector AimDirection = FVector::ForwardVector;

	// SetOriginLocation으로 받은 시작점(총구)
	FVector OriginLocation = FVector::ZeroVector;
	bool bHasOrigin = false;
	// 메시 위치를 총구로 옮김. 몸 중심~총구 사이에 벽이 있으면 벽 앞에서 시작
	void ApplyOrigin();

	// RebuildMesh에서 계산 — 무기가 바뀔 때만 갱신
	float CachedLength = 0.f;          // 최대 사거리
	float CachedHalfAngleDeg = 0.f;    // 콘 반각
	FLinearColor CachedColor = FLinearColor::White;
	int32 RayCount = 2;

	// 매 틱 트레이스 결과 — 광선별 실제 길이 (벽에 막히면 짧아짐)
	TArray<float> RayLengths;

	// SetRangeMultiplier로 받은 배율 (기본 1)
	float RangeMultiplier = 1.f;

	bool bTelegraphMode = false;
	float TelegraphElapsed = 0.f;
	float TelegraphTotal = 0.f;

	void RebuildMesh();
	void GetShapeFromWeapon(float& OutLength, float& OutHalfWidth, FLinearColor& OutColor) const;

	float GetRayAngleDeg(int32 Index) const;
	void UpdateRayLengths(const FVector& FlatDir);

	// MaxDistance: 이 거리까지만 그림(채움용). 광선 길이와 둘 중 짧은 쪽 사용. 음수면 광선 길이 그대로
	void WriteFan(int32 SectionIndex, float MaxDistance, const FLinearColor& Color, bool bCreate);
	void UpdateTelegraphVisual();
	void SetIntensity(float Value);
};