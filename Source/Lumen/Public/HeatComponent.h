
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HeatComponent.generated.h"

class UWeaponDataAsset;

UENUM(BlueprintType)
enum class EHeatState : uint8
{
    Ready,       // 정상 사격 가능
    Overheated   // 과열, 강제 쿨타임 중
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHeatChanged, float, NewHeatRatio); // UI/AI Blackboard 바인딩용
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOverheated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHeatRecovered);

UCLASS(ClassGroup = (Weapon), meta = (BlueprintSpawnableComponent))
class LUMEN_API UHeatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UHeatComponent();

    // 0~MaxHeat 범위의 단일 게이지 (5-1-1: 색 간 공유)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Heat")
    float MaxHeat = 100.f;

    // 과열 시 강제 쿨타임 (5-4-1과 동일한 원칙: 완전 소진 시 짧은 강제 대기)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Heat")
    float OverheatForcedCooldown = 2.f;

    UPROPERTY(BlueprintAssignable)
    FOnHeatChanged OnHeatChanged;

    UPROPERTY(BlueprintAssignable)
    FOnOverheated OnOverheated;

    UPROPERTY(BlueprintAssignable)
    FOnHeatRecovered OnHeatRecovered;
    //테스트용
    UFUNCTION()
    void DebugPrintHeat(float Ratio);

    // 발사 시 무기 쪽에서 호출 — 발사 가능 여부를 반환해서 무기가 스스로 판단하게 함
    UFUNCTION(BlueprintCallable, Category = "Heat")
    bool TryConsumeHeat(const UWeaponDataAsset* WeaponData);

    UFUNCTION(BlueprintCallable, Category = "Heat")
    bool IsOverheated() const { return State == EHeatState::Overheated; }

    UFUNCTION(BlueprintCallable, Category = "Heat")
    float GetHeatRatio() const { return MaxHeat > 0.f ? CurrentHeat / MaxHeat : 0.f; }

    // 체크포인트 리스폰 시 발열을 완전히 초기화 (플레이어 사망→재시작 연결용, 신규)
    UFUNCTION(BlueprintCallable, Category = "Heat")
    void ResetHeat();

    // 전투룸에 여러 적을 스폰할 때 발열 사이클을 서로 어긋나게 하기 위한 초기값 설정 (신규)
    // Fraction은 0~1, MaxHeat 대비 비율. 과열 상태로 직접 진입시키지는 않음(Ready 유지).
    UFUNCTION(BlueprintCallable, Category = "Heat")
    void SetHeatFraction(float Fraction);

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    float CurrentHeat = 0.f;
    EHeatState State = EHeatState::Ready;
    float OverheatTimer = 0.f;

    // 과열 진입 직전까지 어떤 무기로 쐈는지 기억 — 냉각 중엔 소모율이 아니라 냉각률만 필요하므로
    // 매 틱 냉각률을 어디서 가져올지가 문제. 아래에서 처리.
    UPROPERTY()
    const UWeaponDataAsset* LastUsedWeapon = nullptr;
};