// APuzzleRoomVolume — 퍼즐룸. 센서가 켜질 때마다 조건을 검사해 문을 열고, 미클리어 상태에서 사망하면 진행을 되돌림.
// 문 여는 방식 두 가지 (같이 써도 됨)
//   1) Sensors + ExitDoors : Sensors 전부 활성화 → ExitDoors 개방 (AND형)
//   2) DoorRules           : 규칙마다 "필요한 센서들 → 열 문들" (특정 센서 → 특정 문)
//                            bRequirePreviousRule 체크 시 앞 규칙 완료 후에만 활성화 (순서형)
// 센서의 OwningRoom은 BeginPlay에서 Sensors + 모든 규칙의 센서 기준으로 자동 연결. 거울 초기화는 APuzzleResetActor가 ResetMirrors()로 요청.
// 연결: APuzzleSensorActor, ALumenDoorActor(ExitDoors/DoorRules), AMirrorActor, APuzzleResetActor

#pragma once

#include "CoreMinimal.h"
#include "RoomVolume.h"
#include "MirrorActor.h"
#include "PuzzleRoomVolume.generated.h"

class UBoxComponent;
class ALumenDoorActor;
class APuzzleSensorActor;
class AMirrorActor;
class USoundBase;

// 규칙 하나 = "이 센서들이 전부 켜지면 → 이 문들을 연다"
USTRUCT(BlueprintType)
struct FPuzzleDoorRule
{
	GENERATED_BODY()

	// 로그 구분용 이름 (예: "1단계 빨강") — 비워도 됨
	UPROPERTY(EditAnywhere, Category = "Rule")
	FName RuleName;

	// 이 센서들이 전부 켜지면 규칙 완료
	UPROPERTY(EditAnywhere, Category = "Rule")
	TArray<TObjectPtr<APuzzleSensorActor>> RequiredSensors;

	// 규칙 완료 시 열리는 문
	UPROPERTY(EditAnywhere, Category = "Rule")
	TArray<TObjectPtr<ALumenDoorActor>> DoorsToOpen;

	// 체크하면 바로 앞 규칙이 완료된 뒤에만 활성화 (순서형).
	// 활성화 전엔 이 규칙의 센서가 빛에 맞아도 켜지지 않음. 첫 번째 규칙에선 무시됨
	UPROPERTY(EditAnywhere, Category = "Rule")
	bool bRequirePreviousRule = false;

	// 런타임 상태 (저장/편집 안 함)
	bool bCompleted = false;
};

UCLASS()
class LUMEN_API APuzzleRoomVolume : public ARoomVolume
{
	GENERATED_BODY()

public:
	APuzzleRoomVolume();

	// ===== 퍼즐 구성 (AND형) =====
	// 이 센서들이 전부 켜지면 ExitDoors가 열림. 규칙(DoorRules)만 쓸 거면 비워도 됨.
	// 넣기만 하면 센서의 OwningRoom은 BeginPlay에서 자동 연결됨
	UPROPERTY(EditAnywhere, Category = "PuzzleRoom")
	TArray<TObjectPtr<APuzzleSensorActor>> Sensors;

	// ===== 문 (AND형) =====
	// Sensors 전부 활성화 시 열리고, 사망 리셋 시 닫히는 문
	UPROPERTY(EditAnywhere, Category = "PuzzleRoom")
	TArray<TObjectPtr<ALumenDoorActor>> ExitDoors;

	// ===== 규칙형 =====
	// 센서 조건별로 특정 문 열기 / 순서형 퍼즐. 규칙에 넣은 센서도 OwningRoom 자동 연결됨
	UPROPERTY(EditAnywhere, Category = "PuzzleRoom")
	TArray<FPuzzleDoorRule> DoorRules;

	// ===== 사운드 =====
	// 퍼즐 전체 클리어(사용한 방식 전부 완료) 징글 — 2D 재생. 비워두면 무음
	UPROPERTY(EditAnywhere, Category = "PuzzleRoom|Sound")
	TObjectPtr<USoundBase> ClearSound;

	// 규칙 하나가 완료될 때(전체 클리어 제외) — 2D 재생. 비워두면 무음
	UPROPERTY(EditAnywhere, Category = "PuzzleRoom|Sound")
	TObjectPtr<USoundBase> RuleCompleteSound;

	// ===== 진입 감지 =====
	// 방 전체를 덮는 진입 감지 트리거. 이 범위가 방 카메라/마스킹 범위로도 쓰임
	UPROPERTY(VisibleAnywhere, Category = "PuzzleRoom")
	UBoxComponent* EntryTrigger;

	// ===== 퍼즐 진행 =====
	// 사망 시 호출. 이미 클리어한 방은 리셋하지 않음. 센서/규칙을 되돌리고 열렸던 문을 닫음
	virtual void ResetRoom() override;

	// 센서가 활성화될 때마다 호출 — AND형/규칙형 모두 검사해 조건을 만족한 문만 엶 (문 여는 책임은 방이 가짐)
	void NotifySensorActivated();

	// 센서가 지금 켜질 수 있는지 — 순서형에서 아직 차례가 아닌 센서는 false
	bool CanSensorActivate(const APuzzleSensorActor* Sensor) const;

	// 이 방에 속한 센서인지 (Sensors 또는 아무 규칙에나 포함)
	bool ContainsSensor(const APuzzleSensorActor* Sensor) const;

	// [AND형] 유효한 센서가 1개 이상이고 모두 활성화됐는지. 빈 칸(None)은 건너뜀
	bool AreAllSensorsActivated() const;

	// 퍼즐 전체 완료 — 사용하는 방식(AND형/규칙형) 전부 끝났을 때 true.
	// 리셋 오브젝트 표시 여부, 사망 리셋 생략 판단에 사용
	bool IsPuzzleSolved() const;

	// Mirrors의 회전을 초기값으로 되돌림 (APuzzleResetActor가 호출)
	void ResetMirrors();


protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnEntryOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	// 방 밖으로 나갔을 때 카메라 해제 + CurrentRoom을 지워주기 위한 EndOverlap
	UFUNCTION()
	void OnEntryEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// 퍼즐 리셋 오브젝트로 회전을 초기화할 거울들
	UPROPERTY(EditInstanceOnly, Category = "Puzzle")
	TArray<TObjectPtr<AMirrorActor>> Mirrors;

private:
	// 규칙 Index가 지금 잠겨 있는지 (순서형에서 앞 규칙 미완료)
	bool IsRuleLocked(int32 Index) const;

	// Sensors + 모든 규칙의 센서 (중복 제거, None 제외)
	void GatherAllSensors(TArray<APuzzleSensorActor*>& OutSensors) const;

	FString GetRuleLabel(int32 Index) const;

	// ClearSound를 한 번만 재생하기 위함
	bool bSolvedNotified = false;
};
