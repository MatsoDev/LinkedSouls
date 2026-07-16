// Copyright Epic Games, Inc. All Rights Reserved.


#include "GE_CorruptionDamage.h"
#include "Player/LinkedSoulsAttributeSet.h"

ULS_GE_CorruptionDamage::ULS_GE_CorruptionDamage(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo CorruptionMod;
	CorruptionMod.Attribute = ULinkedSoulsAttributeSet::GetCorruptionAttribute();
	CorruptionMod.ModifierOp = EGameplayModOp::Additive;
	CorruptionMod.ModifierMagnitude = FScalableFloat(15.0f);
	Modifiers.Add(CorruptionMod);
}
