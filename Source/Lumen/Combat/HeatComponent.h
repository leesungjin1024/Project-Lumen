// UHeatComponent — 플레이어/적 공용 단일 발열 게이지(모든 색 공유). 발사마다 HeatPerShot 누적, MaxHeat 도달 시 OverheatForcedCooldown 동안 발사 불가.
// 냉각률은 마지막으로 쏜 무기의 CooldownRate. 연결: UWeaponDataAsset, ALumenCharacter, ALumenEnemyCharacter, BTService_LumenPerception(bOverheated), ACombatRoomVolume(SetHeatFraction)

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HeatComponent.generated.h"

class UWeaponDataAsset;
class USoundBase;

UENUM(BlueprintType)
enum class EHeatState : uint8
{
    Ready,       // 정상 사격 가능
    Overheated   // 과열, 강제 쿨타임 중
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHeatChanged, float, NewHeatRatio); // 발열 비율(0~1) 변경 알림 (냉각 중엔 매 틱)
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOverheated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHeatRecovered);

UCLASS(ClassGroup = (Weapon), meta = (BlueprintSpawnableComponent))
class LUMEN_API UHeatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UHeatComponent();

    // ===== 발열 =====
    // 게이지 최대값. 모든 색 무기가 이 게이지 하나를 공유
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Heat")
    float MaxHeat = 100.f;

    // 과열 시 발사 불가 시간(초). 이 동안에도 자연 냉각은 계속됨
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Heat")
    float OverheatForcedCooldown = 2.f;

    // ===== 사운드 =====
    // 과열 진입 / 과열 해제 — 플레이어·적 공용 (적 과열음 = 반격 타이밍 신호)
    // BP의 HeatComponent 디테일에서 지정. 비워두면 무음
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Heat|Sound")
    TObjectPtr<USoundBase> OverheatSound;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Heat|Sound")
    TObjectPtr<USoundBase> OverheatRecoverSound;

    // ===== 이벤트 =====
    UPROPERTY(BlueprintAssignable)
    FOnHeatChanged OnHeatChanged;

    UPROPERTY(BlueprintAssignable)
    FOnOverheated OnOverheated;

    // 강제 쿨타임이 끝나 다시 발사 가능해졌을 때 (ResetHeat로 과열이 풀릴 때도)
    UPROPERTY(BlueprintAssignable)
    FOnHeatRecovered OnHeatRecovered;

    // ===== 조회 / 제어 =====
    // 발사 시 무기 쪽에서 호출. 과열 중이면 false(발사 불가), 아니면 발열을 더하고 true
    // 이번 발사로 MaxHeat에 닿으면 발사는 허용하고 그 직후 과열 진입
    UFUNCTION(BlueprintCallable, Category = "Heat")
    bool TryConsumeHeat(const UWeaponDataAsset* WeaponData);

    UFUNCTION(BlueprintCallable, Category = "Heat")
    bool IsOverheated() const { return State == EHeatState::Overheated; }

    UFUNCTION(BlueprintCallable, Category = "Heat")
    float GetHeatRatio() const { return MaxHeat > 0.f ? CurrentHeat / MaxHeat : 0.f; }

    // 체크포인트 리스폰 시 발열을 완전히 초기화 (과열 중이었으면 OnHeatRecovered도 발생)
    UFUNCTION(BlueprintCallable, Category = "Heat")
    void ResetHeat();

    // 전투룸에 여러 적을 스폰할 때 발열 사이클을 서로 어긋나게 하기 위한 초기값 설정
    // Fraction은 0~1, MaxHeat 대비 비율. 과열 상태로 직접 진입시키지는 않음(Ready 유지)
    UFUNCTION(BlueprintCallable, Category = "Heat")
    void SetHeatFraction(float Fraction);

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    float CurrentHeat = 0.f;
    EHeatState State = EHeatState::Ready;
    float OverheatTimer = 0.f;

    // 마지막으로 쏜 무기. 냉각 시 이 무기의 CooldownRate를 사용 (아직 안 쐈으면 기본 20/초)
    UPROPERTY()
    const UWeaponDataAsset* LastUsedWeapon = nullptr;
};