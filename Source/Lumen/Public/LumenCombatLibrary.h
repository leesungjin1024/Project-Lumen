// LumenCombatLibrary.h
//
// 배경: 2주차/3주차 문서에서 FireLineTrace가 ALumenCharacter 전용으로 짜여 있어
//   (1) ALightEmitterActor(테스트/퍼즐 발사기)가 초록을 못 쏘고
//   (2) 이번에 추가하는 AEnemyCharacter도 초록을 쏘려면 같은 로직을 또 복붙해야 하는 문제가 있었음.
// 해결: 반사+관통 트레이스 로직을 정적 함수로 뽑아서 캐릭터/AI/이미터가 전부 공용으로 씀.
//   (5-1 "데이터만 다르게" 철학을 로직 재사용에도 그대로 적용)
//
// 이 함수 하나가 다음을 전부 처리함:
//   - IReflective 판별 및 반사 (기존 FireLineTrace의 while 루프 그대로 이식)
//   - 관통(bPierces): 반사 대상이 아니면서 데미지를 받을 수 있는 액터(UHealthComponent 보유)를
//     맞히면 정지하지 않고 데미지만 적용한 뒤 계속 진행
//   - 반사 데미지 역추적: 방패(AShieldActor)에 맞아 반사되면 그 시점부터 Instigator/Causer를
//     방패 소유자로 재설정 (3주차 §2-4-1과 동일 원칙, 로컬 변수로 갱신 — 3주차 5장에서
//     이미 이렇게 하기로 정해둔 방식)
//   - 총 이동 거리(WeaponData->TraceRange) 초과 시 종료
//   - 반사 횟수(WeaponData->MaxReflections) 초과 시 종료 (관통으로 지나간 횟수도 함께 카운트)

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LumenCombatLibrary.generated.h"

class UWeaponDataAsset;
class UNiagaraSystem;

UCLASS()
class LUMEN_API ULumenCombatLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 초록(라인트레이스) 발사 - 반사 + 관통을 한 프레임 내에서 처리.
	 *
	 * @param WorldContextObject   트레이스를 실행할 월드를 얻기 위한 컨텍스트(보통 발사 주체 자신)
	 * @param OwnerActor           발사 주체 액터(캐릭터 또는 적) - 스스로를 맞히지 않도록 최초 무시 목록에 들어감
	 * @param StartLocation        발사 시작 위치(총구 소켓 등)
	 * @param Direction            발사 방향(정규화된 벡터)
	 * @param WeaponData           초록 무기 데이터에셋 (Damage/MaxReflections/TraceRange/bPierces)
	 * @param InitialInstigator    최초 발사자의 Controller (플레이어 PC 또는 적 AIController)
	 * @param InitialCauser        최초 발사자 액터(DamageCauser로 사용)
	 * @param ReflectionSparkVFX   반사 지점에 재생할 Niagara 이펙트(5-2-2). 기존 AProjectile이 쓰는
	 *                             에셋과 동일한 것을 넘기면 시각적으로도 일관됨. 없으면 생략 가능
	 */
	UFUNCTION(BlueprintCallable, Category = "Lumen|Combat")
	static void FireReflectiveLineTrace(
		const UObject* WorldContextObject,
		AActor* OwnerActor,
		FVector StartLocation,
		FVector Direction,
		UWeaponDataAsset* WeaponData,
		AController* InitialInstigator,
		AActor* InitialCauser,
		UNiagaraSystem* ReflectionSparkVFX = nullptr);
};
