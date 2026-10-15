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
	bNotifyTaskFinished = true; // 이게 없으면 OnTaskFinished가 호출되지 않음
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

	// BB의 TargetActor 기준 대략적인 방향 (액터 중심 기준). ShowTelegraph가 총구 기준으로 다시 계산함
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
	// 성공 시엔 그대로 둠 — 다음 FireWeapon이 고정된 예고선 방향으로 쏘고, 발사 후 HideTelegraph 태스크가 끔.
	// 발사 전에 중단(Abort/실패)되면 예고선이 남거나 포탑 회전이 멈춘 채로 남지 않게 정리
	if (TaskResult != EBTNodeResult::Succeeded)
	{
		if (AAIController* AICon = OwnerComp.GetAIOwner())
		{
			if (ALumenEnemyCharacter* Enemy = Cast<ALumenEnemyCharacter>(AICon->GetPawn()))
			{
				Enemy->CancelTelegraph();
			}
		}
	}

	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}
