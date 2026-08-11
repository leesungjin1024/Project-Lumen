#include "HealthComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false; // 체력은 이벤트 기반이라 Tick 불필요
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;

	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.AddDynamic(this, &UHealthComponent::HandleTakeAnyDamage);
	}
	// 임시 테스트용 - 확인 후 삭제
	OnHealthChanged.AddDynamic(this, &UHealthComponent::DebugPrintHealth);
	OnDeath.AddDynamic(this, &UHealthComponent::DebugPrintDeath);
}

void UHealthComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	AController* InstigatedBy, AActor* DamageCauser)
{
	if (bIsDead || Damage <= 0.f)
	{
		return;
	}

	CurrentHealth = FMath::Max(0.f, CurrentHealth - Damage);
	OnHealthChanged.Broadcast(GetHealthRatio(), Damage);

	if (CurrentHealth <= 0.f)
	{
		bIsDead = true;
		OnDeath.Broadcast();
	}
}

//테스트용
void UHealthComponent::DebugPrintHealth(float Ratio, float DamageAmount)
{
	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red,
		FString::Printf(TEXT("Took %.1f dmg | HP: %.0f%%"), DamageAmount, Ratio * 100.f));
}

void UHealthComponent::DebugPrintDeath()
{
	GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan, TEXT("DEAD"));
}