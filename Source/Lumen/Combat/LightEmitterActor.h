// ALightEmitterActor — 퍼즐용 광원. 액터 정면(forward) 방향으로 일정 간격마다 WeaponData의 발사체를 자동 발사.
// 연결: UWeaponDataAsset, AProjectile (Pawn이 아닌 소유자 → 플레이어 발사체 채널로 취급)
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LightEmitterActor.generated.h"

class UArrowComponent;
class UWeaponDataAsset;
class AProjectile;

UCLASS()
class LUMEN_API ALightEmitterActor : public AActor
{
	GENERATED_BODY()

public:
	ALightEmitterActor();

protected:
	virtual void BeginPlay() override;

	// PelletCount만큼 부채꼴로 발사체 스폰 (빨강 1발 / 파랑 펠릿)
	void Fire();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UArrowComponent> DirectionArrow; // 에디터에서 발사 방향을 눈으로 보기 위함 (액터 forward = 발사 방향)

	// ===== 발사 =====
	// 발사할 무기 데이터 재사용 - 색상별 Speed/Damage/MaxReflections/펠릿 설정이 그대로 적용됨.
	// 발사체 전용 — 라인트레이스형(초록) 무기는 지원 안 함
	UPROPERTY(EditAnywhere, Category = "Emitter")
	TObjectPtr<UWeaponDataAsset> WeaponData;

	UPROPERTY(EditAnywhere, Category = "Emitter")
	TSubclassOf<AProjectile> ProjectileClass;

	// 발사 간격(초)
	UPROPERTY(EditAnywhere, Category = "Emitter")
	float FireInterval = 1.5f;

	// 켜면 BeginPlay 직후부터 FireInterval마다 발사
	UPROPERTY(EditAnywhere, Category = "Emitter")
	bool bStartActive = true;

	FTimerHandle FireTimerHandle;
};
