// Copyright Epic Games, Inc. All Rights Reserved.


#include "LinkedSoulsPlayerCharacter.h"
#include "LinkedSoulsAttributeSet.h"

// -- Engine includes (kept out of the header) -------------------------------
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
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

	// Character rotates to face movement direction (standard third-person feel)
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 600.0f;

	// Position the mesh so the capsule base touches the ground
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));

	// Load shared Input Actions so they are non-null when SetupPlayerInputComponent binds them
	static ConstructorHelpers::FObjectFinder<UInputAction> MoveAsset(TEXT("/Game/Input/Actions/IA_Move.IA_Move"));
	if (MoveAsset.Succeeded()) MoveAction = MoveAsset.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> JumpAsset(TEXT("/Game/Input/Actions/IA_Jump.IA_Jump"));
	if (JumpAsset.Succeeded()) JumpAction = JumpAsset.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> LookAsset(TEXT("/Game/Input/Actions/IA_Look.IA_Look"));
	if (LookAsset.Succeeded()) LookAction = LookAsset.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> MouseLookAsset(TEXT("/Game/Input/Actions/IA_MouseLook.IA_MouseLook"));
	if (MouseLookAsset.Succeeded()) MouseLookAction = MouseLookAsset.Object;
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
		if (MouseLookAction)
		{
			EnhancedInput->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ALinkedSoulsPlayerCharacter::Look);
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
	if (Controller)
	{
		// camera-relative movement: WASD aligns to camera facing, not actor facing
		const FRotator YawRotation(0, Controller->GetControlRotation().Yaw, 0);
		const FVector ForwardDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(ForwardDir, Forward);
		AddMovementInput(RightDir, Right);
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

// -- Co-op partner -----------------------------------------------------------

void ALinkedSoulsPlayerCharacter::SetLinkedPartner(ALinkedSoulsPlayerCharacter* Partner)
{
	LinkedPartner = Partner;
}

ALinkedSoulsPlayerCharacter* ALinkedSoulsPlayerCharacter::GetLinkedPartner() const
{
	return LinkedPartner.Get();
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

	// Deferred IMC setup: in multiplayer BeginPlay fires before possession,
	// so GetController() is null. A short timer retries after possession.
	GetWorldTimerManager().SetTimer(DelayedIMCSetupTimer, this,
		&ALinkedSoulsPlayerCharacter::AddInputContexts, 0.1f, false);
}

void ALinkedSoulsPlayerCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();

	// Client path: controller just replicated, add input contexts now.
	AddInputContexts();
}

void ALinkedSoulsPlayerCharacter::AddInputContexts()
{
	// Prevent double-add if multiple paths fire (e.g. PossessedBy + timer).
	if (bInputContextsAdded)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		// Not possessed yet — timer (or future call via PossessedBy / OnRep) will retry.
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(
			PC->GetLocalPlayer());
	if (!Subsystem)
	{
		// Not a local player (e.g. this Pawn is controlled by a remote client on the server).
		return;
	}

	bInputContextsAdded = true;
	GetWorldTimerManager().ClearTimer(DelayedIMCSetupTimer);

	UInputMappingContext* IMC_Default = LoadObject<UInputMappingContext>(nullptr,
		TEXT("/Game/Input/IMC_Default.IMC_Default"));
	if (IMC_Default)
	{
		Subsystem->AddMappingContext(IMC_Default, 0);
	}

	UInputMappingContext* IMC_MouseLook = LoadObject<UInputMappingContext>(nullptr,
		TEXT("/Game/Input/IMC_MouseLook.IMC_MouseLook"));
	if (IMC_MouseLook)
	{
		Subsystem->AddMappingContext(IMC_MouseLook, 1);
	}
}

void ALinkedSoulsPlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// server-side GAS initialization
	InitAbilitySystem();

	// Server path: controller is now valid. For listen-server players this
	// also adds the input mapping contexts immediately.
	AddInputContexts();
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

void ALinkedSoulsPlayerCharacter::OnCharacterDeath()
{
	Destroy();
}
