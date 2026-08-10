#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UNiagaraSystem;

UCLASS()
class LUMEN_API AProjectile : public AActor
{
	GENERATED_BODY()

public:
	AProjectile();

	virtual void Tick(float DeltaTime) override;

	// 캐릭터가 발사할 때 방향을 넣어주는 함수 (Speed는 내부 값 사용)
	void InitVelocity(const FVector& Direction);

protected:
	virtual void BeginPlay() override;

	void HandleHit(const FHitResult& Hit);

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

	//모서리 반사 해결용
	UPROPERTY()
	TWeakObjectPtr<AActor> LastReflectedActor;

	float IgnoreLastActorTimer = 0.f;
};