// APuzzleSensorActor — 퍼즐룸 클리어 판정용 센서. RequiredColor 빛(발사체/라인트레이스)에 맞으면 활성화되고 방에 알림.
// 반사되면 안 되므로 IReflective 미구현·Reflective 채널 미사용 → 발사체/라인트레이스의 "정지 분기"에서 NotifyHitByLight() 직접 호출.
// 연결: APuzzleRoomVolume(OwningRoom 자동 연결), AProjectile::HandleHit, ULumenCombatLibrary::FireReflectiveLineTrace

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponDataAsset.h" // ELumenWeaponColor
#include "PuzzleSensorActor.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class APuzzleRoomVolume;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class USoundBase;

UCLASS()
class LUMEN_API APuzzleSensorActor : public AActor
{
	GENERATED_BODY()

public:
	APuzzleSensorActor();

	// ===== 퍼즐 =====
	// 이 센서를 활성화시키는 색
	UPROPERTY(EditAnywhere, Category = "PuzzleSensor")
	ELumenWeaponColor RequiredColor = ELumenWeaponColor::Red;

	// 이 센서가 속한 방. 비워둬도 됨 — 방의 Sensors 배열에 넣으면 방이 BeginPlay에서 자동으로 채움
	// (센서마다 손으로 지정하면 빠뜨리기 쉬워 "색은 맞았는데 문이 안 열림"의 원인이 됨)
	UPROPERTY(EditAnywhere, Category = "PuzzleSensor")
	TObjectPtr<APuzzleRoomVolume> OwningRoom;

	// ===== 비주얼 =====
	// 활성화됐을 때 SensorMesh에 씌울 머티리얼 (M_Projectile 권장 — "Color" 파라미터에 센서 색을 넣어 발광)
	// 비워두면 시각 변화 없음 (로그로만 확인 가능)
	UPROPERTY(EditAnywhere, Category = "PuzzleSensor")
	TObjectPtr<UMaterialInterface> ActivatedMaterial;

	// ===== 사운드 =====
	// 맞는 색으로 활성화됐을 때. 비워두면 무음
	UPROPERTY(EditAnywhere, Category = "PuzzleSensor|Sound")
	TObjectPtr<USoundBase> ActivateSound;

	// 다른 색에 맞았을 때 (짧은 거부음 권장). 비워두면 무음
	UPROPERTY(EditAnywhere, Category = "PuzzleSensor|Sound")
	TObjectPtr<USoundBase> WrongColorSound;

	// ===== 상태 =====
	// AProjectile::HandleHit() / FireReflectiveLineTrace()에서 이 센서에 명중했을 때 호출. 이미 활성화면 무시
	UFUNCTION(BlueprintCallable)
	void NotifyHitByLight(ELumenWeaponColor HitColor);

	// APuzzleRoomVolume::ResetRoom()에서 사망 리셋 시 호출 — 원래 머티리얼로 되돌리고 비활성 상태로
	UFUNCTION(BlueprintCallable)
	void Deactivate();

	bool IsActivated() const { return bActivated; }

	// ===== 컴포넌트 =====
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* SensorMesh;

	// 발사체/라인트레이스 명중 판정용 (ProjectileTrace만 Block)
	UPROPERTY(VisibleAnywhere)
	UBoxComponent* SensorCollision;

protected:
	virtual void BeginPlay() override;

	bool bActivated = false;

	void Activate();

private:
	// 비활성 상태로 되돌릴 때 복원할 원래 머티리얼
	UPROPERTY()
	TObjectPtr<UMaterialInterface> OriginalMaterial;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> ActivatedMID;

	// 파랑 펠릿 여러 발이 동시에 맞아도 거부음은 한 번만
	double LastWrongColorSoundTime = -1000.0;
};