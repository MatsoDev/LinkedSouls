// Copyright Epic Games, Inc. All Rights Reserved.


#include "Enemies/BaseEnemy.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/SkeletalMesh.h"
#include "Components/SkeletalMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"

ABaseEnemy::ABaseEnemy()
{
	bReplicates = true;

	PrimaryActorTick.bCanEverTick = false;

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
}

// -- Lifecycle ---------------------------------------------------------------

void ABaseEnemy::BeginPlay()
{
	Super::BeginPlay();

	CurrentPhysicalHP = PhysicalHP;
	CurrentSpiritHP = SpiritHP;
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
