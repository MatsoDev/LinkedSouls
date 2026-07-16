// Copyright Epic Games, Inc. All Rights Reserved.


#include "GE_CorruptionDecay.h"
#include "Player/LinkedSoulsAttributeSet.h"

ULS_GE_CorruptionDecay::ULS_GE_CorruptionDecay(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo DecayMod;
	DecayMod.Attribute = ULinkedSoulsAttributeSet::GetCorruptionAttribute();
	DecayMod.ModifierOp = EGameplayModOp::Additive;
	DecayMod.ModifierMagnitude = FScalableFloat(-5.0f);
	Modifiers.Add(DecayMod);
}
