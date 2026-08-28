//// PuzzleRoomVolume.h
// 퍼즐룸: CombatRoomVolume과 대응되는 퍼즐룸 쪽 "방" 개념.
// 실제 클리어 판정은 여전히 PuzzleSensorActor(SensorGroupID)가 하지만,
// "지금 플레이어가 이 방에 있다"를 추적하고 죽었을 때 센서 전체를 되돌리는 역할을 맡음.

#pragma once

#include "CoreMinimal.h"
#include "RoomVolume.h"
#include "PuzzleRoomVolume.generated.h"

class UBoxComponent;
class ALumenDoorActor;
class APuzzleSensorActor;

UCLASS()
class LUMEN_API APuzzleRoomVolume : public ARoomVolume
{
	GENERATED_BODY()

public:
	APuzzleRoomVolume();

	// PuzzleSensorActor들의 SensorGroupID와 동일한 값을 넣어서 매칭 (센서 쪽 클리어 판정용)
	UPROPERTY(EditAnywhere, Category = "PuzzleRoom")
	FName SensorGroupID;

	// 이 방에 속한 센서들 — ResetRoom()은 SensorGroupID 문자열 매칭이 아니라 여기 명시된
	// 센서만 리셋함 (그룹 없는 센서가 여러 방에 있을 때 서로 잘못 리셋되는 문제 방지)
	UPROPERTY(EditAnywhere, Category = "PuzzleRoom")
	TArray<TObjectPtr<APuzzleSensorActor>> Sensors;

	// 이 방의 문 — PuzzleSensorActor 쪽에도 개별로 ExitDoor가 있지만, 리셋 시 닫는 건 여기서 처리
	UPROPERTY(EditAnywhere, Category = "PuzzleRoom")
	TArray<TObjectPtr<ALumenDoorActor>> ExitDoors;

	UPROPERTY(VisibleAnywhere, Category = "PuzzleRoom")
	UBoxComponent* EntryTrigger;

	virtual void ResetRoom() override;

	// 센서가 활성화될 때마다 호출 — 그룹 전체가 다 맞았으면 문을 엶 (문 여는 책임을 방으로 통일)
	void NotifySensorActivated();

	bool AreAllSensorsActivated() const;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnEntryOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	// 방 밖으로 나갔을 때 CurrentRoom을 지워주기 위한 EndOverlap (신규)
	UFUNCTION()
	void OnEntryEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};