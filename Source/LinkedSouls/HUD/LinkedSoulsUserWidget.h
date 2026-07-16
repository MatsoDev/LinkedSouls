#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Elements/LinkedSoulsElement.h"
#include "LinkedSoulsUserWidget.generated.h"

UCLASS()
class LINKEDSOULS_API ULinkedSoulsUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	ULinkedSoulsUserWidget(const FObjectInitializer& ObjectInitializer);

	// ── Programmatic layout ──

	virtual void NativeConstruct() override;

	// ── BindWidget UMG elements (bound from optional Blueprint child) ──

	UPROPERTY(meta = (BindWidget))
	class UProgressBar* HealthBar;

	UPROPERTY(meta = (BindWidget))
	class UProgressBar* SoulEnergyBar;

	UPROPERTY(meta = (BindWidget))
	class UProgressBar* CorruptionBar;

	UPROPERTY(meta = (BindWidget))
	class UImage* SynergyIndicator;

	UPROPERTY(meta = (BindWidget))
	class UImage* WorldIndicator;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* HealthLabel;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* SoulEnergyLabel;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* CorruptionLabel;

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

	// ── BlueprintImplementableEvent (visual layer for BP child) ──

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
	void SetSynergyActive(bool bActive);

private:

	/** Creates the full UMG layout in C++ when no Blueprint child provides BindWidget slots. */
	void CreateProgrammaticLayout();

	/** Helper: builds a progress bar. */
	class UProgressBar* MakeBar(const FLinearColor& FillColor);

	/** Helper: builds a text label. */
	class UTextBlock* MakeLabel(const FString& DefaultText);

};
