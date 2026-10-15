#include "LumenMusicSubsystem.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

void ULumenMusicSubsystem::PlayMusic(USoundBase* Music, float FadeTime)
{
	if (Music == CurrentMusic && CurrentComp && CurrentComp->IsPlaying())
	{
		return;   // 이미 재생 중인 곡 — 처음부터 다시 틀지 않음
	}

	if (CurrentComp)
	{
		CurrentComp->FadeOut(FadeTime, 0.f);   // 다 줄면 자동 정지 + 삭제(bAutoDestroy)
		CurrentComp = nullptr;
	}

	CurrentMusic = Music;
	if (!Music)
	{
		return;
	}

	UAudioComponent* Comp = UGameplayStatics::CreateSound2D(
		GetWorld(), Music, 1.f, 1.f, 0.f, nullptr,
		/*bPersistAcrossLevelTransition=*/false, /*bAutoDestroy=*/true);
	if (!Comp)
	{
		return;
	}

	// UI 사운드로 지정해 일시정지 중에도 배경음이 계속 재생되게 함
	Comp->bIsUISound = true;
	Comp->FadeIn(FadeTime, 1.f);
	CurrentComp = Comp;
}

void ULumenMusicSubsystem::SetDefaultMusic(USoundBase* Music, float FadeTime)
{
	DefaultMusic = Music;
	PlayMusic(Music, FadeTime);
}

void ULumenMusicSubsystem::ReturnToDefaultMusic(float FadeTime)
{
	PlayMusic(DefaultMusic, FadeTime);
}
