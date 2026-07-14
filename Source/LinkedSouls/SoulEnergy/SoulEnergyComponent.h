// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SoulEnergyComponent.generated.h"

/** Fires when the shared Soul Energy pool reaches zero. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEnergyDepleted);

/** Fires when the shared Soul Energy pool reaches maximum. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEnergyFull);

/** Fires on every Soul Energy change; HUD listens for bar updates. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnergyChanged, float, NewValue);

/**
 *  Shared Soul Energy pool used by BOTH the Body and Soul players.
 *
 *  Hosts on the GameState (single shared instance) and replicates to all
 *  clients. Energy regenerates faster while Body and Soul are close together
 *  (ProximityRegenRate), slower when apart (BaseRegenRate), and is drained
 *  by World Shift / Manifest abilities.
 *
 *  NOTE: requires a custom ALinkedSoulsGameState that owns this component.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class LINKEDSOULS_API USoulEnergyComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	/** Constructor. */
	USoulEnergyComponent();

	// -- Static access -------------------------------------------------------

	/**
	 *  Finds the shared SoulEnergyComponent on the current GameState.
	 *  Both characters use this to reach the single shared pool.
	 */
	UFUNCTION(BlueprintPure, Category = "Soul Energy", meta = (WorldContext = "WorldContextObject"))
	static USoulEnergyComponent* GetSoulEnergyComponent(const UObject* WorldContextObject);

	// -- Public API ----------------------------------------------------------

	/**
	 *  Flat energy cost paid when an ability activates (World Shift / Manifest).
	 *  Server-authoritative; ignored on clients and when unaffordable.
	 */
	UFUNCTION(BlueprintCallable, Category = "Soul Energy")
	void DrainEnergy(float Amount);

	/**
	 *  Enables / disables continuous drain (reference-counted).
	 *  Multiple callers (Body + Soul) can each request drain; it stops only
	 *  when the last caller releases.
	 */
	UFUNCTION(BlueprintCallable, Category = "Soul Energy")
	void SetContinuousDrain(bool bEnabled);

	/** @returns the current shared Soul Energy value. */
	UFUNCTION(BlueprintPure, Category = "Soul Energy")
	float GetSoulEnergy() const { return SoulEnergy; }

	/** True when the pool is empty. */
	UFUNCTION(BlueprintPure, Category = "Soul Energy")
	bool IsEnergyDepleted() const { return SoulEnergy <= 0.0f; }

	/** True when there is enough energy to pay the flat ability cost. */
	UFUNCTION(BlueprintPure, Category = "Soul Energy")
	bool CanAffordAbility() const { return SoulEnergy >= AbilityFlatCost; }

	// -- Delegates -----------------------------------------------------------

	/** Raised when Soul Energy crosses to zero. */
	UPROPERTY(BlueprintAssignable, Category = "Soul Energy")
	FOnEnergyDepleted OnEnergyDepleted;

	/** Raised when Soul Energy crosses to maximum. */
	UPROPERTY(BlueprintAssignable, Category = "Soul Energy")
	FOnEnergyFull OnEnergyFull;

	/** Raised on every Soul Energy change. */
	UPROPERTY(BlueprintAssignable, Category = "Soul Energy")
	FOnEnergyChanged OnEnergyChanged;

	// -- Config (designer-tunable) ------------------------------------------

	/** Maximum energy the shared pool can hold. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Soul Energy|Config", meta = (ClampMin = "0.0", ClampMax = "1000.0"))
	float MaxEnergy = 100.0f;

	/** Energy the pool starts at on play begin. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Soul Energy|Config", meta = (ClampMin = "0.0", ClampMax = "1000.0"))
	float StartingEnergy = 50.0f;

	/** Regeneration per second while Body + Soul are within ProximityDistance. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Soul Energy|Config|Regen", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float ProximityRegenRate = 5.0f;

	/** Regeneration per second while Body + Soul are apart. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Soul Energy|Config|Regen", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float BaseRegenRate = 1.0f;

	/** Continuous drain per second while at least one ability is active. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Soul Energy|Config|Drain", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float ContinuousDrainRate = 2.0f;

	/** Distance (units) under which the proximity regen rate applies. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Soul Energy|Config|Proximity", meta = (ClampMin = "0.0", ClampMax = "10000.0", Units = "cm"))
	float ProximityDistance = 1000.0f;

	/** Flat energy cost paid once when World Shift / Manifest activates. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Soul Energy|Config|Drain", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float AbilityFlatCost = 20.0f;

	// -- Debug (dev only) ----------------------------------------------------

	/** Reference count of active continuous-drain requests. */
	// Remove VisibleAnywhere before shipping - dev only
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Debug")
	int32 ActiveDrainCount = 0;

protected:

	// -- Replicated state ----------------------------------------------------

	/** The shared Soul Energy value (0..MaxEnergy). */
	UPROPERTY(ReplicatedUsing = OnRep_SoulEnergy, Transient)
	float SoulEnergy = 0.0f;

	/** RepNotify for SoulEnergy; re-broadcasts delegates on clients. */
	UFUNCTION()
	void OnRep_SoulEnergy(float OldValue);

	// -- ActorComponent overrides -------------------------------------------

	/** Per-tick regen / drain resolution (server-authoritative). */
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Replication setup. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// -- Internal helpers ---------------------------------------------------

	/**
	 *  Server-side setter: clamps to [0, MaxEnergy], stores, and broadcasts
	 *  threshold + change delegates.
	 */
	void SetEnergyInternal(float NewValue);

	/** Broadcasts OnEnergyChanged and the threshold delegates (depleted / full). */
	void BroadcastEnergyDelegates(float OldValue, float NewValue);
};
