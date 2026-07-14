// Copyright Epic Games, Inc. All Rights Reserved.


#include "LinkedSoulsGameState.h"
#include "SoulEnergy/SoulEnergyComponent.h"
#include "DualWorldManager.h"
#include "GameFramework/Character.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Abilities/GE_EnergyDepleted.h"
#include "Abilities/GE_EnergyFull.h"

ALinkedSoulsGameState::ALinkedSoulsGameState()
{
	// create the shared soul energy pool
	SoulEnergyComponent = CreateDefaultSubobject<USoulEnergyComponent>(TEXT("SoulEnergyComponent"));
}

void ALinkedSoulsGameState::BeginPlay()
{
	Super::BeginPlay();

	// bind the soul energy delegates to apply GAS effects
	if (SoulEnergyComponent)
	{
		SoulEnergyComponent->OnEnergyDepleted.AddDynamic(this, &ALinkedSoulsGameState::OnEnergyDepleted);
		SoulEnergyComponent->OnEnergyFull.AddDynamic(this, &ALinkedSoulsGameState::OnEnergyFull);
	}
}

void ALinkedSoulsGameState::OnEnergyDepleted()
{
	ApplyGEToPlayers(GE_EnergyDepleted);
}

void ALinkedSoulsGameState::OnEnergyFull()
{
	ApplyGEToPlayers(GE_EnergyFull);
}

void ALinkedSoulsGameState::ApplyGEToPlayers(TSubclassOf<UGameplayEffect> GEClass)
{
	if (!GEClass)
	{
		return;
	}

	UDualWorldManager* DWM = UDualWorldManager::GetDualWorldManager(this);
	if (!DWM)
	{
		return;
	}

	// apply to Body
	if (ACharacter* Body = DWM->GetBody())
	{
		if (UAbilitySystemComponent* ASC = Body->FindComponentByClass<UAbilitySystemComponent>())
		{
			if (const UGameplayEffect* GE = GEClass.GetDefaultObject())
			{
				ASC->ApplyGameplayEffectToSelf(GE, 1.0f, ASC->MakeEffectContext());
			}
		}
	}

	// apply to Soul
	if (ACharacter* Soul = DWM->GetSoul())
	{
		if (UAbilitySystemComponent* ASC = Soul->FindComponentByClass<UAbilitySystemComponent>())
		{
			if (const UGameplayEffect* GE = GEClass.GetDefaultObject())
			{
				ASC->ApplyGameplayEffectToSelf(GE, 1.0f, ASC->MakeEffectContext());
			}
		}
	}
}
