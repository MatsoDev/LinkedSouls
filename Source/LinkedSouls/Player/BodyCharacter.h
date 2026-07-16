// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LinkedSoulsPlayerCharacter.h"
#include "BodyCharacter.generated.h"

class ASoulCharacter;
class UInputAction;
class UElementComponent;

/**
 *  The Body player character - lives in the Real World.
 *
 *  Physical combatant who can briefly shift into the Spirit World (default
 *  3 seconds) to coordinate with the linked Soul player. Uses the default
 *  Manny placeholder mesh until team art is delivered.
 */
UCLASS()
class LINKEDSOULS_API ABodyCharacter : public ALinkedSoulsPlayerCharacter
{
	GENERATED_BODY()

public:

	/** Constructor. */
	ABodyCharacter();

	// -- Identity ------------------------------------------------------------

	/** Body lives in the Real World. */
	virtual EDualWorld GetPlayerWorld() const override { return EDualWorld::RealWorld; }

protected:

	// -- Lifecycle -----------------------------------------------------------

	virtual void BeginPlay() override;

	/** Adds Body-specific input mapping context (IMC_Body). */
	virtual void AddInputContexts() override;

	// -- Mesh ----------------------------------------------------------------

	/**
	 *  Body uses the stock Manny mesh assigned in the constructor.
	 *  No runtime mesh configuration is needed (no material tint, no gravity
	 *  override, no collision override).
	 */
	virtual void ConfigureMesh() override;

	// -- Combat ---------------------------------------------------------------

	/** Input action for Body's melee attack. */
	UPROPERTY(EditAnywhere, Category = "Combat")
	UInputAction* BodyMeleeAction;

	/** Called when Body melee input is triggered. */
	void OnBodyMelee();

	/** Server RPC — applies damage on authority. */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_BodyMelee();

	/** Performs the melee sphere trace and applies damage (server only). */
	void PerformMeleeAttack();

	// -- World Shift (Body-only ability) ------------------------------------

	/** Input action that briefly shifts Body into the Spirit World. */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* WorldShiftAction;

	/** How long (seconds) Body stays shifted into the Spirit World. */
	UPROPERTY(EditAnywhere, Category = "World Shift", meta = (ClampMin = "0.1", ClampMax = "10.0", Units = "s"))
	float WorldShiftDuration = 3.0f;

	/** True while Body is temporarily present in the Spirit World. */
	// TODO: Replicate for co-op sync
	UPROPERTY(BlueprintReadOnly, Category = "World")
	bool bIsInSpiritWorld = false;

	/** Timer controlling the temporary Spirit World shift. */
	FTimerHandle WorldShiftTimer;

	/** Called when the World Shift input is triggered. */
	void OnWorldShift();

public:

	/** Shifts Body into the Spirit World and starts the duration timer. */
	void EnterSpiritWorld();

	/** Returns Body to the Real World after the shift duration expires. */
	void EndWorldShift();

protected:

	// -- Input ---------------------------------------------------------------

	/** Binds Body-specific input (World Shift) on top of the base actions. */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// -- Co-op link ----------------------------------------------------------

	/** The Soul character linked to this Body (weak to avoid hard references). */
	UPROPERTY(BlueprintReadOnly, Category = "LinkedSouls|Co-op")
	TWeakObjectPtr<ASoulCharacter> LinkedSoul;

public:

	/** Sets the linked Soul character. */
	UFUNCTION(BlueprintCallable, Category = "LinkedSouls|Co-op")
	void SetLinkedSoul(ASoulCharacter* InSoul);

	/** @returns the linked Soul character, or nullptr if unset / destroyed. */
	UFUNCTION(BlueprintPure, Category = "LinkedSouls|Co-op")
	ASoulCharacter* GetLinkedSoul() const;

	/** Element manager — Body can use Fire, Water, Earth. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LinkedSouls|Element")
	UElementComponent* ElementComponent;

	/** @returns true while Body is temporarily shifted into the Spirit World. */
	UFUNCTION(BlueprintPure, Category = "LinkedSouls|World Shift")
	bool IsInSpiritWorld() const { return bIsInSpiritWorld; }
};
