// Copyright Epic Games, Inc. All Rights Reserved.


#include "GE_EnergyFull.h"
#include "Player/LinkedSoulsAttributeSet.h"
#include "GameplayEffectComponents/AssetTagsGameplayEffectComponent.h"

ULS_GE_EnergyFull::ULS_GE_EnergyFull(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 5-second buff
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FScalableFloat(5.0f);

	// AssetTagsGameplayEffectComponent must be created via ObjectInitializer
	UAssetTagsGameplayEffectComponent& AssetTagsComp =
		*ObjectInitializer.CreateDefaultSubobject<UAssetTagsGameplayEffectComponent>(
			this, TEXT("AssetTagsComponent"));
	FInheritedTagContainer Tags;
	Tags.Added.AddTag(FGameplayTag::RequestGameplayTag(FName("State.EnergyFull")));
	AssetTagsComp.SetAndApplyAssetTagChanges(Tags);

	// modify Health with a small regen buff
	FGameplayModifierInfo HealthModifier;
	HealthModifier.Attribute = ULinkedSoulsAttributeSet::GetHealthAttribute();
	HealthModifier.ModifierOp = EGameplayModOp::Additive;
	HealthModifier.ModifierMagnitude = FScalableFloat(5.0f);
	Modifiers.Add(HealthModifier);
}
