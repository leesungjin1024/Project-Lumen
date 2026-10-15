#include "BTService_LumenPerception.h"
#include "LumenAITypes.h"
#include "LumenEnemyCharacter.h"
#include "HeatComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "AIController.h"
#include "WeaponDataAsset.h"

UBTService_LumenPerception::UBTService_LumenPerception()
{
	NodeName = TEXT("Lumen Perception (LOS + Overheat)");
	Interval = 0.2f; // 매 프레임까지는 필요 없음 — 초당 5회 정도면 충분 (비용 절감)
	RandomDeviation = 0.05f;
	bNotifyTick = true;
}

void UBTService_LumenPerception::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AAIController* AICon = OwnerComp.GetAIOwner();
	if (!BB || !AICon)
	{
		return;
	}

	ALumenEnemyCharacter* Enemy = Cast<ALumenEnemyCharacter>(AICon->GetPawn());
	AActor* Target = Cast<AActor>(BB->GetValueAsObject(LumenBBKeys::TargetActor));

	if (!Enemy)
	{
		return;   // 아래에서 Enemy->GetWorld()를 쓰므로 먼저 확인
	}

	if (!Target)
	{
		if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(Enemy->GetWorld(), 0))
		{
			BB->SetValueAsObject(LumenBBKeys::TargetActor, PlayerPawn);
			Target = PlayerPawn;
		}
	}
	// --- LOS 확인 (EQS 대신 단순 트레이스) ---
	// 두 단계로 나눔
	//   1) 감지 (SightRange 안 + 시야 확보)  → 포탑이 플레이어 쪽으로 회전 시작
	//   2) 공격 (무기 사거리 안 + 조준 완료) → HasLineOfSight = true → 텔레그래프/발사
	//   → 플레이어가 포탑 회전보다 빨리 돌아 들어가면 조준을 피할 수 있음
	bool bVisible = false;
	bool bHasLOS = false;
	if (Target)
	{
		const float Distance = FVector::Dist(Enemy->GetActorLocation(), Target->GetActorLocation());

		if (Distance <= SightRange)
		{
			FHitResult Hit;
			FCollisionQueryParams Params;
			Params.AddIgnoredActor(Enemy);
			const bool bBlocked = Enemy->GetWorld()->LineTraceSingleByChannel(
				Hit, Enemy->GetActorLocation(), Target->GetActorLocation(), ECC_Visibility, Params);

			// 방패/거울은 Visibility를 Ignore하므로 방패를 든 플레이어도 그대로 보임
			bVisible = !bBlocked || Hit.GetActor() == Target;
		}

		// 자기 무기가 닿는 거리 안에서만 "공격 가능"으로 판정
		// → 닿지도 않는 거리에서 텔레그래프를 띄우고 헛발사하지 않음
		// → 플레이어 입장에선 "조준선이 안 뜨면 그 적의 사거리 밖"
		float AttackRange = SightRange;
		if (const UWeaponDataAsset* Weapon = Enemy->WeaponData)
		{
			const float WeaponRange = (Weapon->FireMode == EWeaponFireMode::LineTrace)
				? Weapon->TraceRange          // 초록
				: Weapon->FalloffEndRange;    // 빨강/파랑 (에디터 표시명 "Max Range")
			if (WeaponRange > 0.f)
			{
				AttackRange = FMath::Min(SightRange, WeaponRange * AttackRangeRatio);
			}
		}

		// 공격은 "보이고 + 사거리 안 + 포탑이 거의 정면으로 돌아왔을 때"만
		bHasLOS = bVisible && Distance <= AttackRange && Enemy->IsAimedAt(Target);
	}

	// 보이는 동안 회전 대상 지정 (회전 자체는 적 Tick에서 TurnRateDegrees 속도로)
	Enemy->SetTrackTarget(bVisible ? Target : nullptr);
	BB->SetValueAsBool(LumenBBKeys::HasLineOfSight, bHasLOS);

	// --- 과열 상태 반영 (적 무기에도 발열 적용) ---
	if (Enemy->HeatComponent)
	{
		BB->SetValueAsBool(LumenBBKeys::bOverheated, Enemy->HeatComponent->IsOverheated());
	}
}
