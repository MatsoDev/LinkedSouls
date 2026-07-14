// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Enemies/BaseEnemy.h"
#include "TreeHeartBoss.generated.h"

class ABodyCharacter;
class ASoulCharacter;

/** Boss fight phases. */
UENUM(BlueprintType)
enum class EBossPhase : uint8
{
	/** Soul destroys spirit roots while Body fends off attacks. */
	Phase1,
	/** Both players attack the heart simultaneously. */
	Phase2
};

/**
 *  TreeHeartBoss — System 7 boss encounter.
 *
 *  Phase 1: Soul must destroy SpiritRootsTotal spirit roots while the
 *  SpiritBarrier makes the boss immune to physical damage. Body survives
 *  RootSlam / SoulCorrupt attacks.
 *
 *  Phase 2: SpiritBarrier is broken. Both players attack until PhysicalHP
 *  and SpiritHP are depleted.
 */
UCLASS()
class LINKEDSOULS_API ATreeHeartBoss : public ABaseEnemy
{
	GENERATED_BODY()

public:

	ATreeHeartBoss();

	// -- Boss state ---------------------------------------------------------

	UPROPERTY(ReplicatedUsing = OnRep_BossPhase)
	EBossPhase CurrentPhase = EBossPhase::Phase1;

	UPROPERTY(Replicated)
	bool bSpiritBarrierActive = true;

	/** Total number of spirit roots the Soul must destroy in Phase 1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss", meta = (ClampMin = "1"))
	int32 SpiritRootsTotal = 4;

	UPROPERTY(Replicated)
	int32 SpiritRootsDestroyed = 0;

	// -- Abilities ----------------------------------------------------------

	/** Re-activates the Spirit Barrier (blocks all physical damage). */
	UFUNCTION(BlueprintCallable, Category = "Boss")
	void ActivateSpiritBarrier();

	/** Breaks the Spirit Barrier, allowing Body to damage the heart. */
	UFUNCTION(BlueprintCallable, Category = "Boss")
	void DeactivateSpiritBarrier();

	/** Slams the ground — deals 25 physical damage to Body. */
	UFUNCTION(BlueprintCallable, Category = "Boss")
	void RootSlam(ABodyCharacter* Target);

	/** Corrupts the Soul — deals 15 corruption damage. */
	UFUNCTION(BlueprintCallable, Category = "Boss")
	void SoulCorrupt(ASoulCharacter* Target);

	/** Called when a spirit root is destroyed. Increments progress. */
	UFUNCTION(BlueprintCallable, Category = "Boss")
	void OnSpiritRootDestroyed();

protected:

	virtual void BeginPlay() override;

	// -- Overrides ----------------------------------------------------------

	/** Blocks physical damage while SpiritBarrier is active. */
	virtual void TakePhysicalDamage(float Amount, AActor* DamageInstigator) override;

	/** Phase 1: immune. Phase 2: both HP bars must reach 0. */
	virtual void CheckDeathCondition() override;

	/** Boss-specific death: Super + victory log. */
	virtual void Die() override;

	// -- Internal -----------------------------------------------------------

	void CheckPhaseTransition();

	// -- Replication --------------------------------------------------------

	UFUNCTION()
	void OnRep_BossPhase();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
