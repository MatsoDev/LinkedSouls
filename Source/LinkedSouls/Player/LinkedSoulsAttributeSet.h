// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "LinkedSoulsAttributeSet.generated.h"

/**
 * GAS attribute set shared by the Body and Soul players.
 *
 *   Health       (red)     - Body's physical life pool.
 *   SoulEnergy   (blue)    - Shared resource visible to BOTH players.
 *   Corruption   (purple)  - Soul's accumulating corruption damage.
 *
 * Every attribute exposes a base and a max variant so abilities/effects can
 * clamp and scale them the standard GAS way.
 */
UCLASS()
class LINKEDSOULS_API ULinkedSoulsAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:

	/** Default constructor. */
	ULinkedSoulsAttributeSet();

	// ~Begin UAttributeSet interface

	/** Clamps incoming attribute changes against their max values. */
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	/** Routes attribute execution after gameplay effects resolve. */
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	// ~End UAttributeSet interface

	// -- Health (Body's red bar) --------------------------------------------

	/** Current physical health of the Body. */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Health", ReplicatedUsing = OnRep_Health)
	FGameplayAttributeData Health;
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ULinkedSoulsAttributeSet, Health)
	GAMEPLAYATTRIBUTE_VALUE_GETTER(Health)
	GAMEPLAYATTRIBUTE_VALUE_SETTER(Health)
	GAMEPLAYATTRIBUTE_VALUE_INITTER(Health)

	/** Maximum physical health of the Body. */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Health", ReplicatedUsing = OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ULinkedSoulsAttributeSet, MaxHealth)
	GAMEPLAYATTRIBUTE_VALUE_GETTER(MaxHealth)
	GAMEPLAYATTRIBUTE_VALUE_SETTER(MaxHealth)
	GAMEPLAYATTRIBUTE_VALUE_INITTER(MaxHealth)

	// -- Soul Energy (shared blue bar) --------------------------------------

	/** Shared soul energy used by both players. */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Soul Energy", ReplicatedUsing = OnRep_SoulEnergy)
	FGameplayAttributeData SoulEnergy;
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ULinkedSoulsAttributeSet, SoulEnergy)
	GAMEPLAYATTRIBUTE_VALUE_GETTER(SoulEnergy)
	GAMEPLAYATTRIBUTE_VALUE_SETTER(SoulEnergy)
	GAMEPLAYATTRIBUTE_VALUE_INITTER(SoulEnergy)

	/** Maximum soul energy that either player can hold. */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Soul Energy", ReplicatedUsing = OnRep_MaxSoulEnergy)
	FGameplayAttributeData MaxSoulEnergy;
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ULinkedSoulsAttributeSet, MaxSoulEnergy)
	GAMEPLAYATTRIBUTE_VALUE_GETTER(MaxSoulEnergy)
	GAMEPLAYATTRIBUTE_VALUE_SETTER(MaxSoulEnergy)
	GAMEPLAYATTRIBUTE_VALUE_INITTER(MaxSoulEnergy)

	// -- Corruption (Soul's purple bar) -------------------------------------

	/** Soul's accumulated corruption damage. */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Corruption", ReplicatedUsing = OnRep_Corruption)
	FGameplayAttributeData Corruption;
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ULinkedSoulsAttributeSet, Corruption)
	GAMEPLAYATTRIBUTE_VALUE_GETTER(Corruption)
	GAMEPLAYATTRIBUTE_VALUE_SETTER(Corruption)
	GAMEPLAYATTRIBUTE_VALUE_INITTER(Corruption)

	/** Maximum corruption the Soul can accumulate before consequences. */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Corruption", ReplicatedUsing = OnRep_MaxCorruption)
	FGameplayAttributeData MaxCorruption;
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ULinkedSoulsAttributeSet, MaxCorruption)
	GAMEPLAYATTRIBUTE_VALUE_GETTER(MaxCorruption)
	GAMEPLAYATTRIBUTE_VALUE_SETTER(MaxCorruption)
	GAMEPLAYATTRIBUTE_VALUE_INITTER(MaxCorruption)

protected:

	// -- RepNotify handlers (drive the HUD bars) ----------------------------

	/** RepNotify for Health. */
	UFUNCTION()
	virtual void OnRep_Health(const FGameplayAttributeData& OldValue);

	/** RepNotify for MaxHealth. */
	UFUNCTION()
	virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);

	/** RepNotify for SoulEnergy. */
	UFUNCTION()
	virtual void OnRep_SoulEnergy(const FGameplayAttributeData& OldValue);

	/** RepNotify for MaxSoulEnergy. */
	UFUNCTION()
	virtual void OnRep_MaxSoulEnergy(const FGameplayAttributeData& OldValue);

	/** RepNotify for Corruption. */
	UFUNCTION()
	virtual void OnRep_Corruption(const FGameplayAttributeData& OldValue);

	/** RepNotify for MaxCorruption. */
	UFUNCTION()
	virtual void OnRep_MaxCorruption(const FGameplayAttributeData& OldValue);

private:

	/** Clamps a value into the [0, MaxValue] range. */
	static float ClampToRange(float Value, float MaxValue);
};
