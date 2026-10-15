// AProjectile — 빨강/파랑 발사체. 매 틱 스윕(ProjectileTrace)으로 이동·판정하며, 거울/방패에서 반사되고 Max Range 도달 시 페이드 소멸.
// 연결: UWeaponDataAsset(InitFromWeaponData), IReflective/AShieldActor, APuzzleSensorActor, UHealthComponent(VolleyId)
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponDataAsset.h"
#include "Projectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UNiagaraSystem;
class UWeaponDataAsset;
class UMaterialInstanceDynamic;
class USoundBase;
extern TAutoConsoleVariable<bool> CVarShowReflectionDebug;

UCLASS()
class LUMEN_API AProjectile : public AActor
{
	GENERATED_BODY()

public:
	AProjectile();

	virtual void Tick(float DeltaTime) override;

	// 방향만 받아 현재 Speed로 속도를 설정하고 메시를 진행 방향으로 회전
	void InitVelocity(const FVector& Direction);

	// 무기 데이터(데미지/반사 횟수/사거리/속도/색/반사음)를 적용하고 발사 방향 설정. 스폰 직후 호출
	UFUNCTION(BlueprintCallable)
	void InitFromWeaponData(UWeaponDataAsset* WeaponData, const FVector& Direction);

	// InitFromWeaponData 직후 호출 — 방별 사거리 배율 적용 (퍼즐룸 등). 무제한(0)이면 그대로
	void ScaleMaxRange(float Multiplier)
	{
		if (MaxRange > 0.f) MaxRange *= FMath::Max(Multiplier, 0.01f);
		BaseMaxRange = MaxRange;
	}

	// 같은 발사(샷건 1회)에서 나간 펠릿 묶음 번호. 0 = 묶음 아님
	// 같은 번호의 펠릿은 대상당 1회만 데미지 (UHealthComponent::TryConsumeVolley)
	UPROPERTY()
	int32 VolleyId = 0;

	// 새 묶음 번호 발급 (플레이어/적 공용)
	static int32 NewVolleyId()
	{
		static int32 Counter = 0;
		return ++Counter;
	}

protected:
	virtual void BeginPlay() override;

	// 반사체(ECC_Reflective)면 반사, 그 외엔 센서 알림 또는 데미지 적용 후 파괴
	void HandleHit(const FHitResult& Hit);

	// 사거리 끝/반사 한도/뒷면에서 멈출 때: 그 자리에서 작아지며 사라짐
	void BeginFadeOut();

	// 진행 방향으로 액터 회전 — 진행축 기준 회전 대칭 메시라 반사 후에도 같은 실루엣 유지
	void FaceVelocity();

	// WeaponColor_TEMP를 머티리얼 "Color" 파라미터로 전달
	void ApplyWeaponColor(const FLinearColor& Color);

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> MeshMID;

	// ===== 컴포넌트 =====
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> CollisionComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComp;

	// ===== 이동/반사 =====
	// 기본값은 InitFromWeaponData()가 ProjectileSpeed로 덮어씀
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float Speed = 2000.f;

	// 기본값은 InitFromWeaponData()가 항상 덮어씀
	UPROPERTY(EditDefaultsOnly, Category = "Reflection")
	int32 MaxReflections = 5;

	// ===== VFX =====
	// 반사 지점 이펙트. 비반사 대상 명중 시에도 임팩트 이펙트로 재사용
	UPROPERTY(EditDefaultsOnly, Category = "VFX")
	TObjectPtr<UNiagaraSystem> ReflectionSparkFX;

	// 사거리 끝에서 사라질 때 재생 (선택 — 비워두면 스케일 페이드만)
	UPROPERTY(EditDefaultsOnly, Category = "VFX")
	TObjectPtr<UNiagaraSystem> RangeEndFX;

	// 사라지는 데 걸리는 시간 — 갑자기 사라지면 버그처럼 보이므로 짧게라도 유지
	UPROPERTY(EditDefaultsOnly, Category = "VFX", meta = (ClampMin = "0.01"))
	float FadeOutDuration = 0.15f;

	// ===== 런타임 상태 =====
	FVector Velocity = FVector::ZeroVector;
	int32 CurrentReflections = 0;
	bool bStopped = false;

	// 고정 데미지 (거리 감쇠 없음). InitFromWeaponData에서 설정
	UPROPERTY()
	float Damage = 10.f;

	// 사거리 = WeaponData->FalloffEndRange (조준선 길이와 동일). 0 이하면 무제한
	float MaxRange = 0.f;

	// 발사 시점의 사거리 — 거울 반사 시 "이 값 × 거울 배율"로 MaxRange를 다시 채우는 기준
	float BaseMaxRange = 0.f;

	// 이동 거리 누적 — 방패 반사는 그대로 이어가고, 맵 거울 반사는 0으로 리셋
	UPROPERTY()
	float DistanceTraveled = 0.f;

	// 페이드아웃 상태
	bool bFadingOut = false;
	float FadeElapsed = 0.f;
	FVector InitialMeshScale = FVector::OneVector;

	// 방금 반사한 액터 — 모서리에서 같은 대상과 매 틱 재충돌하지 않도록 IgnoreLastActorTimer 동안 무시
	UPROPERTY()
	TWeakObjectPtr<AActor> LastReflectedActor;

	float IgnoreLastActorTimer = 0.f;

	// ApplyDamage에 넘길 Instigator. 방패 반사 시 방패 주인의 Controller로 교체 (DamageCauser는 발사체 유지)
	UPROPERTY() AController* DamageInstigatorController;

	// 센서(APuzzleSensorActor) 색 판정용 — 무기 데이터에서 DeriveWeaponColor로 계산
	UPROPERTY()
	ELumenWeaponColor ProjectileColor = ELumenWeaponColor::Red;

	// 반사음 — InitFromWeaponData에서 DA 값을 받아둠
	UPROPERTY()
	TObjectPtr<USoundBase> ReflectSound;

};