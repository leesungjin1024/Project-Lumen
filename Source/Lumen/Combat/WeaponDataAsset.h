// UWeaponDataAsset — 무기 1색(빨강/파랑/초록)의 모든 수치·비주얼·사운드. 플레이어/적/이미터가 같은 DA를 공유.
// 연결: AProjectile, ULumenCombatLibrary, ULumenAimLineComponent, UHeatComponent, ALumenCharacter/ALumenEnemyCharacter, ALightEmitterActor
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WeaponDataAsset.generated.h"

class ALumenBeamSegment;
class USoundBase;
class UStaticMesh;

UENUM(BlueprintType)
enum class EWeaponFireMode : uint8
{
    Projectile UMETA(DisplayName = "Projectile"),   // 빨강(자동연사), 파랑(펠릿)
    LineTrace  UMETA(DisplayName = "LineTrace")     // 초록(관통)
};

// 센서 판정용 색. 에셋에 저장하지 않고 DeriveWeaponColor()로 FireMode/PelletCount에서 계산
UENUM(BlueprintType)
enum class ELumenWeaponColor : uint8
{
    Red,
    Blue,
    Green
};

UCLASS(BlueprintType)
class LUMEN_API UWeaponDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    // ===== 식별/표시 =====
    // 색은 가안 — 정식 확정 전까지 _TEMP로 표시
    UPROPERTY(EditDefaultsOnly, Category = "Identity")
    FName WeaponName; // "Red_TEMP" 등

    UPROPERTY(EditDefaultsOnly, Category = "Identity")
    FLinearColor WeaponColor_TEMP; // 발사체/레이저/조준선 공통 색

    // ===== 발사 =====
    UPROPERTY(EditDefaultsOnly, Category = "Fire")
    EWeaponFireMode FireMode = EWeaponFireMode::Projectile;

    // 고정 데미지 (거리 감쇠 없음)
    UPROPERTY(EditDefaultsOnly, Category = "Fire")
    float Damage = 1.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Fire", meta = (EditCondition = "FireMode == EWeaponFireMode::Projectile"))
    float ProjectileSpeed = 3000.f;

    UPROPERTY(EditDefaultsOnly, Category = "Fire")
    int32 MaxReflections = 5; // 초록은 관통 횟수도 함께 카운트

    // 자동 연사 무기(빨강)에만 의미 있는 값. 반자동 무기는 무시됨.
    UPROPERTY(EditDefaultsOnly, Category = "Fire")
    float FireRate = 8.f; // 초당 발사 횟수

    // 발사체(빨강/파랑) 사거리 — 이 거리에 도달하면 경계에서 멈춘 뒤 페이드아웃 소멸
    // 조준선 길이도 이 값을 그대로 씀. 맵 거울 반사 시 리셋, 방패 반사 시 누적 유지
    // 변수명은 FalloffEndRange 그대로 유지 — 바꾸면 DA에 입력해둔 값이 날아감. 에디터엔 "Max Range"로 표시
    UPROPERTY(EditDefaultsOnly, Category = "Fire|Range", meta = (DisplayName = "Max Range", EditCondition = "FireMode == EWeaponFireMode::Projectile", ClampMin = "0.0"))
    float FalloffEndRange = 1500.f;

    // ===== 파랑(샷건) 전용: 펠릿 =====
    // 한 번 발사에 나가는 펠릿 수. 같은 발사의 펠릿은 대상당 1회만 데미지(VolleyId)
    UPROPERTY(EditDefaultsOnly, Category = "Fire|Pellet", meta = (EditCondition = "FireMode == EWeaponFireMode::Projectile"))
    int32 PelletCount = 1; // 빨강=1, 파랑=5~8

    // 펠릿 전체 퍼짐 각도(도) — 이 범위에 균등 분산
    UPROPERTY(EditDefaultsOnly, Category = "Fire|Pellet", meta = (EditCondition = "PelletCount > 1"))
    float SpreadAngleDegrees = 0.f;

    UPROPERTY(EditDefaultsOnly, Category = "Fire|Pellet", meta = (EditCondition = "PelletCount > 1"))
    float PelletJitterDegrees = 0.0f; // 균등 분산 위에 얹는 미세 랜덤값

    // ===== 초록(라인트레이스) 전용 =====
    // 켜면 데미지 대상(UHealthComponent 보유)을 맞혀도 멈추지 않고 관통
    UPROPERTY(EditDefaultsOnly, Category = "Fire|LineTrace", meta = (EditCondition = "FireMode == EWeaponFireMode::LineTrace"))
    bool bPierces = true;

    UPROPERTY(EditDefaultsOnly, Category = "Fire|LineTrace", meta = (EditCondition = "FireMode == EWeaponFireMode::LineTrace"))
    float TraceRange = 5000.f; // 레이저 사거리 (조준선 길이). 맵 거울 반사 시 리셋

    // 레이저 시각화 — 세그먼트마다 이 클래스를 스폰 (BP_BeamSegment 지정).
    // DA에 두는 이유: 플레이어/적/이미터 등 이 무기를 쏘는 모든 주체가 코드 수정 없이 같은 비주얼을 씀
    UPROPERTY(EditDefaultsOnly, Category = "Fire|LineTrace", meta = (EditCondition = "FireMode == EWeaponFireMode::LineTrace"))
    TSubclassOf<ALumenBeamSegment> BeamSegmentClass;

    // ===== 발열 =====
    // 게이지는 캐릭터의 UHeatComponent 하나를 공유 — 여기엔 색별 발열량·냉각률만
    UPROPERTY(EditDefaultsOnly, Category = "Heat")
    float HeatPerShot = 10.f; // 파랑은 크게, 초록은 더 크게, 빨강은 작게

    UPROPERTY(EditDefaultsOnly, Category = "Heat")
    float CooldownRate = 20.f; // 초당 식는 양 — 마지막으로 쏜 무기 값 사용 (빨강 큼, 초록 작음)

    // ===== 조준선/텔레그래프 =====
    // 적의 공격 예고 시간(초) — BT 대기 시간과 조준선 번쩍 타이밍이 이 값을 함께 씀
    UPROPERTY(EditDefaultsOnly, Category = "Telegraph")
    float TelegraphDuration = 0.75f;

    UPROPERTY(EditDefaultsOnly, Category = "Telegraph")
    float TelegraphWidthDegrees = 5.f; // 조준선/텔레그래프 부채꼴 전체 폭(도). 빨강 5 / 파랑 40 / 초록 1

    // ===== 사운드 =====
    // DA에 두므로 색마다 다른 소리, 쏘는 주체(플레이어/적)와 무관하게 동일. 비워두면 무음
    UPROPERTY(EditDefaultsOnly, Category = "Sound")
    TObjectPtr<USoundBase> FireSound;        // 발사 1회당 1번 (파랑 펠릿 수와 무관)

    UPROPERTY(EditDefaultsOnly, Category = "Sound")
    TObjectPtr<USoundBase> ReflectSound;     // 거울/방패에서 반사될 때

    UPROPERTY(EditDefaultsOnly, Category = "Sound")
    TObjectPtr<USoundBase> TelegraphSound;   // 적 공격 예고 시작 시

    // ===== 비주얼 =====
    // 손에 드는 총 메시 — 색 전환 시 교체, 적도 같은 DA로 자동 적용
    UPROPERTY(EditDefaultsOnly, Category = "Visual")
    TObjectPtr<UStaticMesh> WeaponMesh;

    // 손 소켓 기준 위치/회전/크기 보정 — 총마다 피벗(원점) 위치가 달라서 각각 맞춰야 함
    // 총 메시에 Muzzle 소켓이 있으면 그 위치가 발사/조준선 시작점이므로, 이 보정도 총구 위치에 영향을 줌
    UPROPERTY(EditDefaultsOnly, Category = "Visual")
    FTransform WeaponMeshOffset;
};

// 무기 데이터에서 색 판별: LineTrace=초록, 펠릿 2개 이상=파랑, 그 외=빨강
inline ELumenWeaponColor DeriveWeaponColor(const UWeaponDataAsset* Data)
{
    if (!Data)
    {
        return ELumenWeaponColor::Red;
    }
    if (Data->FireMode == EWeaponFireMode::LineTrace)
    {
        return ELumenWeaponColor::Green;
    }
    return (Data->PelletCount > 1) ? ELumenWeaponColor::Blue : ELumenWeaponColor::Red;
}