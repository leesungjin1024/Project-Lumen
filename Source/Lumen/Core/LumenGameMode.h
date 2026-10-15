// ALumenGameMode — 기본 게임 모드. 생성자에서 기본 폰을 BP_ThirdPersonCharacter 블루프린트로 지정.
// 연결: ALumenCharacter

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LumenGameMode.generated.h"

UCLASS(minimalapi)
class ALumenGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ALumenGameMode();
};



