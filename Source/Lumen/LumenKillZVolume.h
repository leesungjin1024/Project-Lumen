//// LumenKillZVolume.h
// 레벨 최하단에 배치. 플레이어가 겹치면 데미지 파이프라인을 타지 않고 즉시 리스폰 처리.
//
// 이름 관련 주의: UE5 엔진에 이미 내장 클래스 AKillZVolume이 존재함
// (Engine/Source/Runtime/Engine/Classes/GameFramework/KillZVolume.h).
// UHT(Unreal Header Tool)는 프로젝트 전체에서 헤더 파일명이 고유해야 하므로,
// 같은 이름(KillZVolume.h)으로 새 헤더를 만들면 "Two headers with the same name"
// 빌드 에러가 발생함 — 그래서 클래스/파일명을 ALumenKillZVolume / LumenKillZVolume.h로 변경.
// (참고: 엔진 내장 AKillZVolume은 겹친 액터를 그냥 파괴/FellOutOfWorld 처리만 하고
//  체크포인트 리스폰 로직이 없어서, 그대로 재사용하기보다 별도 클래스로 두는 게 맞음)

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

	UPROPERTY(VisibleAnywhere)
	UBoxComponent* TriggerVolume;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);
};