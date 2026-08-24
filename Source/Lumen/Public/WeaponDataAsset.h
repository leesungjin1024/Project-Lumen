// WeaponDataAsset.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WeaponDataAsset.generated.h"

UENUM(BlueprintType)
enum class EWeaponFireMode : uint8
{
    Projectile UMETA(DisplayName = "Projectile"),   // 빨강, 파랑(펠릿)
    LineTrace  UMETA(DisplayName = "LineTrace")     // 초록
};

UCLASS(BlueprintType)
class LUMEN_API UWeaponDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    // --- 식별/표시 (5-5-4, 가안 색상 — 정식 확정 전까지 _TEMP 명시) ---
    UPROPERTY(EditDefaultsOnly, Category = "Identity")
    FName WeaponName; // "Red_TEMP" 등

    UPROPERTY(EditDefaultsOnly, Category = "Identity")
    FLinearColor WeaponColor_TEMP;

    // --- 발사 방식 (5-1) ---
    UPROPERTY(EditDefaultsOnly, Category = "Fire")
    EWeaponFireMode FireMode = EWeaponFireMode::Projectile;

    UPROPERTY(EditDefaultsOnly, Category = "Fire")
    float Damage = 10.f;

    UPROPERTY(EditDefaultsOnly, Category = "Fire", meta = (EditCondition = "FireMode == EWeaponFireMode::Projectile"))
    float ProjectileSpeed = 3000.f;

    UPROPERTY(EditDefaultsOnly, Category = "Fire")
    int32 MaxReflections = 5; // 2주차엔 AProjectile에 하드코딩 → 여기로 이관

    // 자동 연사 무기(빨강)에만 의미 있는 값. 반자동 무기는 무시됨.
    UPROPERTY(EditDefaultsOnly, Category = "Fire")
    float FireRate = 8.f; // 초당 발사 횟수

    // 거리 기반 데미지 감쇄 (5-2-1: 빨강/파랑 공통 — 빛이 약해지다 사라지는 특성 반영)
    UPROPERTY(EditDefaultsOnly, Category = "Fire|Falloff", meta = (EditCondition = "FireMode == EWeaponFireMode::Projectile"))
    float FalloffStartRange = 500.f; // 이 거리까지는 100% 데미지

    UPROPERTY(EditDefaultsOnly, Category = "Fire|Falloff", meta = (EditCondition = "FireMode == EWeaponFireMode::Projectile"))
    float FalloffEndRange = 1500.f; // 이 거리부터는 MinDamageMultiplier로 고정 — 조준선 길이도 이 값을 그대로 씀
    
    UPROPERTY(EditDefaultsOnly, Category = "Fire|Falloff", meta = (EditCondition = "FireMode == EWeaponFireMode::Projectile", ClampMin = "0.0", ClampMax = "1.0"))
    float MinDamageMultiplier = 0.2f; // 최대 감쇄 시 남는 데미지 비율 (가안)

    // --- 파랑(샷건) 전용: 펠릿 (5-1) ---
    UPROPERTY(EditDefaultsOnly, Category = "Fire|Pellet", meta = (EditCondition = "FireMode == EWeaponFireMode::Projectile"))
    int32 PelletCount = 1; // 빨강=1, 파랑=5~8

    UPROPERTY(EditDefaultsOnly, Category = "Fire|Pellet", meta = (EditCondition = "PelletCount > 1"))
    float SpreadAngleDegrees = 0.f;

    UPROPERTY(EditDefaultsOnly, Category = "Fire|Pellet", meta = (EditCondition = "PelletCount > 1"))
    float PelletJitterDegrees = 1.5f; // 균등 분산 위에 얹는 미세 랜덤값

    // --- 초록(스나이퍼) 전용: 관통 (5-1) ---
    UPROPERTY(EditDefaultsOnly, Category = "Fire|LineTrace", meta = (EditCondition = "FireMode == EWeaponFireMode::LineTrace"))
    bool bPierces = true;

    UPROPERTY(EditDefaultsOnly, Category = "Fire|LineTrace", meta = (EditCondition = "FireMode == EWeaponFireMode::LineTrace"))
    float TraceRange = 5000.f;

    // --- 발열 시스템 (5-1-1, 색별 차등 / 게이지는 공유이므로 여기엔 소모율·냉각률만) ---
    UPROPERTY(EditDefaultsOnly, Category = "Heat")
    float HeatPerShot = 10.f; // 파랑은 크게, 초록은 더 크게, 빨강은 작게

    UPROPERTY(EditDefaultsOnly, Category = "Heat")
    float CooldownRate = 20.f; // 초당 식는 양 (빨강 큼, 초록 작음)

    // --- 조준선/텔레그래프 (5-5, 적 AI에도 재사용) ---
    UPROPERTY(EditDefaultsOnly, Category = "Telegraph")
    float TelegraphDuration = 0.75f;

    UPROPERTY(EditDefaultsOnly, Category = "Telegraph")
    float TelegraphWidthDegrees = 5.f; // 빨강 5 / 파랑 40 / 초록 1 (5-5 표)
};