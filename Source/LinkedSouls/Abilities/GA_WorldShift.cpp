// Copyright Epic Games, Inc. All Rights Reserved.


#include "GA_WorldShift.h"
#include "Player/BodyCharacter.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"

ULS_GA_WorldShift::ULS_GA_WorldShift()
{
	// this ability is gated by SoulEnergyComponent, not GAS cost/cooldown
	FGameplayTagContainer NewTags;
	NewTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Body.WorldShift")));
	SetAssetTags(NewTags);
}

void ULS_GA_WorldShift::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// owner must be the Body character
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	ABodyCharacter* Body = Cast<ABodyCharacter>(ActorInfo->AvatarActor.Get());
	if (!Body)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	// enter the Spirit World for 3 seconds
	Body->EnterSpiritWorld();

	// wait for the shift duration, then end the ability
	UAbilityTask_WaitDelay* WaitTask = UAbilityTask_WaitDelay::WaitDelay(this, 3.0f);
	WaitTask->OnFinish.AddDynamic(this, &ULS_GA_WorldShift::OnShiftComplete);
	WaitTask->ReadyForActivation();
}

void ULS_GA_WorldShift::CancelAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bRepCancelActivate)
{
	Super::CancelAbility(Handle, ActorInfo, ActivationInfo, bRepCancelActivate);

	// immediately end the shift when cancelled
	if (ActorInfo && ActorInfo->AvatarActor.IsValid())
	{
		if (ABodyCharacter* Body = Cast<ABodyCharacter>(ActorInfo->AvatarActor.Get()))
		{
			Body->EndWorldShift();
		}
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void ULS_GA_WorldShift::OnShiftComplete()
{
	// end the shift then close the ability
	if (CurrentActorInfo && CurrentActorInfo->AvatarActor.IsValid())
	{
		if (ABodyCharacter* Body = Cast<ABodyCharacter>(CurrentActorInfo->AvatarActor.Get()))
		{
			Body->EndWorldShift();
		}
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
}
