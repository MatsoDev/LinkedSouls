// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LinkedSoulsElement.generated.h"

/**
 * The six elements of LinkedSouls.
 *
 * NOTE: named ELinkedSoulsElement (not EElementType) because the engine already
 * has an internal Slate enum called EElementType in DrawElementCoreTypes.h.
 *
 * Physical elements belong to the Body player; spiritual elements belong to the
 * Soul player. Element combinations (Body + Soul) drive the combo effects defined
 * by the ElementComponent in System 6.
 */
UENUM(BlueprintType)
enum class ELinkedSoulsElement : uint8
{
	/** Body physical element - fire / heat. */
	Fire UMETA(DisplayName = "Fire"),

	/** Body physical element - water / cold. */
	Water UMETA(DisplayName = "Water"),

	/** Body physical element - earth / stone. */
	Earth UMETA(DisplayName = "Earth"),

	/** Soul spiritual element - light / radiance. */
	Light UMETA(DisplayName = "Light"),

	/** Soul spiritual element - shadow / stealth. */
	Shadow UMETA(DisplayName = "Shadow"),

	/** Soul spiritual element - void / nullification. */
	Void UMETA(DisplayName = "Void")
};
