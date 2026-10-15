// UBTTask_LumenTelegraph — 적 공격 예고 단계. ShowTelegraph 후 WeaponData->TelegraphDuration 동안 대기. 중단되면 CancelTelegraph로 정리.
// 지속시간은 하드코딩하지 않고 무기 DataAsset에서 읽음 → 색별 예고 시간을 코드 수정 없이 조정 가능.
// 연결: ALumenEnemyCharacter::ShowTelegraph/HideTelegraph, UWeaponDataAsset, LumenBBKeys

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
	// 노드 인스턴스 메모리 (적마다 따로 보관)
	struct FMemory
	{
		float ElapsedTime = 0.f;
		float Duration = 0.f;
	};
};
