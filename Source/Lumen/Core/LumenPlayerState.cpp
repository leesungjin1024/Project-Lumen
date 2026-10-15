#include "LumenPlayerState.h"
#include "LumenGameInstance.h"
#include "GameFramework/Pawn.h"

void ALumenPlayerState::BeginPlay()
{
	Super::BeginPlay();

	if (ULumenGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance<ULumenGameInstance>() : nullptr)
	{
		FTransform LoadedTransform;
		FName LoadedID;
		int32 LoadedOrder;

		if (GI->ConsumePendingLoad(LoadedTransform, LoadedID, LoadedOrder))
		{
			LastCheckpointTransform = LoadedTransform;
			LastCheckpointID = LoadedID;
			LastCheckpointOrder = LoadedOrder;
			// 폰 이동은 여기서 하지 않음 — PlayerState가 Pawn보다 먼저 생성되므로 위치 적용은 Character::BeginPlay에서
		}
	}
}