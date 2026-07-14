// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LinkedSoulsElement.h"
#include "ElementComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnElementChanged, ELinkedSoulsElement, NewElement);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnElementCombined, FName, ComboName, float, Magnitude);

/**
 *  Per-character element manager.
 *
 *  Attach to BodyCharacter or SoulCharacter to track which element the player
 *  currently has active. Provides the TryCombineWith() API for the co-op
 *  puzzle ElementLink type (System 5) and future combat combos (System 7).
 *
 *  Body defaults to { Fire, Water, Earth } with Fire active.
 *  Soul defaults to { Light, Shadow, Void } with Light active.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class LINKEDSOULS_API UElementComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UElementComponent();

	// -- Config --------------------------------------------------------------

	/** Which elements this character is allowed to use. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Element")
	TArray<ELinkedSoulsElement> AllowedElements;

	// -- API ----------------------------------------------------------------

	/** Sets the active element. Returns false if the element is not in AllowedElements. */
	UFUNCTION(BlueprintCallable, Category = "Element")
	bool SetActiveElement(ELinkedSoulsElement NewElement);

	/** @returns the currently active element. */
	UFUNCTION(BlueprintPure, Category = "Element")
	ELinkedSoulsElement GetActiveElement() const { return ActiveElement; }

	/**
	 *  Attempts an element combination with another player's ElementComponent.
	 *
	 *  @param OtherComponent  The other player's ElementComponent.
	 *  @return The combo result name, or NAME_None if no valid combination exists.
	 */
	UFUNCTION(BlueprintCallable, Category = "Element")
	FName TryCombineWith(UElementComponent* OtherComponent);

	// -- Delegates ----------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "Element")
	FOnElementChanged OnElementChanged;

	UPROPERTY(BlueprintAssignable, Category = "Element")
	FOnElementCombined OnElementCombined;

	// -- Replication --------------------------------------------------------

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:

	virtual void BeginPlay() override;

private:

	UPROPERTY(ReplicatedUsing = OnRep_ActiveElement)
	ELinkedSoulsElement ActiveElement;

	UFUNCTION()
	void OnRep_ActiveElement(ELinkedSoulsElement OldElement);
};
