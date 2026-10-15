// ALumenEnemyAIController — 적 1체당 BT/BB를 하나씩 소유하고 실행. 중앙 조율자 없이 적끼리 통신하지 않는 독립 구조.
// 연결: ALumenEnemyCharacter, LumenBBKeys(LumenAITypes.h), BTService_LumenPerception / BTTask_Lumen*

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "LumenEnemyAIController.generated.h"

class UBehaviorTree;
class UBlackboardComponent;

UCLASS()
class LUMEN_API ALumenEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	ALumenEnemyAIController();

	UPROPERTY(EditDefaultsOnly, Category = "Lumen|AI")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

	virtual void OnPossess(APawn* InPawn) override;

protected:
	// BB 초기값 설정: TargetActor(플레이어), HasLineOfSight/bOverheated=false, AttackState=Idle
	// 스폰 시 초기 지연 랜덤화는 BT 루트의 Wait(랜덤 편차) 노드가 담당
	void InitializeBlackboard();
};
