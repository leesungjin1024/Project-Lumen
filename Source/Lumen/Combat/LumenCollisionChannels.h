// 프로젝트 커스텀 콜리전 채널 별칭 — DefaultEngine.ini의 GameTraceChannel 순서와 반드시 일치해야 함.
// 연결: AProjectile, ULumenCombatLibrary, ULumenAimLineComponent, AMirrorActor, AShieldActor
#pragma once
#include "Engine/EngineTypes.h"

#define ECC_Reflective        ECC_GameTraceChannel1 // 거울/방패 오브젝트 타입 (Visibility는 Ignore)
#define ECC_PlayerProjectile  ECC_GameTraceChannel2 // 플레이어(및 Pawn 아닌 소유자) 발사체 오브젝트 타입
#define ECC_EnemyProjectile   ECC_GameTraceChannel3 // 적 발사체 오브젝트 타입
#define ECC_ProjectileTrace   ECC_GameTraceChannel4 // 발사체 스윕/레이저/조준선 트레이스 채널. 트리거 볼륨은 Ignore 필요 (Trigger 프리셋이 커스텀 채널 기본값 Block을 물려받음)