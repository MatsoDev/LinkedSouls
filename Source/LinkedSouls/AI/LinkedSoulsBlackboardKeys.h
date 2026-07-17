#pragma once

#include "CoreMinimal.h"

/** Blackboard key name constants for enemy AI. */
struct LINKEDSOULS_API FLinkedSoulsBlackboardKeys
{
	static const FName TargetActor;
	static const FName HomeLocation;
	static const FName CanSeeTarget;
	static const FName IsAttacking;
	static const FName LastSeenLocation;
	static const FName TimeSinceLostSight;
};
