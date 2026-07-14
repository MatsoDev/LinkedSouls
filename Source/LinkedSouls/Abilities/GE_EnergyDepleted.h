// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_EnergyDepleted.generated.h"

/**
 *  Instant GameplayEffect: Health regeneration debuff applied when SoulEnergy
 *  is fully depleted. Both Body and Soul receive this via the GameState delegate.
 */
UCLASS()
class LINKEDSOULS_API ULS_GE_EnergyDepleted : public UGameplayEffect
{
	GENERATED_BODY()

public:

	/** Constructor: configures the instant Health debuff modifier. */
	ULS_GE_EnergyDepleted(const FObjectInitializer& ObjectInitializer);
};
