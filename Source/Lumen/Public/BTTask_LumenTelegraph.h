// BTTask_LumenTelegraph.h
//
// 5-5-3: "텔레그래프 지속시간의 데이터 소스: 텔레그래프 Task 내부에 하드코딩하지 않고
// 5-1의 무기 DataAsset에서 값을 읽어오게 구현" - 그대로 반영.
// 5-5-1 "초록 텔레그래프 지속시간 조정 검토"가 코드 수정 없이 DataAsset 값만
// 바꿔서 가능하도록, Duration을 여기서 하드코딩하지 않음.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_LumenTelegraph.generated.h"

UCLASS()
class LUMEN_API UBTTask_LumenTelegraph : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_LumenTelegraph();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FMemory); }

private:
	struct FMemory
	{
		float ElapsedTime = 0.f;
		float Duration = 0.f;
	};
};
