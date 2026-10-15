// LumenAITypes — 적 AI 공용 타입: 공격 상태 열거형 + Blackboard 키 이름 모음(오타로 키가 어긋나는 사고 방지).
// Blackboard 에셋의 키는 반드시 이 이름과 정확히 같게 추가할 것.
// 연결: ALumenEnemyAIController, BTService_LumenPerception, BTTask_LumenTelegraph/FireWeapon

#pragma once

#include "CoreMinimal.h"
#include "LumenAITypes.generated.h"

// 적 공격 상태. BB AttackState 키에 저장 (C++에선 Idle/Telegraph/Fire만 설정. Cooldown은 Blackboard 에셋 호환용으로 남겨 둠)
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

	// Bool: 공격 가능 여부 (보임 + 사거리 안 + 조준 완료). BTService_LumenPerception이 갱신
	static const FName HasLineOfSight(TEXT("HasLineOfSight"));

	// Enum(ELumenAttackState): 현재 상태
	static const FName AttackState(TEXT("AttackState"));

	// Bool: HeatComponent->IsOverheated()를 Service가 복사
	// 발열 수치(Float) 대신 Bool만 노출 — BT 분기엔 이것만 있으면 충분해 Decorator가 단순해짐
	static const FName bOverheated(TEXT("bOverheated"));

	// 냉각 대기는 BT의 Wait 노드(Random Deviation)가 담당하므로 별도 키 없음
}
