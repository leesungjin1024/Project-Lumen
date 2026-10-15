#include "BTTask_LumenHideTelegraph.h"
#include "AIController.h"
#include "LumenEnemyCharacter.h"

UBTTask_LumenHideTelegraph::UBTTask_LumenHideTelegraph()
{
    NodeName = TEXT("Lumen Hide Telegraph");
}

EBTNodeResult::Type UBTTask_LumenHideTelegraph::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* AICon = OwnerComp.GetAIOwner();
    if (!AICon)
    {
        return EBTNodeResult::Failed;
    }

    if (ALumenEnemyCharacter* Enemy = Cast<ALumenEnemyCharacter>(AICon->GetPawn()))
    {
        Enemy->HideTelegraph();
        return EBTNodeResult::Succeeded;
    }

    return EBTNodeResult::Failed;
}