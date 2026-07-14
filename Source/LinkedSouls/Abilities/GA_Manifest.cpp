// Copyright Epic Games, Inc. All Rights Reserved.


#include "GA_Manifest.h"
#include "Player/SoulCharacter.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"

ULS_GA_Manifest::ULS_GA_Manifest()
{
	// this ability is gated by SoulEnergyComponent, not GAS cost/cooldown
	FGameplayTagContainer NewTags;
	NewTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Soul.Manifest")));
	SetAssetTags(NewTags);
}

void ULS_GA_Manifest::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// owner must be the Soul character
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	ASoulCharacter* Soul = Cast<ASoulCharacter>(ActorInfo->AvatarActor.Get());
	if (!Soul)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	// manifest into the Real World for 3 seconds
	Soul->EnterRealWorld();

	// wait for the manifest duration, then end the ability
	UAbilityTask_WaitDelay* WaitTask = UAbilityTask_WaitDelay::WaitDelay(this, 3.0f);
	WaitTask->OnFinish.AddDynamic(this, &ULS_GA_Manifest::OnManifestComplete);
	WaitTask->ReadyForActivation();
}

void ULS_GA_Manifest::CancelAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bRepCancelActivate)
{
	Super::CancelAbility(Handle, ActorInfo, ActivationInfo, bRepCancelActivate);

	// immediately end the manifest when cancelled
	if (ActorInfo && ActorInfo->AvatarActor.IsValid())
	{
		if (ASoulCharacter* Soul = Cast<ASoulCharacter>(ActorInfo->AvatarActor.Get()))
		{
			Soul->EndManifest();
		}
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void ULS_GA_Manifest::OnManifestComplete()
{
	// end the manifest then close the ability
	if (CurrentActorInfo && CurrentActorInfo->AvatarActor.IsValid())
	{
		if (ASoulCharacter* Soul = Cast<ASoulCharacter>(CurrentActorInfo->AvatarActor.Get()))
		{
			Soul->EndManifest();
		}
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
}
