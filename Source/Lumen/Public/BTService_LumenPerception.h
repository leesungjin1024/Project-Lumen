// BTService_LumenPerception.h
//
// 5-5-3: "HasLineOfSight | Bool | Service가 매 틱 갱신",
// "EQS는 사용하지 않음 - 좁은 감지 범위 + 단순 LOS 트레이스만으로 충분".
// 이 Service를 BT의 최상위 노드(전체를 감싸는 Selector/Sequence)에 붙여서
// 트리 실행 내내 매 틱 LOS/과열 상태를 갱신하게 구성할 것.

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

	// 감지 판정 최대 거리 (5-5-3: "좁은 감지 범위"에 대응하는 값, 그레이박스에서 튜닝)
	UPROPERTY(EditAnywhere, Category = "Lumen|AI")
	float SightRange = 2000.f;

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
