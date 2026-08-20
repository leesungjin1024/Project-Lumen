#include "HealthComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "DrawDebugHelpers.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = true; // 디버그 표시를 위해 켬 (체력 로직 자체는 여전히 이벤트 기반)
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
	//OnHealthChanged.AddDynamic(this, &UHealthComponent::DebugPrintHealth);
	//OnDeath.AddDynamic(this, &UHealthComponent::DebugPrintDeath);
#if !UE_BUILD_SHIPPING
	UpdateHealthDebugDisplay(); // 시작 시 풀피 상태로 한 번 표시
#endif
}

void UHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

#if !UE_BUILD_SHIPPING
	UpdateHealthDebugDisplay();
#endif
}

void UHealthComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	AController* InstigatedBy, AActor* DamageCauser)
{
	UE_LOG(LogTemp, Warning, TEXT("[Damage Check] bIsDead=%d Damage=%.1f bIsInvulnerable=%d"),
		bIsDead, Damage, bIsInvulnerable);

	if (bIsDead || Damage <= 0.f || bIsInvulnerable)   // ← bIsInvulnerable 추가
	{
		return;
	}

	// 검증용 로그
	UE_LOG(LogTemp, Warning, TEXT("[Damage] Target=%s Damage=%.1f Instigator=%s Causer=%s"),
		*GetOwner()->GetName(), Damage,
		InstigatedBy ? *InstigatedBy->GetName() : TEXT("NULL"),
		DamageCauser ? *DamageCauser->GetName() : TEXT("NULL"));

	CurrentHealth = FMath::Max(0.f, CurrentHealth - Damage);
	OnHealthChanged.Broadcast(GetHealthRatio(), Damage);

	if (CurrentHealth <= 0.f)
	{
		bIsDead = true;
		OnDeath.Broadcast();
	}
#if !UE_BUILD_SHIPPING
	UpdateHealthDebugDisplay();
#endif
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

void UHealthComponent::UpdateHealthDebugDisplay()
{
	if (AActor* Owner = GetOwner())
	{
		const FString DebugText = FString::Printf(
			TEXT("%s\nHP: %.0f / %.0f%s"),
			*Owner->GetName(), CurrentHealth, MaxHealth,
			bIsDead ? TEXT(" [DEAD]") : TEXT("")
		);
		const FColor TextColor = bIsDead ? FColor::Red : (bIsInvulnerable ? FColor::Blue : FColor::White);

		DrawDebugString(GetWorld(), Owner->GetActorLocation() + FVector(0, 0, 220.f),  // ← 위치도 올림
			DebugText, nullptr, TextColor, 0.f, true);   // ← Duration 0 (매 틱 그려서 안 겹침)
	}
}
