// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "DualWorldManager.h"     // EDualWorld (return type of GetPlayerWorld)
#include "LinkedSoulsElement.h"   // ELinkedSoulsElement (backs the equipped-slots UPROPERTY)
#include "LinkedSoulsPlayerCharacter.generated.h"

class UAbilitySystemComponent;
class ULinkedSoulsAttributeSet;
class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;

/**
 *  Abstract base for both co-op player characters in LinkedSouls (Body + Soul).
 *
 *  Owns the GAS AbilitySystemComponent + LinkedSoulsAttributeSet, holds the
 *  Enhanced Input actions shared by both characters, and exposes the
 *  equipped-element slots (max 2). Each subclass declares its world
 *  affiliation (GetPlayerWorld) and its mesh/art configuration (ConfigureMesh).
 */
UCLASS(Abstract)
class LINKEDSOULS_API ALinkedSoulsPlayerCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:

	/** Constructor. */
	ALinkedSoulsPlayerCharacter();

	// ~Begin IAbilitySystemInterface

	/** Returns the GAS ability system component. */
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// ~End IAbilitySystemInterface

	// -- Components ----------------------------------------------------------

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	/** GAS ability system component that drives attributes and abilities */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UAbilitySystemComponent* AbilitySystemComponent;

	/** Attribute set holding Health / SoulEnergy / Corruption */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	ULinkedSoulsAttributeSet* AttributeSet;

	// -- Identity (subclasses must implement) -------------------------------

	/** Which world this character belongs to (Real or Spirit). */
	UFUNCTION(BlueprintPure, Category = "LinkedSouls|Player")
	virtual EDualWorld GetPlayerWorld() const PURE_VIRTUAL(ALinkedSoulsPlayerCharacter::GetPlayerWorld, return EDualWorld::RealWorld;);

	// -- Element slots ------------------------------------------------------

	/** Maximum number of elements a player may have equipped at once. */
	static constexpr int32 MaxEquippedElements = 2;

	/** Equips an element if there is a free slot. Returns false if full or duplicate. */
	UFUNCTION(BlueprintCallable, Category = "LinkedSouls|Player|Elements")
	bool EquipElement(ELinkedSoulsElement Element);

	/** Removes an equipped element. Returns false if it was not equipped. */
	UFUNCTION(BlueprintCallable, Category = "LinkedSouls|Player|Elements")
	bool UnequipElement(ELinkedSoulsElement Element);

	/** @returns the currently equipped elements (read-only). */
	UFUNCTION(BlueprintPure, Category = "LinkedSouls|Player|Elements")
	const TArray<ELinkedSoulsElement>& GetEquippedElements() const { return EquippedElements; }

	// -- Input (shared Enhanced Input actions) ------------------------------

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* MouseLookAction;

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoJumpStart();

	/** Handles jump released inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoJumpEnd();

protected:

	/** Elements currently equipped (clamped to MaxEquippedElements at runtime). */
	UPROPERTY(BlueprintReadOnly, Category = "LinkedSouls|Player|Elements")
	TArray<ELinkedSoulsElement> EquippedElements;

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	/**
	 *  Subclasses configure their art here: skeletal mesh, material tint,
	 *  gravity scale, collision profile, etc. Called from BeginPlay.
	 */
	virtual void ConfigureMesh() PURE_VIRTUAL(ALinkedSoulsPlayerCharacter::ConfigureMesh, );

	// -- Lifecycle ----------------------------------------------------------

	/** Initializes GAS and registers the character with the DualWorldManager. */
	virtual void BeginPlay() override;

	/** Server-side GAS initialization. */
	virtual void PossessedBy(AController* NewController) override;

	/** Client-side GAS initialization. */
	virtual void OnRep_PlayerState() override;

	/** Called when the Controller replicates to clients. Adds input contexts. */
	virtual void OnRep_Controller() override;

	/** Adds the shared Enhanced Input mapping contexts (IMC_Default, IMC_MouseLook). */
	virtual void AddInputContexts();

	/** Initializes the AbilitySystemComponent + AttributeSet (called on server and client paths). */
	void InitAbilitySystem();

protected:
	/** Flag to prevent adding input contexts multiple times. */
	bool bInputContextsAdded = false;

	/** Timer handle for deferred IMC setup in BeginPlay. */
	FTimerHandle DelayedIMCSetupTimer;

public:

	// -- Co-op partner link --------------------------------------------------

	/** The other player character (Body ↔ Soul). Set by GameMode PostLogin. */
	UPROPERTY(BlueprintReadOnly, Category = "LinkedSouls|Co-op")
	TWeakObjectPtr<ALinkedSoulsPlayerCharacter> LinkedPartner;

	/** Assign the co-op partner. */
	UFUNCTION(BlueprintCallable, Category = "LinkedSouls|Co-op")
	void SetLinkedPartner(ALinkedSoulsPlayerCharacter* Partner);

	/** @returns the linked partner, or nullptr if unset or destroyed. */
	UFUNCTION(BlueprintPure, Category = "LinkedSouls|Co-op")
	ALinkedSoulsPlayerCharacter* GetLinkedPartner() const;

	/** Called when the character dies (Health ≤ 0 or Corruption ≥ Max). */
	UFUNCTION(BlueprintCallable, Category = "LinkedSouls|Combat")
	virtual void OnCharacterDeath();

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};
