#include "HeatComponent.h"
#include "WeaponDataAsset.h"
#include "DrawDebugHelpers.h"

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
    //OnHeatChanged.AddDynamic(this, &UHeatComponent::DebugPrintHeat);
}

// 헤더에 UFUNCTION() 붙여서 임시로 추가
void UHeatComponent::DebugPrintHeat(float Ratio)
{
    //테스트 로그
    //GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Yellow, FString::Printf(TEXT("Heat: %.0f%%"), Ratio * 100.f));
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

#if !UE_BUILD_SHIPPING
    if (AActor* Owner = GetOwner())
    {
        const FString DebugText = FString::Printf(
            TEXT("%s\nHeat: %.1f / %.1f%s"),
            *Owner->GetName(),
            CurrentHeat,
            MaxHeat,
            IsOverheated() ? TEXT(" [OVERHEATED]") : TEXT("")
        );
        const FColor TextColor = IsOverheated() ? FColor::Red : FColor::Green;
        DrawDebugString(
            GetWorld(),
            Owner->GetActorLocation() + FVector(0, 0, 120.f),  // 머리 위
            DebugText,
            nullptr,
            TextColor,
            0.f,  // 이 틱에만 그리고 매 프레임 새로 그림 (지속시간 0 = 즉시, 매 틱 갱신)
            true
        );
    }
#endif
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