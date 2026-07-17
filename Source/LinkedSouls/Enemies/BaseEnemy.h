// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "BaseEnemy.generated.h"

class ABodyCharacter;
class ASoulCharacter;
class ALinkedSoulsPlayerCharacter;
class UAbilitySystemComponent;
class ULinkedSoulsAttributeSet;
class UWidgetComponent;
class UUserWidget;
class UBehaviorTree;
class UAnimMontage;
class UNiagaraSystem;
class USoundBase;
class UDamageNumberWidget;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UWidgetComponent* HealthBarComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|UI")
	TSubclassOf<UUserWidget> EnemyHealthBarClass;

	/** Refresh the floating health bar (Physical HP pool). */
	UFUNCTION(BlueprintCallable, Category = "Enemy|UI")
	void OnHealthUpdated(float Current, float Max);

	/** Behavior tree asset assigned in editor (BT_LinkedSoulsEnemy). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	UBehaviorTree* BehaviorTree = nullptr;

	/** AI-driven attack: Corruption on Soul, Health damage on Body. */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Combat")
	void PerformAttack(ALinkedSoulsPlayerCharacter* Target);

	// -- Combat Juice (VFX / SFX / animation feedback) ----------------------

	/** Montage played on the server+clients whenever this enemy takes damage. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Juice")
	UAnimMontage* HitReactMontage;

	/** Niagara system spawned at the enemy's chest on every hit. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Juice")
	UNiagaraSystem* HitVFX;

	/** Niagara system spawned once at the enemy's location on death. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Juice")
	UNiagaraSystem* DeathVFX;

	/** One-shot sound played on every hit. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Juice")
	USoundBase* HitSound;

	/** One-shot sound played on death. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Juice")
	USoundBase* DeathSound;

	/** World-space widget class for floating damage numbers. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Juice")
	TSubclassOf<UDamageNumberWidget> DamageNumberWidgetClass;

	/**
	 *  Drives all combat-juice feedback (montage, VFX, sound, damage number)
	 *  for a single hit. Called on the server from OnHealthUpdated whenever
	 *  Health decreases, then multicasts the cosmetic playback to clients.
	 *  @param DamageAmount   Magnitude of the hit (positive number).
	 *  @param bIsCorruption  True for purple corruption hits, false for red damage.
	 */
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayHitReact(float DamageAmount, bool bIsCorruption);

	/** Local helper that plays the hit-react montage + VFX + sound. */
	void PlayHitReact(float DamageAmount, bool bIsCorruption);

	/** Local helper that spawns the death VFX + sound (called before destroy). */
	void PlayDeathFeedback();

protected:

	virtual void BeginPlay() override;

	void InitEnemyAbilitySystem();

	/** Checks whether the enemy should die based on its world type. */
	virtual void CheckDeathCondition();

	/** Kills the enemy. */
	virtual void Die();

	/** Spawns death VFX/sound and destroys the actor after DeathDelaySeconds. */
	void DestroyEnemy();

	/** Timer handle backing the delayed destroy in DestroyEnemy(). */
	FTimerHandle DeathTimerHandle;

	/** Seconds between death feedback and actor destroy. */
	static constexpr float DeathDelaySeconds = 2.0f;

	/** Called when SpiritHP reaches zero. */
	void OnSpiritDestroyed();

	/**
	 *  Last Health value reported by OnHealthUpdated.
	 *  Used to detect decreases and fire combat-juice feedback exactly once
	 *  per hit (rather than once per replicated correction).
	 */
	float LastReportedHealth = -1.0f;

	// -- Replication --------------------------------------------------------

	UFUNCTION()
	void OnRep_EnemyState(EEnemyState OldState);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
