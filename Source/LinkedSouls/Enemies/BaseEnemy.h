// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "BaseEnemy.generated.h"

class ABodyCharacter;
class ASoulCharacter;
class UAbilitySystemComponent;
class ULinkedSoulsAttributeSet;

/** Which world(s) this enemy exists in. */
UENUM(BlueprintType)
enum class EEnemyWorld : uint8
{
	/** Body can only damage this enemy. */
	RealOnly,
	/** Soul weakens the spirit shell, then Body finishes it. */
	SpiritOnly,
	/** Requires both players — spirit must be destroyed first. */
	BothWorlds
};

/** Current life-state of the enemy. */
UENUM(BlueprintType)
enum class EEnemyState : uint8
{
	Alive,
	/** SpiritHP is zero but PhysicalHP remains. */
	Weakened,
	Dead
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBaseEnemyDied);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyWeakened, ABaseEnemy*, Enemy);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPhysicalDamage, ABaseEnemy*, Enemy, float, Amount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSpiritDamage, ABaseEnemy*, Enemy, float, Amount);

/**
 *  Base class for all LinkedSouls enemies.
 *
 *  Enemies live in one or both worlds and have two health bars: PhysicalHP
 *  (damaged by Body) and SpiritHP (damaged by Soul). Behaviour per
 *  EEnemyWorld is enforced in the damage API — the wrong player type is
 *  rejected with a log message.
 */
UCLASS()
class LINKEDSOULS_API ABaseEnemy : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:

	ABaseEnemy();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystemComponent; }

	// -- Config --------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	EEnemyWorld EnemyWorld = EEnemyWorld::RealOnly;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health", meta = (ClampMin = "1.0"))
	float PhysicalHP = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health", meta = (ClampMin = "1.0"))
	float MaxPhysicalHP = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health", meta = (ClampMin = "1.0"))
	float SpiritHP = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health", meta = (ClampMin = "1.0"))
	float MaxSpiritHP = 100.0f;

	// -- Runtime state (replicated) -----------------------------------------

	UPROPERTY(Replicated)
	float CurrentPhysicalHP;

	UPROPERTY(Replicated)
	float CurrentSpiritHP;

	UPROPERTY(ReplicatedUsing = OnRep_EnemyState)
	EEnemyState CurrentState = EEnemyState::Alive;

	// -- Delegates ----------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "Enemy")
	FOnBaseEnemyDied OnEnemyDied;

	UPROPERTY(BlueprintAssignable, Category = "Enemy")
	FOnEnemyWeakened OnEnemyWeakened;

	UPROPERTY(BlueprintAssignable, Category = "Enemy")
	FOnPhysicalDamage OnPhysicalDamage;

	UPROPERTY(BlueprintAssignable, Category = "Enemy")
	FOnSpiritDamage OnSpiritDamage;

	// -- Damage API ---------------------------------------------------------

	/** Applies physical damage. Body's attack method. */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Combat")
	virtual void TakePhysicalDamage(float Amount, AActor* DamageInstigator);

	/** Applies spirit damage. Soul's attack method. */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Combat")
	virtual void TakeSpiritDamage(float Amount, AActor* DamageInstigator);

	// -- GAS Components ------------------------------------------------------

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	ULinkedSoulsAttributeSet* AttributeSet;

protected:

	virtual void BeginPlay() override;

	void InitEnemyAbilitySystem();

	/** Checks whether the enemy should die based on its world type. */
	virtual void CheckDeathCondition();

	/** Kills the enemy. */
	virtual void Die();

	/** Called when SpiritHP reaches zero. */
	void OnSpiritDestroyed();

	/** Placeholder attack: applies GE_CorruptionDamage to nearby Soul. */
	void Server_EnemyAttackSoul();

	/** Timer handle for the placeholder attack. */
	FTimerHandle AttackSoulTimerHandle;

	// -- Replication --------------------------------------------------------

	UFUNCTION()
	void OnRep_EnemyState(EEnemyState OldState);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
