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

	void Fire();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UArrowComponent> DirectionArrow; // 에디터에서 발사 방향을 눈으로 보기 위함 (액터 forward = 발사 방향)

	// 발사할 무기 데이터 재사용 - 색상별 Speed/Damage/MaxReflections 그대로 적용됨
	UPROPERTY(EditAnywhere, Category = "Emitter")
	TObjectPtr<UWeaponDataAsset> WeaponData;

	UPROPERTY(EditAnywhere, Category = "Emitter")
	TSubclassOf<AProjectile> ProjectileClass;

	// 라인트레이스형(초록) 무기는 지금은 지원 안 함 - 발사체 전용
	UPROPERTY(EditAnywhere, Category = "Emitter")
	float FireInterval = 1.5f; // 몇 초마다 발사할지 (가안)

	UPROPERTY(EditAnywhere, Category = "Emitter")
	bool bStartActive = true;

	FTimerHandle FireTimerHandle;
};
