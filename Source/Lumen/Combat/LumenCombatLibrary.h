// ULumenCombatLibrary — 초록(라인트레이스) 무기의 즉발 반사·관통 판정. 플레이어/적 공용 정적 함수.
// 연결: ALumenCharacter/ALumenEnemyCharacter(호출), IReflective/AShieldActor, UHealthComponent, APuzzleSensorActor, ALumenBeamSegment
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
	 *   - IReflective면 반사(뒷면이면 정지). 방패에 맞으면 그 시점부터 Instigator/Causer를 방패 소유자로 재설정
	 *   - 관통(bPierces): 반사 대상이 아니면서 UHealthComponent를 가진 액터는 데미지만 주고 계속 진행
	 *   - 그 외(벽, 센서 등)에 맞으면 정지. 센서면 Green으로 NotifyHitByLight
	 *   - 사거리(TraceRange) 소진 또는 세그먼트 수(반사+관통)가 MaxReflections를 넘으면 종료
	 *   - 세그먼트마다 BeamSegmentClass를 스폰해 레이저를 그림
	 *
	 * @param WorldContextObject   트레이스를 실행할 월드를 얻기 위한 컨텍스트(보통 발사 주체 자신)
	 * @param OwnerActor           발사 주체 액터(캐릭터 또는 적) - 스스로를 맞히지 않도록 최초 무시 목록에 들어감
	 * @param StartLocation        발사 시작 위치(총구 소켓 등)
	 * @param Direction            발사 방향(정규화된 벡터)
	 * @param WeaponData           초록 무기 데이터에셋 (Damage/MaxReflections/TraceRange/bPierces)
	 * @param InitialInstigator    최초 발사자의 Controller (플레이어 PC 또는 적 AIController)
	 * @param InitialCauser        최초 발사자 액터(DamageCauser로 사용)
	 * @param ReflectionSparkVFX   반사 지점에 재생할 Niagara 이펙트. AProjectile과 같은 에셋을 넘기면
	 *                             시각적으로 일관됨. 없으면 생략 가능
	 * @param RangeEndVFX          사거리 끝(아무것도 안 맞고 TraceRange 소진)에서 재생할 소멸 이펙트. 없으면 생략 가능
	 * @param RangeMultiplier      TraceRange에 곱하는 배율 (플레이어가 퍼즐룸 안에 있을 때 등). 기본 1
	 *
	 * 사거리 규칙: 맵 거울 반사 시 남은 사거리를 TraceRange로 리셋, 방패 반사 시 그대로 이어감
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
		UNiagaraSystem* ReflectionSparkVFX = nullptr,
		UNiagaraSystem* RangeEndVFX = nullptr,
		float RangeMultiplier = 1.f);
};