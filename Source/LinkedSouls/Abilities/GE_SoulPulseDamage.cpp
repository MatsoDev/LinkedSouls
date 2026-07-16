// Copyright Epic Games, Inc. All Rights Reserved.


#include "GE_SoulPulseDamage.h"
#include "Player/LinkedSoulsAttributeSet.h"
#include "GameplayEffectComponents/AssetTagsGameplayEffectComponent.h"

ULS_GE_SoulPulseDamage::ULS_GE_SoulPulseDamage(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo DamageMod;
	DamageMod.Attribute = ULinkedSoulsAttributeSet::GetHealthAttribute();
	DamageMod.ModifierOp = EGameplayModOp::Additive;
	DamageMod.ModifierMagnitude = FScalableFloat(-20.0f);
	Modifiers.Add(DamageMod);
}
