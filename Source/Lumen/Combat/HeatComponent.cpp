#include "HeatComponent.h"
#include "WeaponDataAsset.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

UHeatComponent::UHeatComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UHeatComponent::BeginPlay()
{
    Super::BeginPlay();
    CurrentHeat = 0.f;
    State = EHeatState::Ready;
}

bool UHeatComponent::TryConsumeHeat(const UWeaponDataAsset* WeaponData)
{
    if (!WeaponData || State == EHeatState::Overheated)
    {
        return false; // 과열 중엔 무조건 발사 불가
    }

    CurrentHeat += WeaponData->HeatPerShot;
    LastUsedWeapon = WeaponData; // 냉각 시 이 무기의 CooldownRate 사용

    if (CurrentHeat >= MaxHeat)
    {
        CurrentHeat = MaxHeat;
        State = EHeatState::Overheated;
        OverheatTimer = OverheatForcedCooldown;
        if (OverheatSound && GetOwner())
        {
            UGameplayStatics::PlaySoundAtLocation(this, OverheatSound, GetOwner()->GetActorLocation());
        }
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

    // 과열(State)은 "발사 가능 여부"만 통제하고, 게이지 자연 냉각은 과열 여부와 무관하게 계속 진행
    if (State == EHeatState::Overheated)
    {
        OverheatTimer -= DeltaTime;
        if (OverheatTimer <= 0.f)
        {
            State = EHeatState::Ready;
            if (OverheatRecoverSound && GetOwner())
            {
                UGameplayStatics::PlaySoundAtLocation(this, OverheatRecoverSound, GetOwner()->GetActorLocation());
            }
            OnHeatRecovered.Broadcast();
        }
        // 여기서 return하지 않음: 강제 쿨타임 중에도 아래 자연 냉각은 계속 진행
    }

    const float CoolRate = LastUsedWeapon ? LastUsedWeapon->CooldownRate : 20.f;
    CurrentHeat = FMath::Max(0.f, CurrentHeat - CoolRate * DeltaTime);
    OnHeatChanged.Broadcast(GetHeatRatio());
}

void UHeatComponent::ResetHeat()
{
    const bool bWasOverheated = (State == EHeatState::Overheated);

    CurrentHeat = 0.f;
    OverheatTimer = 0.f;
    State = EHeatState::Ready;

    OnHeatChanged.Broadcast(GetHeatRatio());

    if (bWasOverheated)
    {
        OnHeatRecovered.Broadcast();
    }
}

void UHeatComponent::SetHeatFraction(float Fraction)
{
    // 과열 상태로 직접 만들지 않음 — 스폰 시점의 "출발선"만 다르게 주는 용도이므로
    // State는 그대로 Ready 유지, TickComponent의 자연 냉각/과열 로직이 이후를 처리
    Fraction = FMath::Clamp(Fraction, 0.f, 1.f);
    CurrentHeat = MaxHeat * Fraction;
    OnHeatChanged.Broadcast(GetHeatRatio());
}