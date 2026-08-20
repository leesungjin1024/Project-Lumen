// LumenAITypes.h
// 5-5-3 결정: Behavior Tree + Blackboard, 에이전트별 완전 독립 구조.
// Blackboard 키 이름을 여기 한 곳에 모아두면, BT 에디터에서 오타로 키 이름이
// 어긋나는 사고를 줄일 수 있음 (BB 키는 반드시 이 이름과 "정확히" 동일하게
// Blackboard 에셋에 추가할 것).

#pragma once

#include "CoreMinimal.h"
#include "LumenAITypes.generated.h"

// 5-5-3 표의 AttackState 열거형 (Idle/Telegraph/Fire/Cooldown)
UENUM(BlueprintType)
enum class ELumenAttackState : uint8
{
	Idle,
	Telegraph,
	Fire,
	Cooldown
};

namespace LumenBBKeys
{
	// Object: 플레이어(타겟) 참조
	static const FName TargetActor(TEXT("TargetActor"));

	// Bool: Service가 매 틱 갱신 (LOS 확인)
	static const FName HasLineOfSight(TEXT("HasLineOfSight"));

	// Enum(ELumenAttackState): 현재 상태
	static const FName AttackState(TEXT("AttackState"));

	// Bool: HeatComponent->IsOverheated()를 Service가 매 틱 복사
	// (5-5-3의 HeatValue를 Float 그대로 노출하는 대신, BT 분기에는
	//  Bool 하나만 있으면 충분해서 Decorator 설계가 더 단순해짐)
	static const FName bOverheated(TEXT("bOverheated"));

	// Float: 과열될 때마다 새로 뽑는 랜덤 냉각 시간(2.5~3.5초, 5-5-1)
	// BT의 Wait 노드가 "Random Deviation"으로 이 범위를 직접 표현하므로
	// 이 키는 디버그/로그 용도로만 쓰고, 실제 대기는 Wait 노드가 담당
	static const FName NextCooldownDuration(TEXT("NextCooldownDuration"));
}
