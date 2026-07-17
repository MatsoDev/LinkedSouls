// Copyright Epic Games, Inc. All Rights Reserved.


#include "SoulCharacter.h"
#include "BodyCharacter.h"
#include "SoulEnergy/SoulEnergyComponent.h"
#include "Elements/ElementComponent.h"
#include "HUD/LinkedSoulsHUD.h"

// -- Engine includes (kept out of the header) -------------------------------
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/SkeletalMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "TimerManager.h"
#include "DualWorldManager.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GE_SpiritAttackDamage.h"
#include "Abilities/GE_SpiritAttackSynergyDamage.h"
#include "Abilities/GE_BodySynergyBuff.h"
#include "Abilities/GE_SoulPulseDamage.h"
#include "Abilities/GE_CorruptionDecay.h"

ASoulCharacter::ASoulCharacter()
{
	/*
	 * TEAM ASSET HANDOFF
	 * System: Character
	 * Replace with: Soul hero mesh (الروح)
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
			UE_LOG(LogTemp, Error, TEXT("SoulCharacter: Failed to load placeholder mesh - check Manny path"));
		}
	}

	// Soul drifts - low gravity for ethereal movement
	GetCharacterMovement()->GravityScale = 0.3f;
	GetCharacterMovement()->JumpZVelocity = 150.f;
	GetCharacterMovement()->AirControl = 0.8f;
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);

	// Rotation: face movement direction
	bUseControllerRotationYaw = false;

	// Pawn profile: blocks world static (floor, walls) so Soul stands on solid ground.
	// If spirit-phasing through enemies is needed later, create a custom collision
	// profile that blocks WorldStatic but overlaps Pawn.
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));

	// ElementComponent — Soul uses spiritual elements
	ElementComponent = CreateDefaultSubobject<UElementComponent>(TEXT("ElementComponent"));
	ElementComponent->AllowedElements = { ELinkedSoulsElement::Light, ELinkedSoulsElement::Shadow, ELinkedSoulsElement::Void };
	ElementComponent->SetActiveElement(ELinkedSoulsElement::Light);

	// Assign the Manny animation blueprint so the mesh plays locomotion
	static ConstructorHelpers::FClassFinder<UAnimInstance> ABP_Unarmed(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C"));
	if (ABP_Unarmed.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(ABP_Unarmed.Class);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("SoulCharacter: Failed to load ABP_Unarmed - T-pose will persist"));
	}

	// Load Soul-specific Input Actions
	static ConstructorHelpers::FObjectFinder<UInputAction> ManifestAsset(TEXT("/Game/Input/Actions/IA_Manifest.IA_Manifest"));
	if (ManifestAsset.Succeeded()) ManifestAction = ManifestAsset.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> SpiritAttackAsset(TEXT("/Game/Input/Actions/IA_SpiritAttack.IA_SpiritAttack"));
	if (SpiritAttackAsset.Succeeded()) SpiritAttackAction = SpiritAttackAsset.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> SoulPulseAsset(TEXT("/Game/Input/Actions/IA_SoulPulse.IA_SoulPulse"));
	if (SoulPulseAsset.Succeeded()) SoulPulseAction = SoulPulseAsset.Object;
}

void ASoulCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(CorruptionDecayTimer, this,
			&ASoulCharacter::TickCorruptionDecay, 2.0f, true);
	}
}

void ASoulCharacter::AddInputContexts()
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
		TEXT("/Game/Input/IMC_Soul.IMC_Soul"));
	if (IMC)
	{
		// Priority 2 > MouseLook(1) so Soul actions win cleanly
		Subsystem->AddMappingContext(IMC, 2);
		UE_LOG(LogTemp, Warning, TEXT("SoulCharacter: IMC_Soul added (priority 2, no IMC_Default stack)"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("SoulCharacter: IMC_Soul not found!"));
	}
}

// -- Mesh + material ---------------------------------------------------------

void ASoulCharacter::ConfigureMesh()
{
	// refuse to build a MID if the mesh is missing
	if (!GetMesh() || !GetMesh()->GetSkeletalMeshAsset())
	{
		UE_LOG(LogTemp, Error, TEXT("SoulCharacter: Mesh not set - MID creation skipped"));
		return;
	}

	// translucent blue tint that marks the Soul as a spirit
	const FLinearColor SoulTint(0.2f, 0.5f, 1.0f, 0.5f);

	// create a dynamic material instance from the mesh's material slot 0
	SoulMaterialInstance = UMaterialInstanceDynamic::Create(GetMesh()->GetMaterial(0), this);
	if (SoulMaterialInstance)
	{
		// NOTE: If Manny material has no "Tint"/"Opacity" params this is a no-op - safe.
		// Replace material with team Soul material that exposes these params when art is ready.
		SoulMaterialInstance->SetVectorParameterValue(TEXT("Tint"), SoulTint);
		SoulMaterialInstance->SetScalarParameterValue(TEXT("Opacity"), SoulTint.A);

		// apply the tinted material back onto the mesh
		GetMesh()->SetMaterial(0, SoulMaterialInstance);
	}
}

// -- Co-op link --------------------------------------------------------------

void ASoulCharacter::SetLinkedBody(ABodyCharacter* InBody)
{
	LinkedBody = InBody;
}

ABodyCharacter* ASoulCharacter::GetLinkedBody() const
{
	return LinkedBody.Get();
}

// -- Manifest (Soul-only ability, mirrors Body's World Shift) ----------------

void ASoulCharacter::OnManifest()
{
	// refuse to re-trigger while already manifested
	if (bIsManifested)
	{
		return;
	}

	EnterRealWorld();
}

void ASoulCharacter::OnSpiritAttack()
{
	Server_SpiritAttack();
}

void ASoulCharacter::Server_SpiritAttack_Implementation()
{
	USoulEnergyComponent* SEC = USoulEnergyComponent::GetSoulEnergyComponent(this);
	if (!SEC || !SEC->ConsumeSoulEnergy(15.0f))
	{
		UE_LOG(LogTemp, Warning, TEXT("SoulCharacter: SpiritAttack blocked — insufficient SoulEnergy"));
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	const bool bHasSynergy = ASC->HasMatchingGameplayTag(
		FGameplayTag::RequestGameplayTag(FName("Linked.Synergy.Active")));
	if (bHasSynergy)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpiritAttack: Synergy bonus active — 45 damage"));
	}

	FVector Start = GetActorLocation();
	FVector End = Start + GetActorForwardVector() * 800.0f;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	FHitResult Hit;
	GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Pawn, QueryParams);

	AActor* Target = Hit.GetActor();
	if (!Target)
	{
		return;
	}

	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Target);
	if (!ASI)
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = ASI->GetAbilitySystemComponent();
	if (!TargetASC)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddInstigator(this, this);

	TSubclassOf<UGameplayEffect> DamageGE = bHasSynergy
		? ULS_GE_SpiritAttackSynergyDamage::StaticClass()
		: ULS_GE_SpiritAttackDamage::StaticClass();

	const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(
		DamageGE, 1.0f, EffectContext);

	if (SpecHandle.Data.IsValid())
	{
		ASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	}
}

bool ASoulCharacter::Server_SpiritAttack_Validate()
{
	return true;
}

void ASoulCharacter::OnSoulPulse()
{
	Server_SoulPulse();
}

void ASoulCharacter::Server_SoulPulse_Implementation()
{
	USoulEnergyComponent* SEC = USoulEnergyComponent::GetSoulEnergyComponent(this);
	if (!SEC || !SEC->ConsumeSoulEnergy(20.0f))
	{
		UE_LOG(LogTemp, Warning, TEXT("SoulCharacter: SoulPulse blocked — insufficient SoulEnergy"));
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionShape SphereShape = FCollisionShape::MakeSphere(400.0f);
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity, FCollisionObjectQueryParams(ECC_TO_BITFIELD(ECC_Pawn)), SphereShape, QueryParams);

	TSet<AActor*> AlreadyAffected;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Target = Overlap.GetActor();
		if (!Target || AlreadyAffected.Contains(Target))
		{
			continue;
		}
		AlreadyAffected.Add(Target);

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
			ULS_GE_SoulPulseDamage::StaticClass(), 1.0f, EffectContext);

		if (SpecHandle.Data.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
		}
	}
}

bool ASoulCharacter::Server_SoulPulse_Validate()
{
	return true;
}

void ASoulCharacter::EnterRealWorld()
{
	// resolve the shared soul energy pool
	USoulEnergyComponent* SEC = USoulEnergyComponent::GetSoulEnergyComponent(this);
	if (!SEC)
	{
		UE_LOG(LogTemp, Error, TEXT("SoulCharacter: SoulEnergyComponent not found - Manifest blocked"));
		return;
	}
	if (!SEC->CanAffordAbility())
	{
		UE_LOG(LogTemp, Warning, TEXT("SoulCharacter: Manifest blocked - insufficient SoulEnergy (%.1f)"), SEC->GetSoulEnergy());
		return;
	}

	// warn if the co-op link is not established yet
	if (!LinkedBody.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("SoulCharacter: Manifest triggered but no LinkedBody assigned"));
	}

	bIsManifested = true;

	// notify HUD that Soul has manifested into Real World
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

	// pay the flat ability cost and start continuous drain while manifested
	SEC->DrainEnergy(SEC->AbilityFlatCost);
	SEC->SetContinuousDrain(true);

	// become visible in both worlds while manifested (lets Soul see Body)
	if (UDualWorldManager* Manager = UDualWorldManager::GetDualWorldManager(this))
	{
		Manager->SetActorPresence(this, EWorldPresence::BothWorlds);
	}

	// start the temporary manifest duration
	GetWorldTimerManager().SetTimer(ManifestTimer, this, &ASoulCharacter::EndManifest, ManifestDuration, false);
}

void ASoulCharacter::EndManifest()
{
	// stop the continuous drain when the manifest ends
	if (USoulEnergyComponent* SEC = USoulEnergyComponent::GetSoulEnergyComponent(this))
	{
		SEC->SetContinuousDrain(false);
	}

	bIsManifested = false;

	// notify HUD that Soul has returned to Spirit World
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

	// Soul returns to Spirit - opposite of Body's RealOnly
	if (UDualWorldManager* Manager = UDualWorldManager::GetDualWorldManager(this))
	{
		Manager->SetActorPresence(this, EWorldPresence::SpiritOnly);
	}

	// clear the timer handle
	GetWorldTimerManager().ClearTimer(ManifestTimer);
}

// -- Combat ------------------------------------------------------------------

void ASoulCharacter::TickCorruptionDecay()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddInstigator(this, this);

	const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(
		ULS_GE_CorruptionDecay::StaticClass(), 1.0f, EffectContext);

	if (SpecHandle.Data.IsValid())
	{
		ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

void ASoulCharacter::OnCorruptionDamage(float Amount)
{
	// TODO: Apply GAS GameplayEffect for Corruption in System 4
	UE_LOG(LogTemp, Warning, TEXT("SoulCharacter: Corruption %.1f - GAS GE not wired yet (System 4)"), Amount);
}

// -- Input -------------------------------------------------------------------

void ASoulCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// bind the shared base actions first
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// then bind Soul-specific input
	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (ManifestAction)
		{
			EnhancedInput->BindAction(ManifestAction, ETriggerEvent::Started, this, &ASoulCharacter::OnManifest);
		}

		if (SpiritAttackAction)
		{
			EnhancedInput->BindAction(SpiritAttackAction, ETriggerEvent::Started, this, &ASoulCharacter::OnSpiritAttack);
		}

		if (SoulPulseAction)
		{
			EnhancedInput->BindAction(SoulPulseAction, ETriggerEvent::Started, this, &ASoulCharacter::OnSoulPulse);
		}
	}
}

// [end of file]
