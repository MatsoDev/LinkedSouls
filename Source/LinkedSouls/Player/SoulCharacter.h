// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LinkedSoulsPlayerCharacter.h"
#include "SoulCharacter.generated.h"

class ABodyCharacter;
class UInputAction;
class UMaterialInstanceDynamic;
class UElementComponent;

/**
 *  The Soul player character - lives in the Spirit World.
 *
 *  Spiritual combatant who can briefly manifest into the Real World (default
 *  3 seconds) to coordinate with the linked Body player. Wears a translucent
 *  blue material tint over the Manny placeholder mesh until team art arrives.
 *  Takes corruption damage instead of physical damage.
 */
UCLASS()
class LINKEDSOULS_API ASoulCharacter : public ALinkedSoulsPlayerCharacter
{
	GENERATED_BODY()

public:

	/** Constructor. */
	ASoulCharacter();

	// -- Identity ------------------------------------------------------------

	/** Soul lives in the Spirit World. */
	virtual EDualWorld GetPlayerWorld() const override { return EDualWorld::SpiritWorld; }

protected:

	// -- Lifecycle -----------------------------------------------------------

	virtual void BeginPlay() override;

	/** Adds Soul-specific input mapping context (IMC_Soul). */
	virtual void AddInputContexts() override;

	// -- Mesh + material ----------------------------------------------------

	/**
	 *  Loads the Manny placeholder in the constructor, then at runtime applies
	 *  a translucent blue UMaterialInstanceDynamic tint.
	 */
	virtual void ConfigureMesh() override;

	/** Dynamic material instance applied to the Soul mesh. */
	// TODO: driven by corruption level later (purple shift)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LinkedSouls|Soul|Material")
	UMaterialInstanceDynamic* SoulMaterialInstance;

	// -- Co-op link ----------------------------------------------------------

	/** The Body character linked to this Soul (weak to avoid hard references). */
	UPROPERTY(BlueprintReadOnly, Category = "LinkedSouls|Co-op")
	TWeakObjectPtr<ABodyCharacter> LinkedBody;

public:

	/** Sets the linked Body character. */
	UFUNCTION(BlueprintCallable, Category = "LinkedSouls|Co-op")
	void SetLinkedBody(ABodyCharacter* InBody);

	/** @returns the linked Body character, or nullptr if unset / destroyed. */
	UFUNCTION(BlueprintPure, Category = "LinkedSouls|Co-op")
	ABodyCharacter* GetLinkedBody() const;

	// -- Soul-specific input ------------------------------------------------

	/** Input action for the Soul's primary spiritual attack. */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* SpiritAttackAction;

	/** Input action that briefly manifests Soul into the Real World (E key). */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* ManifestAction;

	/** Input action for the Soul Pulse ability. */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* SoulPulseAction;

	// -- Manifest (Soul-only ability, mirrors Body's World Shift) -----------

	/** How long (seconds) Soul stays manifested in the Real World. */
	UPROPERTY(EditAnywhere, Category = "Manifest", meta = (ClampMin = "0.1", ClampMax = "10.0", Units = "s"))
	float ManifestDuration = 3.0f;

	/** True while Soul is temporarily manifested in the Real World. */
	// TODO: Replicate for co-op sync
	UPROPERTY(BlueprintReadOnly, Category = "Manifest")
	bool bIsManifested = false;

protected:

	/** Timer controlling the temporary Real World manifestation. */
	FTimerHandle ManifestTimer;

	/** Called when the Manifest input is triggered. */
	void OnManifest();

	/** Called when Spirit Attack input is triggered. */
	void OnSpiritAttack();

	/** Server RPC — applies spirit attack damage on authority. */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SpiritAttack();

	/** Applies one tick of Corruption Decay (called every 2s). */
	void TickCorruptionDecay();

	FTimerHandle CorruptionDecayTimer;

	/** Called when Soul Pulse input is triggered. */
	void OnSoulPulse();

	/** Server RPC — applies soul pulse damage on authority. */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SoulPulse();

public:

	/** Manifests Soul into the Real World and starts the duration timer. */
	void EnterRealWorld();

	/** Returns Soul to the Spirit World after the manifest duration expires. */
	void EndManifest();

protected:

	// -- Combat --------------------------------------------------------------

public:

	/**
	 *  Applies corruption damage to this Soul via GAS.
	 *  @param Amount   How much corruption to add.
	 */
	// TODO: wire to GAS GameplayEffect in System 4
	UFUNCTION(BlueprintCallable, Category = "LinkedSouls|Soul|Combat")
	void OnCorruptionDamage(float Amount);

protected:

	// -- Input ---------------------------------------------------------------

	/** Binds Soul-specific input (Manifest / Spirit Attack / Soul Pulse). */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:

	/** @returns true while Soul is temporarily manifested in the Real World. */
	UFUNCTION(BlueprintPure, Category = "LinkedSouls|Manifest")
	bool IsManifested() const { return bIsManifested; }

	/** Element manager — Soul can use Light, Shadow, Void. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LinkedSouls|Element")
	UElementComponent* ElementComponent;
};
