// Copyright Epic Games, Inc. All Rights Reserved.


#include "BodyCharacter.h"
#include "SoulCharacter.h"
#include "SoulEnergy/SoulEnergyComponent.h"
#include "Elements/ElementComponent.h"
#include "HUD/LinkedSoulsHUD.h"

// -- Engine includes (kept out of the header) -------------------------------
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "TimerManager.h"
#include "DualWorldManager.h"

ABodyCharacter::ABodyCharacter()
{
	/*
	 * TEAM ASSET HANDOFF
	 * System: Character
	 * Replace with: Body hero mesh (البطل)
	 * Delivered by:
	 * Format: .uasset (SkeletalMesh)
	 * Priority: High
	 */
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MannySkelMesh(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"));

	if (MannySkelMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(MannySkelMesh.Object);
	}
	else
	{
		// fallback: try the full Manny mesh before giving up
		static ConstructorHelpers::FObjectFinder<USkeletalMesh> MannyFallback(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny"));
		if (MannyFallback.Succeeded())
		{
			GetMesh()->SetSkeletalMesh(MannyFallback.Object);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("BodyCharacter: Failed to load placeholder mesh - check Manny path"));
		}
	}

	// ElementComponent — Body uses physical elements
	ElementComponent = CreateDefaultSubobject<UElementComponent>(TEXT("ElementComponent"));
	ElementComponent->AllowedElements = { ELinkedSoulsElement::Fire, ELinkedSoulsElement::Water, ELinkedSoulsElement::Earth };
	ElementComponent->SetActiveElement(ELinkedSoulsElement::Fire);

	// Load Body-specific Input Actions
	static ConstructorHelpers::FObjectFinder<UInputAction> WorldShiftAsset(TEXT("/Game/Input/Actions/IA_WorldShift.IA_WorldShift"));
	if (WorldShiftAsset.Succeeded()) WorldShiftAction = WorldShiftAsset.Object;
}

void ABodyCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void ABodyCharacter::AddInputContexts()
{
	Super::AddInputContexts();

	if (!bInputContextsAdded)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(
			PC->GetLocalPlayer());
	if (!Subsystem) return;

	UInputMappingContext* IMC = LoadObject<UInputMappingContext>(nullptr,
		TEXT("/Game/Input/IMC_Body.IMC_Body"));
	if (IMC)
	{
		Subsystem->AddMappingContext(IMC, 0);
		UE_LOG(LogTemp, Warning, TEXT("BodyCharacter: IMC_Body added"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("BodyCharacter: IMC_Body not found!"));
	}
}

void ABodyCharacter::ConfigureMesh()
{
	// Body uses the stock Manny mesh assigned in the constructor.
	// No runtime mesh configuration is needed (no tint, no gravity, no collision override).
}

// -- World Shift -------------------------------------------------------------

void ABodyCharacter::OnWorldShift()
{
	// refuse to re-trigger while already shifted
	if (bIsInSpiritWorld)
	{
		return;
	}

	EnterSpiritWorld();
}

void ABodyCharacter::EnterSpiritWorld()
{
	// resolve the shared soul energy pool
	USoulEnergyComponent* SEC = USoulEnergyComponent::GetSoulEnergyComponent(this);
	if (!SEC)
	{
		UE_LOG(LogTemp, Error, TEXT("BodyCharacter: SoulEnergyComponent not found - WorldShift blocked"));
		return;
	}
	if (!SEC->CanAffordAbility())
	{
		UE_LOG(LogTemp, Warning, TEXT("BodyCharacter: WorldShift blocked - insufficient SoulEnergy (%.1f)"), SEC->GetSoulEnergy());
		return;
	}

	// warn if the co-op link is not established yet
	if (!LinkedSoul.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("BodyCharacter: WorldShift triggered but no LinkedSoul assigned"));
	}

	bIsInSpiritWorld = true;

	// notify HUD that Body has entered Spirit World
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (ALinkedSoulsHUD* HUD = Cast<ALinkedSoulsHUD>(PC->GetHUD()))
		{
			if (HUD->HUDWidget)
			{
				HUD->HUDWidget->UpdateWorldIndicator(true);
			}
		}
	}

	// pay the flat ability cost and start continuous drain while shifted
	SEC->DrainEnergy(SEC->AbilityFlatCost);
	SEC->SetContinuousDrain(true);

	// become visible in both worlds while shifted (lets Body see Soul)
	if (UDualWorldManager* Manager = UDualWorldManager::GetDualWorldManager(this))
	{
		Manager->SetActorPresence(this, EWorldPresence::BothWorlds);
	}

	// start the temporary shift duration
	GetWorldTimerManager().SetTimer(WorldShiftTimer, this, &ABodyCharacter::EndWorldShift, WorldShiftDuration, false);
}

void ABodyCharacter::EndWorldShift()
{
	// stop the continuous drain when the shift ends
	if (USoulEnergyComponent* SEC = USoulEnergyComponent::GetSoulEnergyComponent(this))
	{
		SEC->SetContinuousDrain(false);
	}

	bIsInSpiritWorld = false;

	// notify HUD that Body has returned to Real World
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (ALinkedSoulsHUD* HUD = Cast<ALinkedSoulsHUD>(PC->GetHUD()))
		{
			if (HUD->HUDWidget)
			{
				HUD->HUDWidget->UpdateWorldIndicator(false);
			}
		}
	}

	// return to Real-only presence
	if (UDualWorldManager* Manager = UDualWorldManager::GetDualWorldManager(this))
	{
		Manager->SetActorPresence(this, EWorldPresence::RealOnly);
	}

	// clear the timer handle
	GetWorldTimerManager().ClearTimer(WorldShiftTimer);
}

// -- Input -------------------------------------------------------------------

void ABodyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// bind the shared base actions first
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// then bind Body-specific input (World Shift)
	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (WorldShiftAction)
		{
			EnhancedInput->BindAction(WorldShiftAction, ETriggerEvent::Started, this, &ABodyCharacter::OnWorldShift);
		}
	}
}

// -- Co-op link --------------------------------------------------------------

void ABodyCharacter::SetLinkedSoul(ASoulCharacter* InSoul)
{
	LinkedSoul = InSoul;
}

ASoulCharacter* ABodyCharacter::GetLinkedSoul() const
{
	return LinkedSoul.Get();
}
