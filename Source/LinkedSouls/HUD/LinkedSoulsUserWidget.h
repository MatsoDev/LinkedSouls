#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Elements/LinkedSoulsElement.h"
#include "LinkedSoulsUserWidget.generated.h"

/**
 * TEAM ASSET HANDOFF
 * System: UI/HUD
 * Replace with: BP_LinkedSoulsHUDWidget
 *   (child of ULinkedSoulsUserWidget)
 *   implements all BlueprintImplementableEvents
 *   with actual UMG bars, icons, animations
 * Delivered by: UI Designer
 * Format: Blueprint + UMG Widget
 * Priority: High
 */
UCLASS()
class LINKEDSOULS_API ULinkedSoulsUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	ULinkedSoulsUserWidget(const FObjectInitializer& ObjectInitializer);

	// ── Data (BlueprintReadOnly for UMG binding) ──

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Health")
	float CurrentHealth = 100.0f;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Health")
	float MaxHealth = 100.0f;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|SoulEnergy")
	float CurrentSoulEnergy = 50.0f;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|SoulEnergy")
	float MaxSoulEnergy = 100.0f;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Corruption")
	float CurrentCorruption = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Corruption")
	float MaxCorruption = 100.0f;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Energy")
	bool bEnergyDepletedWarning = false;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Element")
	ELinkedSoulsElement ActiveElementSlot0 = ELinkedSoulsElement::Fire;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Element")
	ELinkedSoulsElement ActiveElementSlot1 = ELinkedSoulsElement::Fire;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|World")
	bool bIsBodyPlayer = true;

	// ── BlueprintImplementableEvent (visual layer) ──

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnHealthUpdated(float Current, float Max);

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnSoulEnergyUpdated(float Current, float Max);

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnCorruptionUpdated(float Current, float Max);

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnEnergyWarningChanged(bool bDepleted);

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnElementSlotUpdated(int32 SlotIndex, ELinkedSoulsElement Element);

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnWorldIndicatorUpdated(bool bInSpiritWorld);

	// ── C++ wrapper API (called by HUD) ──

	void UpdateHealthBar(float Current, float Max);
	void UpdateSoulEnergyBar(float Current, float Max);
	void UpdateCorruptionBar(float Current, float Max);
	void SetEnergyDepletedWarning(bool bDepleted);
	void UpdateElementSlot(int32 SlotIndex, ELinkedSoulsElement Element);
	void UpdateWorldIndicator(bool bInSpiritWorld);
};
