// UHealthComponent — 플레이어/적 공용 체력(기본 3 = 하트 3칸). OnTakeAnyDamage 기반 피격/사망 처리·효과음, 대쉬 무적, 조준 대상 표시 상태, 펠릿 VolleyId 중복 데미지 방지.
// 연결: ALumenCharacter, ALumenEnemyCharacter, ULumenHealthChunksWidget, AProjectile(TryConsumeVolley)

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float, NewHealthRatio, float, DamageAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeath);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTargetedChanged, bool, bNewTargeted);

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class LUMEN_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHealthComponent();

	// ===== 체력 =====
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health")
	float MaxHealth = 3.0f;

	// 하트 UI 칸 수 (플레이어/적 공용, ULumenHealthChunksWidget). 칸당 체력 = MaxHealth / 칸 수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health")
	int32 NumHealthChunks = 3;

	// ===== 사운드 =====
	// 피격음 / 사망음 — 플레이어·적 BP의 HealthComponent에서 각각 다른 소리 지정 가능. 비워두면 무음
	// 사망음이 비어 있으면 마지막 피격 때도 피격음 재생
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	TObjectPtr<USoundBase> HitSound;

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	TObjectPtr<USoundBase> DeathSound;

	// ===== 이벤트 =====
	// 리스폰(ResetHealth) 때는 DamageAmount = 0
	UPROPERTY(BlueprintAssignable)
	FOnHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable)
	FOnDeath OnDeath;

	UPROPERTY(BlueprintAssignable)
	FOnTargetedChanged OnTargetedChanged;

	// ===== 조회 / 제어 =====
	UFUNCTION(BlueprintCallable, Category = "Health")
	float GetHealthRatio() const { return MaxHealth > 0.f ? CurrentHealth / MaxHealth : 0.f; }

	UFUNCTION(BlueprintCallable, Category = "Health")
	float GetCurrentHealth() const { return CurrentHealth; }

	// 채워진 하트 칸 수 (일부만 남은 칸도 찬 칸으로 올림)
	UFUNCTION(BlueprintCallable, Category = "Health")
	int32 GetFilledChunkCount() const;

	UFUNCTION(BlueprintCallable, Category = "Health")
	bool IsDead() const { return bIsDead; }

	UFUNCTION(BlueprintCallable, Category = "Health")
	bool IsTargeted() const { return bIsTargeted; }

	// 플레이어 조준 대상 여부. ALumenCharacter의 조준 트레이스가 호출, 적 하트 위젯 표시 조건
	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetTargeted(bool bNewTargeted);

	// 대쉬 무적용. 콜리전은 유지하고 데미지만 무시 (콜리전을 끄면 발사체가 뚫고 지나감)
	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetInvulnerable(bool bNewInvulnerable) { bIsInvulnerable = bNewInvulnerable; }

	// 체크포인트 리스폰 시 체력을 완전히 회복하고 사망 상태 해제
	UFUNCTION(BlueprintCallable, Category = "Health")
	void ResetHealth();

	// 파랑 펠릿 중복 데미지 방지: 같은 발사(VolleyId)의 두 번째 이후 펠릿이면 false. Id 0은 항상 통과
	bool TryConsumeVolley(int32 Id)
	{
		if (Id != 0 && Id == LastVolleyId) return false;
		LastVolleyId = Id;
		return true;
	}

protected:
	virtual void BeginPlay() override;

	// 소유 액터의 OnTakeAnyDamage에 BeginPlay에서 바인딩
	UFUNCTION()
	void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
		AController* InstigatedBy, AActor* DamageCauser);

private:
	float CurrentHealth = 0.f;
	bool bIsDead = false;
	bool bIsInvulnerable = false; // 대쉬 무적
	bool bIsTargeted = false;
	int32 LastVolleyId = 0;
};