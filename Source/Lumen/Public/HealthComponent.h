#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float, NewHealthRatio, float, DamageAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeath);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTargetedChanged, bool, bNewTargeted); // ← 추가

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class LUMEN_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHealthComponent();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health")
	float MaxHealth = 100.f;

	// 플레이어 조각형 HP 표시용 칸 수 (5-9 기본 5칸). 적은 숫자 표시라 이 값 안 씀
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health")
	int32 NumHealthChunks = 5; // ← 추가

	UPROPERTY(BlueprintAssignable)
	FOnHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable)
	FOnDeath OnDeath;

	UPROPERTY(BlueprintAssignable)
	FOnTargetedChanged OnTargetedChanged; // ← 추가

	UFUNCTION(BlueprintCallable, Category = "Health")
	float GetHealthRatio() const { return MaxHealth > 0.f ? CurrentHealth / MaxHealth : 0.f; }

	// 적 숫자 HP바("62/100") 표시용 - 기존엔 비율만 공개돼 있어서 추가
	UFUNCTION(BlueprintCallable, Category = "Health")
	float GetCurrentHealth() const { return CurrentHealth; } // ← 추가

	// 플레이어 조각형 HP 표시용 - 칸 수만큼 올림 처리
	UFUNCTION(BlueprintCallable, Category = "Health")
	int32 GetFilledChunkCount() const; // ← 추가

	UFUNCTION(BlueprintCallable, Category = "Health")
	bool IsDead() const { return bIsDead; }

	UFUNCTION(BlueprintCallable, Category = "Health")
	bool IsTargeted() const { return bIsTargeted; }

	// 적 HP바 표시 여부 제어용 - 타겟팅 판정 로직에서 호출
	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetTargeted(bool bNewTargeted); // ← 추가

	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetInvulnerable(bool bNewInvulnerable) { bIsInvulnerable = bNewInvulnerable; }

	// 체크포인트 리스폰 시 체력을 완전히 회복 (플레이어 사망→재시작 연결용, 신규)
	UFUNCTION(BlueprintCallable, Category = "Health")
	void ResetHealth();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override; // ← 추가

	UFUNCTION()
	void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
		AController* InstigatedBy, AActor* DamageCauser);


		// 임시 테스트용
	UFUNCTION()
	void DebugPrintHealth(float Ratio, float DamageAmount);

	UFUNCTION()
	void DebugPrintDeath();

	// 머리 위 체력 디버그 텍스트 갱신 (몹/캐릭터 구분용)
	void UpdateHealthDebugDisplay();

private:
	float CurrentHealth = 0.f;
	bool bIsDead = false;
	bool bIsInvulnerable = false; // ← 추가, 대쉬(4-2) 무적 프레임용
	bool bIsTargeted = false; // ← 추가
};