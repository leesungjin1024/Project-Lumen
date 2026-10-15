// ULumenMusicSubsystem — 배경음 관리. 곡 전환 시 크로스페이드. 월드 서브시스템이라 레벨마다 자동 생성.
// 흐름: 레벨 시작 → 기본 곡(SetDefaultMusic) / 전투 시작 → 전투 곡(PlayMusic) / 전멸·사망 리셋 → 기본 곡 복귀
// 연결: ACombatRoomVolume::CombatMusic, 레벨 블루프린트

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LumenMusicSubsystem.generated.h"

class USoundBase;
class UAudioComponent;

UCLASS()
class LUMEN_API ULumenMusicSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// 곡 전환 (같은 곡이 이미 재생 중이면 무시) — 이전 곡 페이드아웃 + 새 곡 페이드인
	UFUNCTION(BlueprintCallable, Category = "Lumen|Audio")
	void PlayMusic(USoundBase* Music, float FadeTime = 1.5f);

	// 이 레벨의 평소 곡 지정 + 바로 재생 (타이틀은 Level Blueprint에서 호출)
	UFUNCTION(BlueprintCallable, Category = "Lumen|Audio")
	void SetDefaultMusic(USoundBase* Music, float FadeTime = 1.5f);

	// 전멸/사망 리셋 등 → 평소 곡으로 복귀
	UFUNCTION(BlueprintCallable, Category = "Lumen|Audio")
	void ReturnToDefaultMusic(float FadeTime = 2.f);

private:
	// 지금 재생 중인 곡의 오디오 컴포넌트 (교체 시 페이드아웃 후 자동 삭제)
	UPROPERTY()
	TObjectPtr<UAudioComponent> CurrentComp;

	UPROPERTY()
	TObjectPtr<USoundBase> CurrentMusic;

	UPROPERTY()
	TObjectPtr<USoundBase> DefaultMusic;
};
