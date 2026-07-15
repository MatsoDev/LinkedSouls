// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Player/BodyCharacter.h"
#include "Player/SoulCharacter.h"
#include "LinkedSoulsGameMode.generated.h"

/**
 *  Simple GameMode for a third person game
 */
UCLASS(abstract)
class ALinkedSoulsGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	ALinkedSoulsGameMode();

	virtual void BeginPlay() override;

protected:

	virtual void PostLogin(APlayerController* NewPlayer) override;

private:

	void SpawnPlayerPawn(APlayerController* NewPlayer, bool bIsBody);

	UPROPERTY()
	int32 NumInitializedPlayers = 0;
};



