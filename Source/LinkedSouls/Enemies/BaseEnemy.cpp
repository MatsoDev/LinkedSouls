// Copyright Epic Games, Inc. All Rights Reserved.


#include "Enemies/BaseEnemy.h"
#include "Player/LinkedSoulsAttributeSet.h"
#include "Player/LinkedSoulsPlayerCharacter.h"
#include "Player/BodyCharacter.h"
#include "Player/SoulCharacter.h"
#include "UI/EnemyHealthBarWidget.h"
#include "AI/LinkedSoulsAIController.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GE_CorruptionDamage.h"
#include "Abilities/GE_BodyMeleeDamage.h"
#include "BehaviorTree/BehaviorTree.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/SkeletalMesh.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/CapsuleComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"
#include "Blueprint/UserWidget.h"

ABaseEnemy::ABaseEnemy()
{
	bReplicates = true;

	PrimaryActorTick.bCanEverTick = false;

	AIControllerClass = ALinkedSoulsAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	static ConstructorHelpers::FObjectFinder<UBehaviorTree> BTAsset(
		TEXT("/Game/AI/BT_LinkedSoulsEnemy.BT_LinkedSoulsEnemy"));
	if (BTAsset.Succeeded())
	{
		BehaviorTree = BTAsset.Object;
	}

	CurrentPhysicalHP = PhysicalHP;
	CurrentSpiritHP = SpiritHP;
	CurrentState = EEnemyState::Alive;

	/*
	 * TEAM ASSET HANDOFF
	 * System: Enemy
	 * Replace with: enemy creature mesh
	 * Delivered by:
	 * Format: .uasset (SkeletalMesh)
	 * Priority: Medium
	 */
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MannySkelMesh(
		TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"));

	if (MannySkelMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(MannySkelMesh.Object);
	}
	else
	{
		static ConstructorHelpers::FObjectFinder<USkeletalMesh> MannyFallback(
			TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny"));
		if (MannyFallback.Succeeded())
		{
			GetMesh()->SetSkeletalMesh(MannyFallback.Object);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("BaseEnemy: Failed to load placeholder mesh - check Manny path"));
		}
	}

	// Manny faces +Y; capsule forward is +X — yaw -90 so mesh faces movement direction
	GetMesh()->SetRelativeLocationAndRotation(
		FVector(0.f, 0.f, -96.f),
		FRotator(0.f, -90.f, 0.f));

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<ULinkedSoulsAttributeSet>(TEXT("AttributeSet"));

	HealthBarComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBar"));
	HealthBarComponent->SetupAttachment(RootComponent);
	HealthBarComponent->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
	HealthBarComponent->SetWidgetSpace(EWidgetSpace::Screen);
	HealthBarComponent->SetDrawSize(FVector2D(100.f, 15.f));
	HealthBarComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EnemyHealthBarClass = UEnemyHealthBarWidget::StaticClass();
	HealthBarComponent->SetWidgetClass(UEnemyHealthBarWidget::StaticClass());
}

// -- Lifecycle ---------------------------------------------------------------

void ABaseEnemy::BeginPlay()
{
	Super::BeginPlay();

	CurrentPhysicalHP = PhysicalHP;
	CurrentSpiritHP = SpiritHP;

	// Keep placeholder enemies out of the spawn cluster unless already placed far away
	if (HasAuthority() && GetActorLocation().Size2D() < 100.f)
	{
		SetActorLocation(FVector(0.f, 2000.f, 100.f));
	}

	if (EnemyHealthBarClass && HealthBarComponent)
	{
		HealthBarComponent->SetWidgetClass(EnemyHealthBarClass);
	}
	OnHealthUpdated(CurrentPhysicalHP, MaxPhysicalHP);

	if (HasAuthority())
	{
		InitEnemyAbilitySystem();

		if (!BehaviorTree)
		{
			BehaviorTree = LoadObject<UBehaviorTree>(nullptr,
				TEXT("/Game/AI/BT_LinkedSoulsEnemy.BT_LinkedSoulsEnemy"));
		}

		// Fallback if OnPossess ran before BT asset was available
		if (BehaviorTree)
		{
			if (ALinkedSoulsAIController* AIC = Cast<ALinkedSoulsAIController>(GetController()))
			{
				AIC->StartBehaviorTree(BehaviorTree);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("BaseEnemy [%s]: Controller is not LinkedSoulsAIController (%s)"),
					*GetName(), GetController() ? *GetController()->GetClass()->GetName() : TEXT("null"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("BaseEnemy [%s]: BehaviorTree asset missing"), *GetName());
		}
	}
}

void ABaseEnemy::InitEnemyAbilitySystem()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

// -- Damage API --------------------------------------------------------------

void ABaseEnemy::TakePhysicalDamage(float Amount, AActor* DamageInstigator)
{
	if (!HasAuthority())
	{
		return;
	}

	if (EnemyWorld == EEnemyWorld::SpiritOnly)
	{
		UE_LOG(LogTemp, Warning, TEXT("BaseEnemy [%s]: SpiritOnly — Body cannot damage directly"), *GetName());
		return;
	}

	if (EnemyWorld == EEnemyWorld::BothWorlds && CurrentSpiritHP > 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("BaseEnemy [%s]: BothWorlds not weakened — Soul must destroy SpiritHP first"), *GetName());
		return;
	}

	CurrentPhysicalHP = FMath::Clamp(CurrentPhysicalHP - Amount, 0.0f, MaxPhysicalHP);

	OnPhysicalDamage.Broadcast(this, Amount);
	OnHealthUpdated(CurrentPhysicalHP, MaxPhysicalHP);

	if (CurrentPhysicalHP <= 0.0f)
	{
		CheckDeathCondition();
	}
}

void ABaseEnemy::TakeSpiritDamage(float Amount, AActor* DamageInstigator)
{
	if (!HasAuthority())
	{
		return;
	}

	if (EnemyWorld == EEnemyWorld::RealOnly)
	{
		UE_LOG(LogTemp, Warning, TEXT("BaseEnemy [%s]: RealOnly — Soul cannot damage it"), *GetName());
		return;
	}

	CurrentSpiritHP = FMath::Clamp(CurrentSpiritHP - Amount, 0.0f, MaxSpiritHP);

	OnSpiritDamage.Broadcast(this, Amount);

	if (CurrentSpiritHP <= 0.0f)
	{
		OnSpiritDestroyed();
	}
}

void ABaseEnemy::OnSpiritDestroyed()
{
	if (!HasAuthority())
	{
		return;
	}

	if (CurrentState != EEnemyState::Alive)
	{
		return;
	}

	CurrentState = EEnemyState::Weakened;

	OnEnemyWeakened.Broadcast(this);

	UE_LOG(LogTemp, Warning, TEXT("BaseEnemy [%s]: Spirit destroyed — Body can now finish it"), *GetName());
}

void ABaseEnemy::PerformAttack(ALinkedSoulsPlayerCharacter* Target)
{
	if (!HasAuthority() || !AbilitySystemComponent || !Target)
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
	if (!TargetASC)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddInstigator(this, this);

	TSubclassOf<UGameplayEffect> DamageGE = nullptr;
	if (Cast<ASoulCharacter>(Target))
	{
		DamageGE = ULS_GE_CorruptionDamage::StaticClass();
	}
	else if (Cast<ABodyCharacter>(Target))
	{
		DamageGE = ULS_GE_BodyMeleeDamage::StaticClass();
	}

	if (!DamageGE)
	{
		return;
	}

	const FGameplayEffectSpecHandle SpecHandle =
		AbilitySystemComponent->MakeOutgoingSpec(DamageGE, 1.0f, EffectContext);
	if (SpecHandle.Data.IsValid())
	{
		AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
		UE_LOG(LogTemp, Warning, TEXT("BaseEnemy: Attacked [%s]"), *Target->GetName());
	}
}

void ABaseEnemy::OnHealthUpdated(float Current, float Max)
{
	if (!HealthBarComponent)
	{
		return;
	}

	UEnemyHealthBarWidget* Bar = Cast<UEnemyHealthBarWidget>(HealthBarComponent->GetUserWidgetObject());
	if (!Bar)
	{
		// Widget may not be ready yet on first frame — force create
		if (EnemyHealthBarClass)
		{
			HealthBarComponent->SetWidgetClass(EnemyHealthBarClass);
			HealthBarComponent->InitWidget();
			Bar = Cast<UEnemyHealthBarWidget>(HealthBarComponent->GetUserWidgetObject());
		}
	}

	if (Bar)
	{
		Bar->UpdateHealth(Current, Max);
	}
}

// -- Death logic -------------------------------------------------------------

void ABaseEnemy::CheckDeathCondition()
{
	switch (EnemyWorld)
	{
	case EEnemyWorld::RealOnly:
		if (CurrentPhysicalHP <= 0.0f)
		{
			Die();
		}
		break;

	case EEnemyWorld::SpiritOnly:
		if (CurrentSpiritHP <= 0.0f)
		{
			Die();
		}
		break;

	case EEnemyWorld::BothWorlds:
		if (CurrentPhysicalHP <= 0.0f && CurrentSpiritHP <= 0.0f)
		{
			Die();
		}
		break;
	}
}

void ABaseEnemy::Die()
{
	if (!HasAuthority())
	{
		return;
	}

	if (CurrentState == EEnemyState::Dead)
	{
		return;
	}

	CurrentState = EEnemyState::Dead;

	OnEnemyDied.Broadcast();

	if (ALinkedSoulsAIController* AIC = Cast<ALinkedSoulsAIController>(GetController()))
	{
		AIC->StopMovement();
		if (AIC->BehaviorTreeComp)
		{
			AIC->BehaviorTreeComp->StopTree();
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("BaseEnemy [%s]: DIED"), *GetName());

	SetLifeSpan(3.0f);
}

// -- Replication -------------------------------------------------------------

void ABaseEnemy::OnRep_EnemyState(EEnemyState OldState)
{
	UE_LOG(LogTemp, Log, TEXT("BaseEnemy [%s]: state → %s (client)"),
		*GetName(), *UEnum::GetValueAsString(CurrentState));
}

void ABaseEnemy::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABaseEnemy, CurrentPhysicalHP);
	DOREPLIFETIME(ABaseEnemy, CurrentSpiritHP);
	DOREPLIFETIME(ABaseEnemy, CurrentState);
}
