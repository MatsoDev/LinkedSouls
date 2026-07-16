// Copyright Epic Games, Inc. All Rights Reserved.


#include "GE_BodySynergyBuff.h"
#include "GameplayEffectComponents/AssetTagsGameplayEffectComponent.h"

ULS_GE_BodySynergyBuff::ULS_GE_BodySynergyBuff(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FScalableFloat(10.0f);

	UAssetTagsGameplayEffectComponent& TagsComp =
		*ObjectInitializer.CreateDefaultSubobject<UAssetTagsGameplayEffectComponent>(
			this, TEXT("AssetTagsComponent"));
	FInheritedTagContainer Tags;
	Tags.Added.AddTag(FGameplayTag::RequestGameplayTag(FName("Linked.Synergy.Active")));
	TagsComp.SetAndApplyAssetTagChanges(Tags);
}
