//// ALumenKillZVolume — 레벨 최하단에 배치하는 낙사 볼륨. 플레이어가 겹치면 데미지 판정 없이 즉시 체크포인트 리스폰.
// 연결: ALumenCharacter::HandleFellOutOfWorld
// 이름 주의: 엔진에 AKillZVolume(KillZVolume.h)이 이미 있고 UHT는 헤더 파일명이 고유해야 하므로 Lumen 접두어 사용.
//   엔진 AKillZVolume은 파괴/FellOutOfWorld 처리만 하고 체크포인트 리스폰이 없어 재사용하지 않음.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LumenKillZVolume.generated.h"

class UBoxComponent;

UCLASS()
class LUMEN_API ALumenKillZVolume : public AActor
{
	GENERATED_BODY()

public:
	ALumenKillZVolume();

	// 낙사 감지 범위 (레벨 바닥 아래 전체를 덮도록 배치)
	UPROPERTY(VisibleAnywhere)
	UBoxComponent* TriggerVolume;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);
};