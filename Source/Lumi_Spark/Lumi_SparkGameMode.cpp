// Copyright Epic Games, Inc. All Rights Reserved.

#include "Lumi_SparkGameMode.h"
#include "Lumi_SparkCharacter.h"
#include "UObject/ConstructorHelpers.h"

ALumi_SparkGameMode::ALumi_SparkGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
