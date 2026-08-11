#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UNiagaraSystem;
class UWeaponDataAsset;
extern TAutoConsoleVariable<bool> CVarShowReflectionDebug;

UCLASS()
class LUMEN_API AProjectile : public AActor
{
	GENERATED_BODY()

public:
	AProjectile();

	virtual void Tick(float DeltaTime) override;

	// 캐릭터가 발사할 때 방향을 넣어주는 함수 (Speed는 내부 값 사용)
	void InitVelocity(const FVector& Direction);

	UFUNCTION(BlueprintCallable)
	void InitFromWeaponData(UWeaponDataAsset* WeaponData, const FVector& Direction);

protected:
	virtual void BeginPlay() override;

	void HandleHit(const FHitResult& Hit);

	float CalculateFalloffMultiplier() const; // 신규 추가

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> CollisionComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComp;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float Speed = 2000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Reflection")
	int32 MaxReflections = 5;

	UPROPERTY(EditDefaultsOnly, Category = "VFX")
	TObjectPtr<UNiagaraSystem> ReflectionSparkFX;

	FVector Velocity = FVector::ZeroVector;
	int32 CurrentReflections = 0;
	bool bStopped = false;
	UPROPERTY()
	float Damage = 10.f;

	// MaxReflections는 기존에 있던 하드코딩 멤버 재사용 (있다면 그대로, 없다면 추가)
	// 기존: int32 MaxReflections = 5;  ← 이 기본값은 유지하되, 이제 데이터에셋이 덮어씀

	// 파랑(펠릿) 거리 감쇄용 — 2주차엔 없었으므로 신규 추가
	UPROPERTY()
	float FalloffStartRange = 0.f; // 0이면 감쇄 없음(빨강/초록)

	UPROPERTY()
	float FalloffEndRange = 0.f;

	UPROPERTY()
	float MinDamageMultiplier = 1.f;

	// 이동 거리 누적 — Tick에서 스윕할 때마다 더해서 감쇄 계산에 사용
	UPROPERTY()
	float DistanceTraveled = 0.f;

	//모서리 반사 해결용
	UPROPERTY()
	TWeakObjectPtr<AActor> LastReflectedActor;

	float IgnoreLastActorTimer = 0.f;
};