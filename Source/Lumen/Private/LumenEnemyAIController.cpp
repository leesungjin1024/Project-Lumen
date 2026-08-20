// LumenEnemyAIController.cpp

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
	//UE_LOG(LogTemp, Warning, TEXT("[%s] OnPossess 호출됨, Pawn=%s"), *GetName(), *GetNameSafe(InPawn));

	if (!BehaviorTreeAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] BehaviorTreeAsset이 지정되지 않음"), *GetName());
		return;
	}

	const bool bSuccess = RunBehaviorTree(BehaviorTreeAsset);
	//UE_LOG(LogTemp, Warning, TEXT("[%s] RunBehaviorTree 결과: %s"), *GetName(), bSuccess ? TEXT("성공") : TEXT("실패"));
	InitializeBlackboard();
}

void ALumenEnemyAIController::InitializeBlackboard()
{
	UBlackboardComponent* BB = GetBlackboardComponent();
	if (!BB)
	{
		return;
	}

	// 3개월 프로토타입 범위: 플레이어는 항상 1명 (9-3 "GetPlayerController(0) 남용 지양"은
	// 멀티 대비 원칙이라 지금은 예외로 그대로 사용, 정식 개발 시 재검토)
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	//UE_LOG(LogTemp, Warning, TEXT("[%s] GetPlayerPawn 결과=%s"), *GetName(), *GetNameSafe(PlayerPawn));

	if (PlayerPawn)
	{
		BB->SetValueAsObject(LumenBBKeys::TargetActor, PlayerPawn);
	}

	BB->SetValueAsBool(LumenBBKeys::HasLineOfSight, false);
	BB->SetValueAsBool(LumenBBKeys::bOverheated, false);
	BB->SetValueAsEnum(LumenBBKeys::AttackState, static_cast<uint8>(ELumenAttackState::Idle));
}
