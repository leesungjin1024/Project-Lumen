// LumenEnemyCharacter.h
//
// 5-5 "최소 적 AI" + 5-5-3 "AI 구현 구조" 반영.
// 무기 데이터(UWeaponDataAsset)·발열(UHeatComponent)·체력(UHealthComponent)은
// 전부 기존 컴포넌트를 그대로 재사용 — "DataAsset 기반이라 플레이어/적 공용"이라는
// 3주차 문서의 설계 그대로.
//
// 적 1체 = 1개 색상만 보유 (0-1: "각자 자신이 지키던 유물의 파장 하나에 잠식되어
// 그 특성의 무기만 쓰는 상태로 남음"). 레벨에 배치할 때 WeaponData만 다르게 지정하면
// 빨강/파랑/초록 3종이 전부 이 클래스 하나로 구현됨.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LumenEnemyCharacter.generated.h"

class UWeaponDataAsset;
class UHeatComponent;
class UHealthComponent;
class UNiagaraSystem;
class AProjectile;
class ULumenAimLineComponent;

UCLASS()
class LUMEN_API ALumenEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ALumenEnemyCharacter();

	// 이 적이 지키는 파장(색상) 하나 - 레벨 배치 시 인스턴스별로 지정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lumen|Weapon")
	TObjectPtr<UWeaponDataAsset> WeaponData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lumen|Combat")
	TObjectPtr<UHeatComponent> HeatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lumen|Combat")
	TObjectPtr<UHealthComponent> HealthComponent;

	// 텔레그래프 비주얼 - 플레이어 조준선과 동일 컴포넌트 재사용 (5-5)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lumen|Telegraph")
	TObjectPtr<ULumenAimLineComponent> TelegraphComp;

	// 반사 스파크 VFX - AProjectile/캐릭터와 동일 에셋을 지정해 시각적 일관성 유지
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lumen|VFX")
	TObjectPtr<UNiagaraSystem> ReflectionSparkVFX;

	// 발사체(빨강/파랑) 스폰용 클래스 - 기존 ALumenCharacter/ALightEmitterActor와 동일하게 지정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lumen|Weapon")
	TSubclassOf<AProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lumen|Weapon")
	FName MuzzleSocketName = TEXT("Muzzle");

	// BTTask_LumenFireWeapon이 호출 - WeaponData->FireMode에 따라 분기
	// (Direction은 BB의 TargetActor 위치 기준으로 Task가 계산해서 넘겨줌)
	UFUNCTION(BlueprintCallable, Category = "Lumen|Weapon")
	void FireWeapon(FVector Direction);

	UFUNCTION(BlueprintCallable, Category = "Lumen|Weapon")
	FVector GetMuzzleLocation() const;

	// 텔레그래프 표시/해제 - BTTask_LumenTelegraph가 호출.
	// C++ 기본 구현은 로그만 남기고, 실제 비주얼(조준선 렌더링 등, 5-5 표)은
	// 블루프린트에서 오버라이드해서 채우는 걸 권장 (5-11 "블루프린트는 UI/애니메이션에 예외 허용"과 동일한 맥락)
	UFUNCTION(BlueprintNativeEvent, Category = "Lumen|Telegraph")
	void ShowTelegraph(float Duration, float WidthDegrees, FVector AimDirection);
	virtual void ShowTelegraph_Implementation(float Duration, float WidthDegrees, FVector AimDirection);

	UFUNCTION(BlueprintNativeEvent, Category = "Lumen|Telegraph")
	void HideTelegraph();
	virtual void HideTelegraph_Implementation();

private:
	// 파랑(샷건) 펠릿 발사 - 3주차 §2-3 FireProjectileSet과 동일한 패턴
	void FireProjectileSet(FVector Direction);

	// 초록(라인트레이스) 발사 - 공용 라이브러리(ULumenCombatLibrary) 호출로 대체
	void FireLineTraceWeapon(FVector Direction);

protected:

	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleDeath();
};
