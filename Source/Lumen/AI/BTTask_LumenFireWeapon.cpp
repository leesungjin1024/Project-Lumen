#include "BTTask_LumenFireWeapon.h"
#include "LumenAITypes.h"
#include "LumenEnemyCharacter.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"

UBTTask_LumenFireWeapon::UBTTask_LumenFireWeapon()
{
	NodeName = TEXT("Lumen Fire Weapon");
}

EBTNodeResult::Type UBTTask_LumenFireWeapon::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AICon = OwnerComp.GetAIOwner();
	ALumenEnemyCharacter* Enemy = AICon ? Cast<ALumenEnemyCharacter>(AICon->GetPawn()) : nullptr;
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();

	if (!Enemy || !BB)
	{
		return EBTNodeResult::Failed;
	}

	AActor* Target = Cast<AActor>(BB->GetValueAsObject(LumenBBKeys::TargetActor));
	if (!Target)
	{
		return EBTNodeResult::Failed;
	}

	// 텔레그래프로 고정된 방향이 있으면 FireWeapon이 이 값을 무시함
	const FVector Direction = (Target->GetActorLocation() - Enemy->GetMuzzleLocation()).GetSafeNormal();

	BB->SetValueAsEnum(LumenBBKeys::AttackState, static_cast<uint8>(ELumenAttackState::Fire));
	Enemy->FireWeapon(Direction);

	return EBTNodeResult::Succeeded;
}
