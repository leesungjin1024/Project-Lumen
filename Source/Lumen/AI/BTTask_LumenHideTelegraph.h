// UBTTask_LumenHideTelegraph — 적 텔레그래프(예고선)를 즉시 끄는 BT 태스크. BT에서 명시적으로 정리할 때 사용.
// 연결: ALumenEnemyCharacter::HideTelegraph, BTTask_LumenTelegraph

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_LumenHideTelegraph.generated.h"

UCLASS()
class LUMEN_API UBTTask_LumenHideTelegraph : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_LumenHideTelegraph();

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};