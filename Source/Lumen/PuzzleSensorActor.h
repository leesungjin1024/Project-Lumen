// PuzzleSensorActor.h
// 퍼즐룸 클리어 판정용 센서. 특정 색 빛(발사체/레이저)에 맞으면 활성화.
//
// ELumenWeaponColor는 LumenCharacter.h에서 WeaponDataAsset.h로 이동하기로 결정됨
// (ELumenWeaponColor_Relocate_Patch.md 참고 — 무기 관련 코드가 캐릭터 헤더 전체에
//  의존하지 않도록 하기 위함). 아래 include는 이동 후 기준.
//
// 설계 수정 (재검토 1): IReflective를 구현하면 AProjectile::HandleHit()이 반사 대상으로
// 판단해 튕겨내 버려서 "맞으면 활성화"가 안 됨 → 제거. 대신 AProjectile::HandleHit() /
// FireReflectiveLineTrace()에서 직접 NotifyHitByLight()를 호출하는 훅 방식으로 교체.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponDataAsset.h" // ELumenWeaponColor 정의 위치 (이동 후)
#include "PuzzleSensorActor.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class APuzzleRoomVolume;

UCLASS()
class LUMEN_API APuzzleSensorActor : public AActor
{
	GENERATED_BODY()

public:
	APuzzleSensorActor();

	// 이 센서를 활성화시키는 색
	UPROPERTY(EditAnywhere, Category = "PuzzleSensor")
	ELumenWeaponColor RequiredColor = ELumenWeaponColor::Red;

	// 이 센서가 속한 방 — 활성화되면 여기에 알려서 방이 "전체 클리어" 판정 및 문 개방을 처리
	// (예전엔 센서마다 개별 ExitDoor를 가지고 있어서, 마지막에 활성화된 센서의 문만 열리는
	//  버그가 있었음 — 문 여는 책임을 전부 방으로 옮겨서 통일)
	UPROPERTY(EditAnywhere, Category = "PuzzleSensor")
	TObjectPtr<APuzzleRoomVolume> OwningRoom;

	// AProjectile::HandleHit() / FireReflectiveLineTrace()에서 이 센서에 명중했을 때 호출
	UFUNCTION(BlueprintCallable)
	void NotifyHitByLight(ELumenWeaponColor HitColor);

	// PuzzleRoomVolume::ResetRoom()에서 죽음으로 인한 리셋 시 호출 — 다시 안 맞은 상태로 되돌림
	UFUNCTION(BlueprintCallable)
	void Deactivate() { bActivated = false; }

	bool IsActivated() const { return bActivated; }

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* SensorMesh;

	UPROPERTY(VisibleAnywhere)
	UBoxComponent* SensorCollision;

protected:
	bool bActivated = false;

	void Activate();
};