// Copyright Epic Games, Inc. All Rights Reserved.

#include "LumenGameMode.h"
#include "LumenCharacter.h"
#include "UObject/ConstructorHelpers.h"

ALumenGameMode::ALumenGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
