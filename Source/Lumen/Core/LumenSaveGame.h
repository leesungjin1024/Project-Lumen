// ULumenSaveGame — 진행 세이브 데이터(마지막 체크포인트). 설정값은 여기 말고 ULumenGameUserSettings에 저장.
// 연결: ULumenGameInstance(슬롯 "LumenSave" 저장/로드), ALumenPlayerState
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "LumenSaveGame.generated.h"

UCLASS()
class LUMEN_API ULumenSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	FTransform SavedTransform;

	UPROPERTY(BlueprintReadWrite, Category = "Save")
	FName SavedCheckpointID;

	UPROPERTY(BlueprintReadWrite, Category = "Save")
	int32 SavedCheckpointOrder = 0;

	// 세이브 구조 버전 (구조가 바뀌면 마이그레이션 분기용)
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	int32 SaveVersion = 1;
};