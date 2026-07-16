// Copyright Epic Games, Inc. All Rights Reserved.


#include "GE_BodyMeleeDamage.h"
#include "Player/LinkedSoulsAttributeSet.h"
#include "GameplayEffectComponents/AssetTagsGameplayEffectComponent.h"

ULS_GE_BodyMeleeDamage::ULS_GE_BodyMeleeDamage(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo DamageMod;
	DamageMod.Attribute = ULinkedSoulsAttributeSet::GetHealthAttribute();
	DamageMod.ModifierOp = EGameplayModOp::Additive;
	DamageMod.ModifierMagnitude = FScalableFloat(-25.0f);
	Modifiers.Add(DamageMod);
}
