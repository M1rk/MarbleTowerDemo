// Copyright Epic Games, Inc. All Rights Reserved.

#include "MarbleTowerDemoGameMode.h"
#include "MarbleTowerDemoCharacter.h"
#include "UObject/ConstructorHelpers.h"

AMarbleTowerDemoGameMode::AMarbleTowerDemoGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}
