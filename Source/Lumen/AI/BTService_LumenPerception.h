// UBTService_LumenPerception — 적 감지 서비스. 시야 확인 → 포탑 회전 대상 지정, 사거리·조준까지 맞으면 BB HasLineOfSight=true, 과열 상태를 BB bOverheated로 복사.
// EQS 없이 단순 거리 + 라인트레이스만 사용. BT 최상위 노드(전체를 감싸는 Selector/Sequence)에 붙여 트리 실행 내내 갱신되게 할 것.
// 연결: ALumenEnemyCharacter(SetTrackTarget/IsAimedAt), UHeatComponent, LumenBBKeys

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_LumenPerception.generated.h"

UCLASS()
class LUMEN_API UBTService_LumenPerception : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_LumenPerception();

	// 감지 최대 거리. 이 안에서 보이면 포탑이 플레이어 쪽으로 회전 시작
	UPROPERTY(EditAnywhere, Category = "Lumen|AI")
	float SightRange = 2000.f;

	// 실제 공격 거리 = Min(SightRange, 무기 사거리 × 이 비율)
	// 1.0이면 사거리 끝에 걸친 플레이어에게 쏴도 탄이 직전에 사라질 수 있어 약간 안쪽으로 잡음
	UPROPERTY(EditAnywhere, Category = "Lumen|AI", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float AttackRangeRatio = 0.9f;

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
