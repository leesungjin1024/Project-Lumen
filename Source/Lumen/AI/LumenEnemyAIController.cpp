#include "LumenEnemyAIController.h"
#include "LumenAITypes.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"

ALumenEnemyAIController::ALumenEnemyAIController()
{
	bAttachToPawn = true;
}

void ALumenEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (!BehaviorTreeAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] BehaviorTreeAsset이 지정되지 않음"), *GetName());
		return;
	}

	if (!RunBehaviorTree(BehaviorTreeAsset))
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] RunBehaviorTree 실패 — BT/Blackboard 에셋 확인"), *GetName());
	}
	InitializeBlackboard();
}

void ALumenEnemyAIController::InitializeBlackboard()
{
	UBlackboardComponent* BB = GetBlackboardComponent();
	if (!BB)
	{
		return;
	}

	// 싱글 플레이 전제라 플레이어 0번을 그대로 사용 (멀티 지원 시 재검토)
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);

	if (PlayerPawn)
	{
		BB->SetValueAsObject(LumenBBKeys::TargetActor, PlayerPawn);
	}

	BB->SetValueAsBool(LumenBBKeys::HasLineOfSight, false);
	BB->SetValueAsBool(LumenBBKeys::bOverheated, false);
	BB->SetValueAsEnum(LumenBBKeys::AttackState, static_cast<uint8>(ELumenAttackState::Idle));
}
