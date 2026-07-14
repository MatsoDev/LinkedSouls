// Copyright Epic Games, Inc. All Rights Reserved.

#include "LinkedSoulsGameMode.h"
#include "LinkedSoulsGameState.h"
#include "HUD/LinkedSoulsHUD.h"

ALinkedSoulsGameMode::ALinkedSoulsGameMode()
{
	// use the custom game state that owns the SoulEnergyComponent
	GameStateClass = ALinkedSoulsGameState::StaticClass();

	// register the HUD class
	HUDClass = ALinkedSoulsHUD::StaticClass();
}
