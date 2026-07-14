#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "LinkedSoulsAttributeSet.h"
#include "SoulEnergyComponent.h"
#include "ElementComponent.h"
#include "HUD/LinkedSoulsUserWidget.h"
#include "LinkedSoulsHUD.generated.h"

/**
 * Server-side HUD manager.
 *
 *  - Creates and owns the widget instance.
 *  - Listens to GAS attribute changes to update the widget.
 *  - Listens to SoulEnergyComponent delegates.
 *  - Listens to ElementComponent OnElementChanged.
 *  - Knows whether the local player is Body or Soul to show correct bars.
 */
UCLASS()
class LINKEDSOULS_API ALinkedSoulsHUD : public AHUD
{
	GENERATED_BODY()

public:

	ALinkedSoulsHUD();

	// ── Config ──

	UPROPERTY(EditAnywhere, Category = "HUD")
	TSubclassOf<ULinkedSoulsUserWidget> HUDWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	ULinkedSoulsUserWidget* HUDWidget = nullptr;

	// ── Lifecycle ──

	virtual void BeginPlay() override;

	void BindToLocalPlayer();

	// ── GAS Attribute Callbacks (AddUObject — no UFUNCTION needed) ──

	void OnHealthChanged(const FOnAttributeChangeData& Data);
	void OnSoulEnergyAttributeChanged(const FOnAttributeChangeData& Data);
	void OnCorruptionChanged(const FOnAttributeChangeData& Data);

	// ── SoulEnergyComponent Callbacks (AddDynamic — need UFUNCTION) ──

	UFUNCTION()
	void OnSoulEnergyChanged(float NewValue);

	UFUNCTION()
	void OnEnergyDepleted();

	UFUNCTION()
	void OnEnergyFull();

	// ── ElementComponent Callback (AddDynamic — need UFUNCTION) ──

	UFUNCTION()
	void OnActiveElementChanged(ELinkedSoulsElement NewElement);

	// ── Helpers ──

	float GetAttributeMax(FGameplayAttribute Attribute) const;

private:

	UPROPERTY()
	bool bLocalPlayerIsBody = true;
};
