// Copyright Epic Games, Inc. All Rights Reserved.


#include "Enemies/TreeHeartBoss.h"
#include "Player/BodyCharacter.h"
#include "Player/SoulCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/SkeletalMesh.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"

ATreeHeartBoss::ATreeHeartBoss()
{
	PhysicalHP = MaxPhysicalHP = 1000.0f;
	SpiritHP = MaxSpiritHP = 800.0f;
	EnemyWorld = EEnemyWorld::BothWorlds;

	CurrentPhase = EBossPhase::Phase1;
	bSpiritBarrierActive = true;
	SpiritRootsDestroyed = 0;

	/*
	 * TEAM ASSET HANDOFF
	 * System: Boss Enemy
	 * Replace with: TreeHeart boss mesh (large)
	 * Delivered by:
	 * Format: .uasset (SkeletalMesh)
	 * Priority: High
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
			UE_LOG(LogTemp, Error, TEXT("TreeHeartBoss: Failed to load placeholder mesh - check Manny path"));
		}
	}
}

// -- Lifecycle ---------------------------------------------------------------

void ATreeHeartBoss::BeginPlay()
{
	Super::BeginPlay();
}

// -- Damage override ---------------------------------------------------------

void ATreeHeartBoss::TakePhysicalDamage(float Amount, AActor* DamageInstigator)
{
	if (!HasAuthority())
	{
		return;
	}

	if (bSpiritBarrierActive)
	{
		UE_LOG(LogTemp, Warning, TEXT("TreeHeartBoss: SpiritBarrier active — Soul must destroy %d/%d roots first"),
			SpiritRootsDestroyed, SpiritRootsTotal);
		return;
	}

	Super::TakePhysicalDamage(Amount, DamageInstigator);
}

// -- Abilities ---------------------------------------------------------------

void ATreeHeartBoss::ActivateSpiritBarrier()
{
	if (!HasAuthority())
	{
		return;
	}

	bSpiritBarrierActive = true;

	UE_LOG(LogTemp, Warning, TEXT("TreeHeartBoss: SpiritBarrier ACTIVATED"));
}

void ATreeHeartBoss::DeactivateSpiritBarrier()
{
	if (!HasAuthority())
	{
		return;
	}

	bSpiritBarrierActive = false;

	UE_LOG(LogTemp, Warning, TEXT("TreeHeartBoss: SpiritBarrier BROKEN — Body can now attack the heart"));
}

void ATreeHeartBoss::RootSlam(ABodyCharacter* Target)
{
	if (!Target)
	{
		return;
	}

	if (!HasAuthority())
	{
		return;
	}

	UGameplayStatics::ApplyDamage(Target, 25.0f, nullptr, this, nullptr);

	UE_LOG(LogTemp, Warning, TEXT("TreeHeartBoss: RootSlam → Body takes 25 damage"));
	// TODO: trigger animation notify in later pass
}

void ATreeHeartBoss::SoulCorrupt(ASoulCharacter* Target)
{
	if (!Target)
	{
		return;
	}

	if (!HasAuthority())
	{
		return;
	}

	Target->OnCorruptionDamage(15.0f);

	UE_LOG(LogTemp, Warning, TEXT("TreeHeartBoss: SoulCorrupt → Soul takes 15 corruption"));
	// TODO: trigger VFX in later pass
}

void ATreeHeartBoss::OnSpiritRootDestroyed()
{
	if (!HasAuthority())
	{
		return;
	}

	if (CurrentPhase != EBossPhase::Phase1)
	{
		return;
	}

	++SpiritRootsDestroyed;

	UE_LOG(LogTemp, Log, TEXT("TreeHeartBoss: Spirit root destroyed %d/%d"),
		SpiritRootsDestroyed, SpiritRootsTotal);

	if (SpiritRootsDestroyed >= SpiritRootsTotal)
	{
		DeactivateSpiritBarrier();
		CheckPhaseTransition();
	}
}

// -- Phase logic -------------------------------------------------------------

void ATreeHeartBoss::CheckPhaseTransition()
{
	if (!HasAuthority())
	{
		return;
	}

	if (CurrentPhase == EBossPhase::Phase1 && !bSpiritBarrierActive)
	{
		CurrentPhase = EBossPhase::Phase2;

		OnRep_BossPhase();

		UE_LOG(LogTemp, Warning, TEXT("TreeHeartBoss: PHASE 2 — Both players attack the heart simultaneously!"));
	}
}

// -- Death logic (overrides) -------------------------------------------------

void ATreeHeartBoss::CheckDeathCondition()
{
	if (CurrentPhase == EBossPhase::Phase1)
	{
		UE_LOG(LogTemp, Log, TEXT("TreeHeartBoss: Phase1 — cannot die yet"));
		return;
	}

	// Phase 2: both HP bars must reach 0
	if (CurrentPhysicalHP <= 0.0f && CurrentSpiritHP <= 0.0f)
	{
		Die();
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("TreeHeartBoss: Phase2 — Physical: %.1f / Spirit: %.1f remaining"),
			CurrentPhysicalHP, CurrentSpiritHP);
	}
}

void ATreeHeartBoss::Die()
{
	Super::Die();

	UE_LOG(LogTemp, Warning, TEXT("TreeHeartBoss: DEFEATED — The dark corruption is vanquished. Balance restored."));
	// TODO: trigger victory sequence in later pass
}

// -- Replication -------------------------------------------------------------

void ATreeHeartBoss::OnRep_BossPhase()
{
	UE_LOG(LogTemp, Log, TEXT("TreeHeartBoss: Phase → %s (client)"),
		*UEnum::GetValueAsString(CurrentPhase));
	// TODO: trigger phase VFX + music swap in later pass
}

void ATreeHeartBoss::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATreeHeartBoss, CurrentPhase);
	DOREPLIFETIME(ATreeHeartBoss, bSpiritBarrierActive);
	DOREPLIFETIME(ATreeHeartBoss, SpiritRootsDestroyed);
}
