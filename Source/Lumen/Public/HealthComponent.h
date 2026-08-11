#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float, NewHealthRatio, float, DamageAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeath);

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class LUMEN_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHealthComponent();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health")
	float MaxHealth = 100.f;

	UPROPERTY(BlueprintAssignable)
	FOnHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable)
	FOnDeath OnDeath;

	UFUNCTION(BlueprintCallable, Category = "Health")
	float GetHealthRatio() const { return MaxHealth > 0.f ? CurrentHealth / MaxHealth : 0.f; }

	UFUNCTION(BlueprintCallable, Category = "Health")
	bool IsDead() const { return bIsDead; }

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
		AController* InstigatedBy, AActor* DamageCauser);


		// 임시 테스트용
	UFUNCTION()
	void DebugPrintHealth(float Ratio, float DamageAmount);

	UFUNCTION()
	void DebugPrintDeath();

private:
	float CurrentHealth = 0.f;
	bool bIsDead = false;
};