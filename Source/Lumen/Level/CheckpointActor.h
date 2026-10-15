// ACheckpointActor — 플레이어가 트리거에 겹치면 더 진행된 체크포인트일 때만 PlayerState에 위치를 기록하고 즉시 세이브.
// 연결: ALumenPlayerState(LastCheckpoint*), ULumenGameInstance::SaveCurrentProgress, ALumenCharacter(리스폰)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CheckpointActor.generated.h"

class UBoxComponent;
class USoundBase;

UCLASS()
class LUMEN_API ACheckpointActor : public AActor
{
	GENERATED_BODY()

public:
	ACheckpointActor();

	// ===== 체크포인트 =====
	// 체크포인트 식별용 이름 (세이브에 함께 기록)
	UPROPERTY(EditAnywhere, Category = "Checkpoint")
	FName CheckpointID;

	// 진행 순서 — 레벨에 배치한 순서대로 0, 1, 2... 로 매겨두면 됨. "더 큰 값 = 더 진행됨"만 중요하며, 지금 값 이하인 체크포인트는 무시(역행 방지)
	UPROPERTY(EditAnywhere, Category = "Checkpoint")
	int32 CheckpointOrder = 0;

	// ===== 사운드 =====
	// 새 체크포인트 도달 시 (2D). 이미 지난 체크포인트를 다시 밟으면 안 울림. 비워두면 무음
	UPROPERTY(EditAnywhere, Category = "Checkpoint|Sound")
	TObjectPtr<USoundBase> CheckpointSound;

	// ===== 컴포넌트 =====
	// 체크포인트 감지 범위. 이 액터의 트랜스폼이 리스폰 위치/방향이 됨
	UPROPERTY(VisibleAnywhere)
	UBoxComponent* TriggerVolume;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);
};
