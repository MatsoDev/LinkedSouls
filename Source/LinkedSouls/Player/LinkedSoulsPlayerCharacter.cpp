// Copyright Epic Games, Inc. All Rights Reserved.


#include "LinkedSoulsPlayerCharacter.h"
#include "LinkedSoulsAttributeSet.h"

// -- Engine includes (kept out of the header) -------------------------------
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "AbilitySystemComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"

ALinkedSoulsPlayerCharacter::ALinkedSoulsPlayerCharacter()
{
	// no ticking needed for the base character
	PrimaryActorTick.bCanEverTick = false;

	// co-op character: must replicate to the other player
	bReplicates = true;

	// camera boom, positioned behind the character
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// follow camera attached to the boom
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// GAS ability system component, replicated for co-op
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	// attribute set backing Health / SoulEnergy / Corruption
	AttributeSet = CreateDefaultSubobject<ULinkedSoulsAttributeSet>(TEXT("AttributeSet"));
}

// -- IAbilitySystemInterface -------------------------------------------------

UAbilitySystemComponent* ALinkedSoulsPlayerCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

// -- Element slots -----------------------------------------------------------

bool ALinkedSoulsPlayerCharacter::EquipElement(ELinkedSoulsElement Element)
{
	// refuse if both slots are already taken
	if (EquippedElements.Num() >= MaxEquippedElements)
	{
		return false;
	}

	// refuse duplicates
	if (EquippedElements.Contains(Element))
	{
		return false;
	}

	EquippedElements.Add(Element);
	return true;
}

bool ALinkedSoulsPlayerCharacter::UnequipElement(ELinkedSoulsElement Element)
{
	// remove the first occurrence; reports whether it was actually equipped
	return EquippedElements.RemoveSingle(Element) > 0;
}

// -- Input -------------------------------------------------------------------

void ALinkedSoulsPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// only bind if this is an Enhanced Input component
	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// jump
		if (JumpAction)
		{
			EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &ALinkedSoulsPlayerCharacter::DoJumpStart);
			EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &ALinkedSoulsPlayerCharacter::DoJumpEnd);
		}

		// move + look
		if (MoveAction)
		{
			EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ALinkedSoulsPlayerCharacter::Move);
		}
		if (LookAction)
		{
			EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &ALinkedSoulsPlayerCharacter::Look);
		}
	}
}

void ALinkedSoulsPlayerCharacter::Move(const FInputActionValue& Value)
{
	// unpack a 2D input into right / forward and forward to DoMove
	const FVector2D MovementVector = Value.Get<FVector2D>();
	DoMove(MovementVector.X, MovementVector.Y);
}

void ALinkedSoulsPlayerCharacter::Look(const FInputActionValue& Value)
{
	// unpack a 2D input into yaw / pitch and forward to DoLook
	const FVector2D LookAxisVector = Value.Get<FVector2D>();
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void ALinkedSoulsPlayerCharacter::DoMove(float Right, float Forward)
{
	// only move while we have a controller
	if (Controller)
	{
		AddMovementInput(GetActorRightVector(), Right);
		AddMovementInput(GetActorForwardVector(), Forward);
	}
}

void ALinkedSoulsPlayerCharacter::DoLook(float Yaw, float Pitch)
{
	// only rotate while we have a controller
	if (Controller)
	{
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void ALinkedSoulsPlayerCharacter::DoJumpStart()
{
	// jump pressed
	Jump();
}

void ALinkedSoulsPlayerCharacter::DoJumpEnd()
{
	// jump released
	StopJumping();
}

// -- Lifecycle ---------------------------------------------------------------

void ALinkedSoulsPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// subclass-specific mesh / material / physics setup
	ConfigureMesh();

	// register this player with the dual-world manager (System 1 API)
	if (UDualWorldManager* Manager = UDualWorldManager::GetDualWorldManager(this))
	{
		if (GetPlayerWorld() == EDualWorld::RealWorld)
		{
			Manager->RegisterBody(this);
		}
		else
		{
			Manager->RegisterSoul(this);
		}
	}
}

void ALinkedSoulsPlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// server-side GAS initialization
	InitAbilitySystem();
}

void ALinkedSoulsPlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// client-side GAS initialization
	InitAbilitySystem();
}

void ALinkedSoulsPlayerCharacter::InitAbilitySystem()
{
	// guard against double initialization or a missing component
	if (!AbilitySystemComponent || AbilitySystemComponent->AbilityActorInfo.IsValid())
	{
		return;
	}

	// UE5.8 GAS API: the character is both owner and avatar of its own ASC
	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	// verify the attribute set actually registered on the ASC
	const TArray<UAttributeSet*>& SpawnedAttributes = AbilitySystemComponent->GetSpawnedAttributes();
	if (SpawnedAttributes.Find(AttributeSet) == INDEX_NONE)
	{
		UE_LOG(LogTemp, Error, TEXT("InitAbilitySystem: AttributeSet not found on ASC for %s"), *GetName());
	}

	// confirm initialization in the output log for testing
	UE_LOG(LogTemp, Warning, TEXT("ASC Initialized for %s"), *GetName());
}
