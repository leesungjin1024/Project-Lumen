// UBTTask_LumenFireWeapon — 적 공격 흐름의 발사 단계. AttackState=Fire로 바꾸고 FireWeapon 호출 후 즉시 완료.
// 발열 증가는 FireWeapon 안의 HeatComponent->TryConsumeHeat이 처리. 실제 방향은 텔레그래프로 고정된 방향이 우선.
// 연결: ALumenEnemyCharacter::FireWeapon, LumenBBKeys

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_LumenFireWeapon.generated.h"

UCLASS()
class LUMEN_API UBTTask_LumenFireWeapon : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_LumenFireWeapon();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
