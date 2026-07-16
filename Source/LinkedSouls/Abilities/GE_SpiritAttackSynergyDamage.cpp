// Copyright Epic Games, Inc. All Rights Reserved.


#include "GE_SpiritAttackSynergyDamage.h"
#include "Player/LinkedSoulsAttributeSet.h"

ULS_GE_SpiritAttackSynergyDamage::ULS_GE_SpiritAttackSynergyDamage(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo DamageMod;
	DamageMod.Attribute = ULinkedSoulsAttributeSet::GetHealthAttribute();
	DamageMod.ModifierOp = EGameplayModOp::Additive;
	DamageMod.ModifierMagnitude = FScalableFloat(-45.0f);
	Modifiers.Add(DamageMod);
}
