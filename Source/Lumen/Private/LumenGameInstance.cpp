#include "LumenGameInstance.h"
#include "LumenSaveGame.h"
#include "Lumen/LumenPlayerState.h"
#include "Kismet/GameplayStatics.h"

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

	// 실제 필드명이 다르면 아래 4줄만 맞춰서 수정
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

	// 한 번 소비하면 비움 - 다음 레벨 전환에서 잘못 재적용되는 것 방지
	bHasPendingLoad = false;
	PendingLoadedSave = nullptr;
	return true;
}