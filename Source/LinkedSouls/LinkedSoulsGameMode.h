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

	// -- Soul Energy (shared pool) ------------------------------------------

	/** Consumes shared SoulEnergy. Returns false if insufficient. */
	UFUNCTION(BlueprintCallable, Category = "LinkedSouls|SoulEnergy")
	bool ConsumeSoulEnergy(float Amount);

	/** Restores shared SoulEnergy (capped at MaxEnergy). */
	UFUNCTION(BlueprintCallable, Category = "LinkedSouls|SoulEnergy")
	void RestoreSoulEnergy(float Amount);

	/** @returns the current shared SoulEnergy value. */
	UFUNCTION(BlueprintPure, Category = "LinkedSouls|SoulEnergy")
	float GetSoulEnergy() const;

	/** Resolves the shared SoulEnergyComponent from GameState. */
	class USoulEnergyComponent* GetSoulEnergyComponent() const;

protected:

	virtual void PostLogin(APlayerController* NewPlayer) override;

	/** Links Body and Soul partners after both have spawned. */
	void LinkPartners();

protected:

	void SpawnPlayerPawn(APlayerController* NewPlayer, bool bIsBody);

	UPROPERTY()
	int32 NumInitializedPlayers = 0;
};



