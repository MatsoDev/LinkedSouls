// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "LinkedSoulsGameState.generated.h"

class USoulEnergyComponent;
class UGameplayEffect;
class ULS_GE_EnergyDepleted;
class ULS_GE_EnergyFull;

/**
 *  Project GameState for LinkedSouls.
 *
 *  Owns the shared USoulEnergyComponent (single pool for both players) and
 *  wires its OnEnergyDepleted / OnEnergyFull delegates to apply GAS gameplay
 *  effects to both the Body and Soul characters.
 */
UCLASS()
class LINKEDSOULS_API ALinkedSoulsGameState : public AGameStateBase
{
	GENERATED_BODY()

public:

	/** Constructor. */
	ALinkedSoulsGameState();

	/** Shared soul energy pool component. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LinkedSouls|Components")
	USoulEnergyComponent* SoulEnergyComponent;

	/** GameplayEffect applied to both players when energy is depleted. */
	UPROPERTY(EditDefaultsOnly, Category = "LinkedSouls|GAS")
	TSubclassOf<ULS_GE_EnergyDepleted> GE_EnergyDepleted;

	/** GameplayEffect applied to both players when energy is full. */
	UPROPERTY(EditDefaultsOnly, Category = "LinkedSouls|GAS")
	TSubclassOf<ULS_GE_EnergyFull> GE_EnergyFull;

protected:

	/** Binds the SoulEnergyComponent delegates. */
	virtual void BeginPlay() override;

	/** Called when the shared SoulEnergy pool is depleted. */
	UFUNCTION()
	void OnEnergyDepleted();

	/** Called when the shared SoulEnergy pool reaches maximum. */
	UFUNCTION()
	void OnEnergyFull();

	/**
	 *  Applies a gameplay effect class to BOTH players (Body + Soul) via the
	 *  DualWorldManager. Null-guarded on both ends.
	 */
	void ApplyGEToPlayers(TSubclassOf<UGameplayEffect> GEClass);
};
