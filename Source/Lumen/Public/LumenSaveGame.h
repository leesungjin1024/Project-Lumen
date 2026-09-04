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

	UPROPERTY(BlueprintReadWrite, Category = "Save")
	int32 SaveVersion = 1;
};