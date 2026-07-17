// Copyright Epic Games, Inc. All Rights Reserved.


#include "BodyCharacter.h"
#include "SoulCharacter.h"
#include "SoulEnergy/SoulEnergyComponent.h"
#include "Elements/ElementComponent.h"
#include "HUD/LinkedSoulsHUD.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

// -- Engine includes (kept out of the header) -------------------------------
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "TimerManager.h"
#include "DualWorldManager.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GE_BodyMeleeDamage.h"
#include "Abilities/GE_BodySynergyBuff.h"

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

	// Rotation: face movement direction
	bUseControllerRotationYaw = false;

	// ElementComponent — Body uses physical elements
	ElementComponent = CreateDefaultSubobject<UElementComponent>(TEXT("ElementComponent"));
	ElementComponent->AllowedElements = { ELinkedSoulsElement::Fire, ELinkedSoulsElement::Water, ELinkedSoulsElement::Earth };
	ElementComponent->SetActiveElement(ELinkedSoulsElement::Fire);

	// Assign the Manny animation blueprint so the mesh plays locomotion
	static ConstructorHelpers::FClassFinder<UAnimInstance> ABP_Unarmed(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C"));
	if (ABP_Unarmed.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(ABP_Unarmed.Class);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("BodyCharacter: Failed to load ABP_Unarmed - T-pose will persist"));
	}

	// Load Body-specific Input Actions
	static ConstructorHelpers::FObjectFinder<UInputAction> WorldShiftAsset(TEXT("/Game/Input/Actions/IA_WorldShift.IA_WorldShift"));
	if (WorldShiftAsset.Succeeded()) WorldShiftAction = WorldShiftAsset.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> BodyMeleeAsset(TEXT("/Game/Input/Actions/IA_BodyMelee.IA_BodyMelee"));
	if (BodyMeleeAsset.Succeeded()) BodyMeleeAction = BodyMeleeAsset.Object;

	// -- Combat Juice: assign built-in attack montage ------------------------
	// The GameMode spawns ABodyCharacter directly from C++ (no Blueprint
	// subclass), so the montage is bound here via ConstructorHelpers — the
	// same pattern this class already uses for the mesh and input actions.
	static ConstructorHelpers::FObjectFinder<UAnimMontage> MeleeMontageAsset(
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01"));
	if (MeleeMontageAsset.Succeeded())
	{
		MeleeAttackMontage = MeleeMontageAsset.Object;
	}

	// MeleeSwingSound is left null — the project ships with no sound assets.
	// MulticastPlayMeleeAttack null-checks before playing, so this is safe.
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
		// Priority 2 > MouseLook(1) so Body actions win cleanly
		Subsystem->AddMappingContext(IMC, 2);
		UE_LOG(LogTemp, Warning, TEXT("BodyCharacter: IMC_Body added (priority 2, no IMC_Default stack)"));
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

	// then bind Body-specific input
	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (WorldShiftAction)
		{
			EnhancedInput->BindAction(WorldShiftAction, ETriggerEvent::Started, this, &ABodyCharacter::OnWorldShift);
		}

		if (BodyMeleeAction)
		{
			EnhancedInput->BindAction(BodyMeleeAction, ETriggerEvent::Started, this, &ABodyCharacter::OnBodyMelee);
		}
	}
}

// -- Combat -------------------------------------------------------------------

void ABodyCharacter::OnBodyMelee()
{
	Server_BodyMelee();
}

void ABodyCharacter::Server_BodyMelee_Implementation()
{
	PerformMeleeAttack();
}

bool ABodyCharacter::Server_BodyMelee_Validate()
{
	return true;
}

void ABodyCharacter::PerformMeleeAttack()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	// Combat juice: play the melee montage + swing sound on every machine.
	MulticastPlayMeleeAttack();

	FVector Start = GetActorLocation();
	FVector Forward = GetActorForwardVector();
	FVector End = Start + Forward * 600.0f;

	FCollisionShape SphereShape = FCollisionShape::MakeSphere(100.0f);
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	TArray<FHitResult> OutHits;
	GetWorld()->SweepMultiByChannel(OutHits, Start, End, FQuat::Identity, ECC_Pawn, SphereShape, QueryParams);

	bool bHitAnyTarget = false;
	TSet<AActor*> AlreadyHit;
	for (const FHitResult& Hit : OutHits)
	{
		AActor* Target = Hit.GetActor();
		if (!Target || AlreadyHit.Contains(Target))
		{
			continue;
		}
		AlreadyHit.Add(Target);

		IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Target);
		if (!ASI)
		{
			continue;
		}

		UAbilitySystemComponent* TargetASC = ASI->GetAbilitySystemComponent();
		if (!TargetASC)
		{
			continue;
		}

		FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
		EffectContext.AddInstigator(this, this);

		const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(
			ULS_GE_BodyMeleeDamage::StaticClass(), 1.0f, EffectContext);

		if (SpecHandle.Data.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
			bHitAnyTarget = true;
		}
	}

	if (bHitAnyTarget)
	{
		ASoulCharacter* Soul = GetLinkedSoul();
		if (Soul)
		{
			UAbilitySystemComponent* SoulASC = Soul->GetAbilitySystemComponent();
			if (SoulASC)
			{
				// Don't re-apply / spam while synergy is already active
				static const FGameplayTag SynergyTag =
					FGameplayTag::RequestGameplayTag(FName("Linked.Synergy.Active"));
				if (SoulASC->HasMatchingGameplayTag(SynergyTag))
				{
					return;
				}

				FGameplayEffectContextHandle SyncCtx = ASC->MakeEffectContext();
				SyncCtx.AddInstigator(this, this);

				const FGameplayEffectSpecHandle SyncSpec = ASC->MakeOutgoingSpec(
					ULS_GE_BodySynergyBuff::StaticClass(), 1.0f, SyncCtx);

				if (SyncSpec.Data.IsValid())
				{
					ASC->ApplyGameplayEffectSpecToTarget(*SyncSpec.Data.Get(), SoulASC);
					UE_LOG(LogTemp, Warning, TEXT("LinkedSouls: Synergy buff applied to Soul (10s)"));
				}
			}
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

// -- Combat Juice ------------------------------------------------------------

void ABodyCharacter::MulticastPlayMeleeAttack_Implementation()
{
	if (MeleeAttackMontage && GetMesh() && GetMesh()->GetAnimInstance())
	{
		GetMesh()->GetAnimInstance()->Montage_Play(MeleeAttackMontage, 1.0f);
	}

	if (MeleeSwingSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, MeleeSwingSound, GetActorLocation());
	}
}
