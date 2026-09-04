#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "LumenGameInstance.generated.h"

class ULumenSaveGame;
class ALumenPlayerState;

UCLASS()
class LUMEN_API ULumenGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	static const FString SaveSlotName;
	static const int32 SaveUserIndex = 0;

	UFUNCTION(BlueprintCallable, Category = "Save")
	bool HasSaveGame() const;

	// 체크포인트를 밟을 때마다 호출 - 디스크에 즉시 기록
	UFUNCTION(BlueprintCallable, Category = "Save")
	void SaveCurrentProgress(ALumenPlayerState* PlayerState);

	// 타이틀에서 Continue 클릭 시 호출 - 로드해서 멤버에 보관 (레벨 전환 이후까지 살아있음)
	UFUNCTION(BlueprintCallable, Category = "Save")
	bool LoadSavedProgress();

	// 레벨 로드 후 PlayerState가 이 값을 가져가서 적용
	UFUNCTION(BlueprintCallable, Category = "Save")
	bool ConsumePendingLoad(FTransform& OutTransform, FName& OutCheckpointID, int32& OutCheckpointOrder);

private:
	UPROPERTY()
	TObjectPtr<ULumenSaveGame> PendingLoadedSave = nullptr;

	bool bHasPendingLoad = false;
};