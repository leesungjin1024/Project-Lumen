// BTTask_LumenFireWeapon.h
//
// 5-5-3 상태 흐름의 "발사(발열 게이지 증가)" 단계.
// 즉시 완료되는 인스턴트 태스크 - 발열 게이지 증가는 ALumenEnemyCharacter::FireWeapon
// 내부에서 HeatComponent->TryConsumeHeat이 처리 (5-1-1 재사용).

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
