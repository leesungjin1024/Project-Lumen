#include "HeatComponent.h"
#include "WeaponDataAsset.h"

UHeatComponent::UHeatComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UHeatComponent::BeginPlay()
{
    Super::BeginPlay();
    CurrentHeat = 0.f;
    State = EHeatState::Ready;
    // 임시 테스트용 - 확인 후 삭제
    OnHeatChanged.AddDynamic(this, &UHeatComponent::DebugPrintHeat);
}

// 헤더에 UFUNCTION() 붙여서 임시로 추가
void UHeatComponent::DebugPrintHeat(float Ratio)
{
    GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Yellow, FString::Printf(TEXT("Heat: %.0f%%"), Ratio * 100.f));
}

bool UHeatComponent::TryConsumeHeat(const UWeaponDataAsset* WeaponData)
{
    if (!WeaponData || State == EHeatState::Overheated)
    {
        return false; // 과열 중엔 무조건 발사 불가
    }

    CurrentHeat += WeaponData->HeatPerShot;
    LastUsedWeapon = WeaponData; // Option B: 마지막 사용 무기 기록 → 냉각 시 이 무기의 CooldownRate 사용

    if (CurrentHeat >= MaxHeat)
    {
        CurrentHeat = MaxHeat;
        State = EHeatState::Overheated;
        OverheatTimer = OverheatForcedCooldown;
        OnOverheated.Broadcast();
    }

    OnHeatChanged.Broadcast(GetHeatRatio());
    return true;
}

void UHeatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (CurrentHeat <= 0.f && State == EHeatState::Ready)
    {
        return;
    }

    // 강제 쿨타임 타이머는 과열 여부(State)와 별개로 흐름 — State는 "재사용 가능 여부"만 통제
    if (State == EHeatState::Overheated)
    {
        OverheatTimer -= DeltaTime;
        if (OverheatTimer <= 0.f)
        {
            State = EHeatState::Ready;
            OnHeatRecovered.Broadcast();
        }
        // return 제거: 강제 쿨타임 중에도 자연 냉각은 계속 진행됨
    }

    const float CoolRate = LastUsedWeapon ? LastUsedWeapon->CooldownRate : 20.f;
    CurrentHeat = FMath::Max(0.f, CurrentHeat - CoolRate * DeltaTime);
    OnHeatChanged.Broadcast(GetHeatRatio());
}