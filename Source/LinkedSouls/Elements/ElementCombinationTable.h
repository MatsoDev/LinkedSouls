// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LinkedSoulsElement.h"

/**
 *  Defines all valid Body + Soul element combinations and their results.
 *
 *  Each combo maps (BodyElement, SoulElement) → (ResultName, Magnitude).
 *  The table is static and compiled in; designers can tune values via the
 *  UElementComponent's TryCombineWith() API.
 */

struct FElementCombo
{
	ELinkedSoulsElement BodyElement;
	ELinkedSoulsElement SoulElement;
	FName ResultName;
	float Magnitude;
};

static const TArray<FElementCombo> CombinationTable =
{
	{ ELinkedSoulsElement::Fire,   ELinkedSoulsElement::Light,  FName("SolarBlast"),  150.0f },
	{ ELinkedSoulsElement::Water,  ELinkedSoulsElement::Shadow, FName("DarkTide"),    120.0f },
	{ ELinkedSoulsElement::Earth,  ELinkedSoulsElement::Void,   FName("VoidQuake"),   180.0f }
};

/**
 *  Looks up the result of combining two elements.
 *
 *  @param Body          Element used by the Body player.
 *  @param Soul          Element used by the Soul player.
 *  @param OutMagnitude  Receives the combo's power value if a match is found.
 *  @return The combo result name, or NAME_None if no valid combination exists.
 */
static FName GetCombinationResult(ELinkedSoulsElement Body, ELinkedSoulsElement Soul, float& OutMagnitude)
{
	for (const FElementCombo& Combo : CombinationTable)
	{
		if (Combo.BodyElement == Body && Combo.SoulElement == Soul)
		{
			OutMagnitude = Combo.Magnitude;
			return Combo.ResultName;
		}
	}

	OutMagnitude = 0.0f;
	return NAME_None;
}
