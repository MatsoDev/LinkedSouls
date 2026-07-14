// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_Manifest.generated.h"

class ASoulCharacter;

/**
 *  GAS GameplayAbility for the Soul's Manifest into the Real World.
 *
 *  On activation: casts owner to ASoulCharacter, calls EnterRealWorld(),
 *  waits 3 seconds, then calls EndManifest() and ends the ability.
 *  Cancelled early: immediately ends the manifest and ability.
 *
 *  Cost gating is handled by SoulEnergyComponent (not GAS cost GE), so
 *  this ability has no GAS Cost or Cooldown.
 */
UCLASS()
class LINKEDSOULS_API ULS_GA_Manifest : public UGameplayAbility
{
	GENERATED_BODY()

public:

	/** Constructor. */
	ULS_GA_Manifest();

protected:

	/** Activation: enter Real World, wait, then end manifest. */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** Cancel: immediately end the manifest and ability. */
	virtual void CancelAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bRepCancelActivate) override;

	/** Completes the ability after the delay finishes. */
	void OnManifestComplete();
};
