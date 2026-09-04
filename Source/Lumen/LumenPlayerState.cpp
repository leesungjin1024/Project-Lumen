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
			// 폰 이동은 여기서 하지 않음 - 이 시점엔 Pawn이 아직 없음(GameModeBase 로그인 순서상 확정)
		}
	}
}