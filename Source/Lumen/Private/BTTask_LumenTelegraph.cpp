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

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();

	// 타겟 방향 계산 - FireWeapon 쪽 Task와 동일한 원칙(BB의 TargetActor 기준)
	FVector Direction = Enemy->GetActorForwardVector();
	if (BB)
	{
		if (AActor* Target = Cast<AActor>(BB->GetValueAsObject(LumenBBKeys::TargetActor)))
		{
			Direction = (Target->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal2D();
		}
	}

	FMemory* Mem = reinterpret_cast<FMemory*>(NodeMemory);
	Mem->ElapsedTime = 0.f;
	Mem->Duration = Enemy->WeaponData->TelegraphDuration;

	if (BB)
	{
		BB->SetValueAsEnum(LumenBBKeys::AttackState, static_cast<uint8>(ELumenAttackState::Telegraph));
	}

	Enemy->ShowTelegraph(Mem->Duration, Enemy->WeaponData->TelegraphWidthDegrees, Direction);
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
