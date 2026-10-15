#include "HealthComponent.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false; // 체력 로직은 전부 이벤트(OnTakeAnyDamage) 기반
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;

	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.AddDynamic(this, &UHealthComponent::HandleTakeAnyDamage);
	}
}

void UHealthComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	AController* InstigatedBy, AActor* DamageCauser)
{
	if (bIsDead || Damage <= 0.f || bIsInvulnerable)
	{
		return;
	}

	CurrentHealth = FMath::Max(0.f, CurrentHealth - Damage);
	OnHealthChanged.Broadcast(GetHealthRatio(), Damage);

	// 피격/사망음 — 무적·사망 상태는 위에서 이미 return, 파랑 펠릿은 VolleyId로 1회만 들어옴
	const bool bWillDie = CurrentHealth <= 0.f;
	if (USoundBase* Sound = (bWillDie && DeathSound) ? DeathSound.Get() : HitSound.Get())
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, GetOwner()->GetActorLocation());
	}

	if (bWillDie)
	{
		bIsDead = true;
		OnDeath.Broadcast();
	}
}

void UHealthComponent::ResetHealth()
{
	CurrentHealth = MaxHealth;
	bIsDead = false;

	OnHealthChanged.Broadcast(GetHealthRatio(), 0.f); // 리스폰이므로 DamageAmount는 0
}

int32 UHealthComponent::GetFilledChunkCount() const
{
	if (NumHealthChunks <= 0 || MaxHealth <= 0.f)
	{
		return 0;
	}
	const float PerChunk = MaxHealth / NumHealthChunks;
	// 올림 전에 아주 작은 값을 빼서, 부동소수 오차로 칸이 하나 더 차 보이는 것 방지
	return FMath::Clamp(FMath::CeilToInt(CurrentHealth / PerChunk - KINDA_SMALL_NUMBER), 0, NumHealthChunks);
}


void UHealthComponent::SetTargeted(bool bNewTargeted)
{
	if (bIsTargeted != bNewTargeted)
	{
		bIsTargeted = bNewTargeted;
		OnTargetedChanged.Broadcast(bIsTargeted);
	}
}