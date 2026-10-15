#include "LumenGameInstance.h"
#include "LumenSaveGame.h"
#include "LumenPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundMix.h"
#include "Sound/SoundClass.h"
#include "LumenGameUserSettings.h"

const FString ULumenGameInstance::SaveSlotName = TEXT("LumenSave");

bool ULumenGameInstance::HasSaveGame() const
{
	return UGameplayStatics::DoesSaveGameExist(SaveSlotName, SaveUserIndex);
}

void ULumenGameInstance::SaveCurrentProgress(ALumenPlayerState* PlayerState)
{
	if (!PlayerState)
	{
		return;
	}

	ULumenSaveGame* SaveObject = Cast<ULumenSaveGame>(
		UGameplayStatics::CreateSaveGameObject(ULumenSaveGame::StaticClass()));

	if (!SaveObject)
	{
		return;
	}

	SaveObject->SavedTransform = PlayerState->LastCheckpointTransform;
	SaveObject->SavedCheckpointID = PlayerState->LastCheckpointID;
	SaveObject->SavedCheckpointOrder = PlayerState->LastCheckpointOrder;
	SaveObject->SaveVersion = PlayerState->SaveVersion;

	UGameplayStatics::SaveGameToSlot(SaveObject, SaveSlotName, SaveUserIndex);
}

bool ULumenGameInstance::LoadSavedProgress()
{
	if (!HasSaveGame())
	{
		bHasPendingLoad = false;
		return false;
	}

	PendingLoadedSave = Cast<ULumenSaveGame>(
		UGameplayStatics::LoadGameFromSlot(SaveSlotName, SaveUserIndex));

	// 현재 빌드보다 새 버전의 세이브는 읽지 않음 (필드 구조가 달라졌을 수 있음).
	// 이전 버전 세이브를 변환해야 할 일이 생기면 여기에 버전별 분기 추가
	const int32 SupportedVersion = GetDefault<ULumenSaveGame>()->SaveVersion;
	if (PendingLoadedSave && PendingLoadedSave->SaveVersion > SupportedVersion)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Save] 세이브 버전 %d 는 지원 범위(%d) 밖 — 불러오지 않음"),
			PendingLoadedSave->SaveVersion, SupportedVersion);
		PendingLoadedSave = nullptr;
	}

	bHasPendingLoad = (PendingLoadedSave != nullptr);
	return bHasPendingLoad;
}

bool ULumenGameInstance::ConsumePendingLoad(FTransform& OutTransform, FName& OutCheckpointID, int32& OutCheckpointOrder)
{
	if (!bHasPendingLoad || !PendingLoadedSave)
	{
		return false;
	}

	OutTransform = PendingLoadedSave->SavedTransform;
	OutCheckpointID = PendingLoadedSave->SavedCheckpointID;
	OutCheckpointOrder = PendingLoadedSave->SavedCheckpointOrder;

	// 한 번 소비하면 비움 — 다음 레벨 전환에서 잘못 재적용되는 것 방지
	bHasPendingLoad = false;
	PendingLoadedSave = nullptr;
	return true;
}

void ULumenGameInstance::OnStart()
{
	Super::OnStart();
	ApplyAudioSettings();
}

void ULumenGameInstance::ApplyAudioSettings()
{
	if (!MasterSoundMix) return;

	const ULumenGameUserSettings* Settings = ULumenGameUserSettings::GetLumenSettings();
	const float Master = Settings ? Settings->GetMasterVolume() : 1.f;
	const float MusicVol = Settings ? Settings->GetMusicVolume() : 1.f;
	const float SFXVol = Settings ? Settings->GetSFXVolume() : 1.f;

	// 마스터는 SC_Master에 지정된 기타 소리(UI 등)용.
	// 배경음/효과음 클래스는 부모-자식으로 묶지 않고 마스터를 직접 곱해서 적용 → 이중 적용 방지
	if (MasterSoundClass)
	{
		UGameplayStatics::SetSoundMixClassOverride(this, MasterSoundMix, MasterSoundClass,
			Master, 1.f, 0.f, /*bApplyToChildren=*/false);
	}
	if (MusicSoundClass)
	{
		UGameplayStatics::SetSoundMixClassOverride(this, MasterSoundMix, MusicSoundClass,
			Master * MusicVol, 1.f, 0.f, false);
	}
	if (SFXSoundClass)
	{
		UGameplayStatics::SetSoundMixClassOverride(this, MasterSoundMix, SFXSoundClass,
			Master * SFXVol, 1.f, 0.f, false);
	}
	UGameplayStatics::PushSoundMixModifier(this, MasterSoundMix);
}

