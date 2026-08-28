// CheckpointActor.h
// 트리거 볼륨에 플레이어가 겹치면 PlayerState에 현재 체크포인트 위치를 기록

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CheckpointActor.generated.h"

class UBoxComponent;

UCLASS()
class LUMEN_API ACheckpointActor : public AActor
{
	GENERATED_BODY()

public:
	ACheckpointActor();

	UPROPERTY(EditAnywhere, Category = "Checkpoint")
	FName CheckpointID;

	// 진행 순서 — 레벨에 배치한 순서대로 0, 1, 2... 라고 매겨두면 됨 (숫자 자체보다 "더 큰 값 = 더 진행됨"만 중요)
	UPROPERTY(EditAnywhere, Category = "Checkpoint")
	int32 CheckpointOrder = 0;

	UPROPERTY(VisibleAnywhere)
	UBoxComponent* TriggerVolume;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);
};
