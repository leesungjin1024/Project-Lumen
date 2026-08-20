// BTService_LumenPerception.cpp

#include "BTService_LumenPerception.h"
#include "LumenAITypes.h"
#include "LumenEnemyCharacter.h"
#include "HeatComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "ShieldActor.h"
#include "AIController.h"

UBTService_LumenPerception::UBTService_LumenPerception()
{
	NodeName = TEXT("Lumen Perception (LOS + Overheat)");
	Interval = 0.2f; // 매 프레임까지는 필요 없음 - 5회/초 정도면 충분 (5-5-3 "매 틱" 의도를 유지하면서 비용 절감)
	RandomDeviation = 0.05f;
	bNotifyTick = true;
	//UE_LOG(LogTemp, Warning, TEXT("BTService_LumenPerception 생성자 호출"));
}

void UBTService_LumenPerception::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	//UE_LOG(LogTemp, Error, TEXT("=== TickNode 진입 (Super 호출 전) ==="));  // Error로 강조
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	//UE_LOG(LogTemp, Warning, TEXT("PerceptionService Tick"));

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AAIController* AICon = OwnerComp.GetAIOwner();
	if (!BB || !AICon)
	{
		return;
	}

	ALumenEnemyCharacter* Enemy = Cast<ALumenEnemyCharacter>(AICon->GetPawn());
	AActor* Target = Cast<AActor>(BB->GetValueAsObject(LumenBBKeys::TargetActor));

	if (!Target)
	{
		if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(Enemy->GetWorld(), 0))
		{
			BB->SetValueAsObject(LumenBBKeys::TargetActor, PlayerPawn);
			Target = PlayerPawn;
		}
	}
	// --- LOS 확인 (EQS 대신 단순 트레이스, 5-5-3 결정) ---
	bool bHasLOS = false;
	if (Enemy && Target)
	{
		const float Distance = FVector::Dist(Enemy->GetActorLocation(), Target->GetActorLocation());
		if (Distance <= SightRange)
		{
			FHitResult Hit;
			FCollisionQueryParams Params;
			Params.AddIgnoredActor(Enemy);
			const bool bBlocked = Enemy->GetWorld()->LineTraceSingleByChannel(
				Hit, Enemy->GetActorLocation(), Target->GetActorLocation(), ECC_Visibility, Params);

			if (!bBlocked || Hit.GetActor() == Target)
			{
				bHasLOS = true;
			}
			else if (AShieldActor* Shield = Cast<AShieldActor>(Hit.GetActor()))
			{
				// 방패에 막혔어도, 그 방패의 소유자가 타겟이면 "거기 있다는 건 보인다"로 처리
				bHasLOS = (Shield->GetOwner() == Target);
			}
			else
			{
				bHasLOS = false;
			}
		}
	}
	BB->SetValueAsBool(LumenBBKeys::HasLineOfSight, bHasLOS);

	// --- 과열 상태 반영 (5-5-1: "적 무기에도 발열 적용") ---
	if (Enemy && Enemy->HeatComponent)
	{
		BB->SetValueAsBool(LumenBBKeys::bOverheated, Enemy->HeatComponent->IsOverheated());
	}

	/*UE_LOG(LogTemp, Warning, TEXT("Enemy=%s Target=%s Distance=%f SightRange=%f"),
		*GetNameSafe(Enemy), *GetNameSafe(Target),
		Enemy && Target ? FVector::Dist(Enemy->GetActorLocation(), Target->GetActorLocation()) : -1.f,
		SightRange);
	*/
}
