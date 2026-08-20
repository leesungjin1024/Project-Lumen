// BTTask_LumenTelegraph.cpp

#include "BTTask_LumenTelegraph.h"
#include "LumenAITypes.h"
#include "LumenEnemyCharacter.h"
#include "WeaponDataAsset.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"

UBTTask_LumenTelegraph::UBTTask_LumenTelegraph()
{
	NodeName = TEXT("Lumen Telegraph");
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_LumenTelegraph::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AICon = OwnerComp.GetAIOwner();
	ALumenEnemyCharacter* Enemy = AICon ? Cast<ALumenEnemyCharacter>(AICon->GetPawn()) : nullptr;

	if (!Enemy || !Enemy->WeaponData)
	{
		return EBTNodeResult::Failed;
	}

	FMemory* Mem = reinterpret_cast<FMemory*>(NodeMemory);
	Mem->ElapsedTime = 0.f;
	Mem->Duration = Enemy->WeaponData->TelegraphDuration; // 5-5-3: 하드코딩 대신 DataAsset에서 읽음

	if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
	{
		BB->SetValueAsEnum(LumenBBKeys::AttackState, static_cast<uint8>(ELumenAttackState::Telegraph));
	}

	Enemy->ShowTelegraph(Mem->Duration, Enemy->WeaponData->TelegraphWidthDegrees);

	return EBTNodeResult::InProgress;
}

void UBTTask_LumenTelegraph::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FMemory* Mem = reinterpret_cast<FMemory*>(NodeMemory);
	Mem->ElapsedTime += DeltaSeconds;

	if (Mem->ElapsedTime >= Mem->Duration)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

void UBTTask_LumenTelegraph::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	if (AAIController* AICon = OwnerComp.GetAIOwner())
	{
		if (ALumenEnemyCharacter* Enemy = Cast<ALumenEnemyCharacter>(AICon->GetPawn()))
		{
			Enemy->HideTelegraph();
		}
	}

	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}
