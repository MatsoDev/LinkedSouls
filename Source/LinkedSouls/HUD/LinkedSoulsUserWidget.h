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

	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	class UProgressBar* HealthBar;

	UPROPERTY(meta = (BindWidget))
	class UProgressBar* SoulEnergyBar;

	UPROPERTY(meta = (BindWidget))
	class UProgressBar* CorruptionBar;

	UPROPERTY(meta = (BindWidgetOptional))
	class UImage* SynergyIndicator;

	UPROPERTY(meta = (BindWidgetOptional))
	class UImage* WorldIndicator;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* HealthLabel;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* SoulEnergyLabel;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* CorruptionLabel;

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

	void UpdateHealthBar(float Current, float Max);
	void UpdateSoulEnergyBar(float Current, float Max);
	void UpdateCorruptionBar(float Current, float Max);
	void SetEnergyDepletedWarning(bool bDepleted);
	void UpdateElementSlot(int32 SlotIndex, ELinkedSoulsElement Element);
	void UpdateWorldIndicator(bool bInSpiritWorld);
	void SetSynergyActive(bool bActive);

private:
	void BuildHUD();

	class UVerticalBox* VBox;
};
