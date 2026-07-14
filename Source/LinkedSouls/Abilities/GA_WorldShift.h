// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/GameplayAbility.h"
#include "GA_WorldShift.generated.h"

class ABodyCharacter;

/**
 *  GAS GameplayAbility for the Body's World Shift.
 *
 *  On activation: casts owner to ABodyCharacter, calls EnterSpiritWorld(),
 *  waits 3 seconds, then calls EndWorldShift() and ends the ability.
 *  Cancelled early: immediately ends the shift and ability.
 *
 *  Cost gating is handled by SoulEnergyComponent (not GAS cost GE), so
 *  this ability has no GAS Cost or Cooldown.
 */
UCLASS()
class LINKEDSOULS_API ULS_GA_WorldShift : public UGameplayAbility
{
	GENERATED_BODY()

public:

	/** Constructor. */
	ULS_GA_WorldShift();

protected:

	/** Activation: enter Spirit World, wait, then end shift. */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** Cancel: immediately end the shift and ability. */
	virtual void CancelAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bRepCancelActivate) override;

	/** Completes the ability after the delay finishes. */
	void OnShiftComplete();
};
