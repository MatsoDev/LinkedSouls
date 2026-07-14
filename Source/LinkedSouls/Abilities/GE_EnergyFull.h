// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_EnergyFull.generated.h"

/**
 *  Timed GameplayEffect: small Health regen buff applied when SoulEnergy
 *  reaches maximum. Both Body and Soul receive this via the GameState delegate.
 *  Duration: 5 seconds.
 */
UCLASS()
class LINKEDSOULS_API ULS_GE_EnergyFull : public UGameplayEffect
{
	GENERATED_BODY()

public:

	/** Constructor: configures the timed Health buff modifier. */
	ULS_GE_EnergyFull(const FObjectInitializer& ObjectInitializer);
};
