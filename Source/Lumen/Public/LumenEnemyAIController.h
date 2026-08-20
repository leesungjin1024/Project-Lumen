// LumenEnemyAIController.h
//
// 5-5-3: "Behavior Tree + Blackboard, 에이전트별 완전 독립 구조",
// "중앙 조율자(GameMode 등) 없이 에이전트별 독립 랜덤"
// -> 이 컨트롤러가 자기 자신의 BT/BB만 소유하고 다른 적과 통신하지 않음.

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
	// 5-5-3: "스폰 시 초기 지연 랜덤화(0~2초)"는 BT 루트의 Wait(랜덤 편차) 노드가 담당하므로
	// 여기서는 BT 실행 시작과 TargetActor(플레이어) 최초 설정만 담당
	void InitializeBlackboard();
};
