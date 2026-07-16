// Copyright Epic Games, Inc. All Rights Reserved.


#include "LinkedSoulsAttributeSet.h"
#include "LinkedSoulsPlayerCharacter.h"
#include "Enemies/BaseEnemy.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

ULinkedSoulsAttributeSet::ULinkedSoulsAttributeSet()
{
	// reasonable starting pools; designers can override via effects on the ASC
	InitHealth(100.0f);
	InitMaxHealth(100.0f);
	InitSoulEnergy(50.0f);
	InitMaxSoulEnergy(100.0f);
	InitCorruption(0.0f);
	InitMaxCorruption(100.0f);
}

void ULinkedSoulsAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	// clamp the changing attribute so it never leaves a valid range
	if (Attribute == GetHealthAttribute())
	{
		NewValue = ClampToRange(NewValue, GetMaxHealth());
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(0.0f, NewValue);
	}
	else if (Attribute == GetSoulEnergyAttribute())
	{
		NewValue = ClampToRange(NewValue, GetMaxSoulEnergy());
	}
	else if (Attribute == GetMaxSoulEnergyAttribute())
	{
		NewValue = FMath::Max(0.0f, NewValue);
	}
	else if (Attribute == GetCorruptionAttribute())
	{
		NewValue = ClampToRange(NewValue, GetMaxCorruption());
	}
	else if (Attribute == GetMaxCorruptionAttribute())
	{
		NewValue = FMath::Max(0.0f, NewValue);
	}
}

void ULinkedSoulsAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// after an effect lands, re-clamp the dependent current attributes
	const float OldHealth = GetHealth();
	const float OldSoulEnergy = GetSoulEnergy();
	const float OldCorruption = GetCorruption();

	SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	SetSoulEnergy(FMath::Clamp(GetSoulEnergy(), 0.0f, GetMaxSoulEnergy()));
	SetCorruption(FMath::Clamp(GetCorruption(), 0.0f, GetMaxCorruption()));

	// death detection
	AActor* Owner = GetOwningActor();
	if (!Owner || Owner->IsA<ABaseEnemy>())
	{
		return;
	}

	ALinkedSoulsPlayerCharacter* PlayerOwner = Cast<ALinkedSoulsPlayerCharacter>(Owner);

	// Health dropped to zero
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		if (GetHealth() <= 0.0f && OldHealth > 0.0f)
		{
			UE_LOG(LogTemp, Warning, TEXT("LinkedSouls: %s killed — Health reached 0"), *Owner->GetName());
			if (PlayerOwner)
			{
				PlayerOwner->OnCharacterDeath();
			}
			else
			{
				Owner->Destroy();
			}
		}
	}

	// Corruption reached maximum
	if (Data.EvaluatedData.Attribute == GetCorruptionAttribute())
	{
		if (GetCorruption() >= GetMaxCorruption() && OldCorruption < GetMaxCorruption())
		{
			UE_LOG(LogTemp, Warning, TEXT("LinkedSouls: Soul overwhelmed by Corruption"));
			if (PlayerOwner)
			{
				PlayerOwner->OnCharacterDeath();
			}
		}
	}
}

// -- Replication --------------------------------------------------------------

void ULinkedSoulsAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ULinkedSoulsAttributeSet, Health, OldValue);
}

void ULinkedSoulsAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ULinkedSoulsAttributeSet, MaxHealth, OldValue);
}

void ULinkedSoulsAttributeSet::OnRep_SoulEnergy(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ULinkedSoulsAttributeSet, SoulEnergy, OldValue);
}

void ULinkedSoulsAttributeSet::OnRep_MaxSoulEnergy(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ULinkedSoulsAttributeSet, MaxSoulEnergy, OldValue);
}

void ULinkedSoulsAttributeSet::OnRep_Corruption(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ULinkedSoulsAttributeSet, Corruption, OldValue);
}

void ULinkedSoulsAttributeSet::OnRep_MaxCorruption(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ULinkedSoulsAttributeSet, MaxCorruption, OldValue);
}

// -- Internal helpers ---------------------------------------------------------

float ULinkedSoulsAttributeSet::ClampToRange(float Value, float MaxValue)
{
	// floor at zero, ceiling at the supplied max
	return FMath::Clamp(Value, 0.0f, FMath::Max(0.0f, MaxValue));
}

void ULinkedSoulsAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// replicate every attribute to clients so the HUD stays in sync
	DOREPLIFETIME(ULinkedSoulsAttributeSet, Health);
	DOREPLIFETIME(ULinkedSoulsAttributeSet, MaxHealth);
	DOREPLIFETIME(ULinkedSoulsAttributeSet, SoulEnergy);
	DOREPLIFETIME(ULinkedSoulsAttributeSet, MaxSoulEnergy);
	DOREPLIFETIME(ULinkedSoulsAttributeSet, Corruption);
	DOREPLIFETIME(ULinkedSoulsAttributeSet, MaxCorruption);
}
