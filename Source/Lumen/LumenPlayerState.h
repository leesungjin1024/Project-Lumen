// LumenPlayerState.h
// 체크포인트 데이터 보관. 12-3 원칙에 따라 SaveVersion 필드를 미리 심어둠(지금 당장 세이브/로드가 없어도 무방)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "LumenPlayerState.generated.h"

UCLASS()
class LUMEN_API ALumenPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	FTransform LastCheckpointTransform;

	UPROPERTY(BlueprintReadOnly)
	FName LastCheckpointID;

	UPROPERTY(BlueprintReadOnly)
	int32 LastCheckpointOrder = -1;

	// 12-3: 세이브 구조가 나중에 바뀌어도 마이그레이션 분기가 가능하도록 지금부터 심어둠
	UPROPERTY(BlueprintReadOnly)
	int32 SaveVersion = 1;
};
