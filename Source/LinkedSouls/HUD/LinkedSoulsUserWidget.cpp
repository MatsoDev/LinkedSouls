#include "HUD/LinkedSoulsUserWidget.h"

ULinkedSoulsUserWidget::ULinkedSoulsUserWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void ULinkedSoulsUserWidget::UpdateHealthBar(float Current, float Max)
{
	CurrentHealth = Current;
	MaxHealth = Max;
	OnHealthUpdated(Current, Max);
}

void ULinkedSoulsUserWidget::UpdateSoulEnergyBar(float Current, float Max)
{
	CurrentSoulEnergy = Current;
	MaxSoulEnergy = Max;
	OnSoulEnergyUpdated(Current, Max);
}

void ULinkedSoulsUserWidget::UpdateCorruptionBar(float Current, float Max)
{
	CurrentCorruption = Current;
	MaxCorruption = Max;
	OnCorruptionUpdated(Current, Max);
}

void ULinkedSoulsUserWidget::SetEnergyDepletedWarning(bool bDepleted)
{
	bEnergyDepletedWarning = bDepleted;
	OnEnergyWarningChanged(bDepleted);
}

void ULinkedSoulsUserWidget::UpdateElementSlot(int32 SlotIndex, ELinkedSoulsElement Element)
{
	if (SlotIndex == 0)
	{
		ActiveElementSlot0 = Element;
	}
	else if (SlotIndex == 1)
	{
		ActiveElementSlot1 = Element;
	}
	OnElementSlotUpdated(SlotIndex, Element);
}

void ULinkedSoulsUserWidget::UpdateWorldIndicator(bool bInSpiritWorld)
{
	OnWorldIndicatorUpdated(bInSpiritWorld);
}
