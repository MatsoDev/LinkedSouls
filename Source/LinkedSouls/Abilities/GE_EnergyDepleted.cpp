// Copyright Epic Games, Inc. All Rights Reserved.


#include "GE_EnergyDepleted.h"
#include "Player/LinkedSoulsAttributeSet.h"
#include "GameplayEffectComponents/AssetTagsGameplayEffectComponent.h"

ULS_GE_EnergyDepleted::ULS_GE_EnergyDepleted(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// instant effect applied once when the delegate fires
	DurationPolicy = EGameplayEffectDurationType::Instant;

	// AssetTagsGameplayEffectComponent must be created via ObjectInitializer
	UAssetTagsGameplayEffectComponent& AssetTagsComp =
		*ObjectInitializer.CreateDefaultSubobject<UAssetTagsGameplayEffectComponent>(
			this, TEXT("AssetTagsComponent"));
	FInheritedTagContainer Tags;
	Tags.Added.AddTag(FGameplayTag::RequestGameplayTag(FName("State.EnergyDepleted")));
	AssetTagsComp.SetAndApplyAssetTagChanges(Tags);

	// modify Health regeneration
	FGameplayModifierInfo HealthModifier;
	HealthModifier.Attribute = ULinkedSoulsAttributeSet::GetHealthAttribute();
	HealthModifier.ModifierOp = EGameplayModOp::Additive;
	HealthModifier.ModifierMagnitude = FScalableFloat(0.0f); // TODO: tune debuff value with design team
	Modifiers.Add(HealthModifier);
}
