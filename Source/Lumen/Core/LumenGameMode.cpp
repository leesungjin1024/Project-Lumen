#include "LumenGameMode.h"
#include "LumenCharacter.h"
#include "UObject/ConstructorHelpers.h"

ALumenGameMode::ALumenGameMode()
{
	// 기본 폰을 블루프린트 캐릭터로 지정 (경로의 에셋이 없으면 엔진 기본 폰 사용)
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
