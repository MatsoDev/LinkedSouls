// Copyright Epic Games, Inc. All Rights Reserved.


#include "GE_SpiritAttackDamage.h"
#include "Player/LinkedSoulsAttributeSet.h"
#include "GameplayEffectComponents/AssetTagsGameplayEffectComponent.h"

ULS_GE_SpiritAttackDamage::ULS_GE_SpiritAttackDamage(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo DamageMod;
	DamageMod.Attribute = ULinkedSoulsAttributeSet::GetHealthAttribute();
	DamageMod.ModifierOp = EGameplayModOp::Additive;
	DamageMod.ModifierMagnitude = FScalableFloat(-30.0f);
	Modifiers.Add(DamageMod);
}
