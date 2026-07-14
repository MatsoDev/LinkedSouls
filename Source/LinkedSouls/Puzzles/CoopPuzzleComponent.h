// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/BoxComponent.h"
#include "CoopPuzzleComponent.generated.h"

class ABodyCharacter;
class ASoulCharacter;

/** Current resolution state of the puzzle. */
UENUM(BlueprintType)
enum class EPuzzleState : uint8
{
	Inactive,
	BodyReady,
	SoulReady,
	Solved
};

/** Which co-op mechanic this puzzle uses. */
UENUM(BlueprintType)
enum class EPuzzleType : uint8
{
	/** Both players stand on pressure pads simultaneously. */
	SyncPressure,
	/** Soul copies Body's movement pattern. */
	EchoMirror,
	/** Soul creates a platform Body can cross. */
	WorldBridge,
	/** Body + Soul combine elements to unlock. */
	ElementLink
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPuzzleSolved);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPuzzleReset);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPuzzleStateChanged, EPuzzleState, NewState);

/**
 *  Generic co-op puzzle component.
 *
 *  Attach to any Actor that acts as a puzzle trigger. Two BoxComponent volumes
 *  detect Body and Soul overlap; when both players are present within the
 *  TimeWindow the puzzle resolves. Different EPuzzleType values log distinct
 *  solve messages; full behaviour per type is stubbed for System 5+ / 6+.
 *
 *  State replicates so the server is authoritative and clients see the correct
 *  state for HUD / VFX.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class LINKEDSOULS_API UCoopPuzzleComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UCoopPuzzleComponent();

	// -- Config --------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle")
	EPuzzleType PuzzleType = EPuzzleType::SyncPressure;

	/** Seconds within which both players must activate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle", meta = (ClampMin = "0.5", ClampMax = "60.0", Units = "s"))
	float TimeWindow = 5.0f;

	/** If true the puzzle can be solved again after reset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle")
	bool bRepeatable = false;

	// -- Trigger volumes ----------------------------------------------------

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Triggers")
	UBoxComponent* BodyTrigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Triggers")
	UBoxComponent* SoulTrigger;

	// -- Delegates ----------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "Puzzle")
	FOnPuzzleSolved OnPuzzleSolved;

	UPROPERTY(BlueprintAssignable, Category = "Puzzle")
	FOnPuzzleReset OnPuzzleReset;

	UPROPERTY(BlueprintAssignable, Category = "Puzzle")
	FOnPuzzleStateChanged OnPuzzleStateChanged;

	// -- Queries ------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "Puzzle")
	EPuzzleState GetPuzzleState() const { return CurrentState; }

	UFUNCTION(BlueprintPure, Category = "Puzzle")
	bool IsSolved() const { return CurrentState == EPuzzleState::Solved; }

	/** Manual reset callable from Blueprints or other systems. */
	UFUNCTION(BlueprintCallable, Category = "Puzzle")
	void ResetPuzzle();

protected:

	virtual void BeginPlay() override;

	// -- Overlap callbacks --------------------------------------------------

	UFUNCTION()
	void OnBodyTriggerEnter(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnBodyTriggerExit(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION()
	void OnSoulTriggerEnter(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnSoulTriggerExit(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// -- State logic --------------------------------------------------------

	/** Called whenever bBodyReady or bBodyReady changes. Starts the timer on first activation. */
	void CheckPuzzleCondition();

	/** Attempts to solve the puzzle (guards: already solved / non-repeatable). */
	void TrySolvePuzzle();

	/** Timeout expired without both players — reset. */
	UFUNCTION()
	void OnPuzzleTimeout();

	// -- Replication --------------------------------------------------------

	UFUNCTION()
	void OnRep_PuzzleState(EPuzzleState OldState);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:

	UPROPERTY(ReplicatedUsing = OnRep_PuzzleState)
	EPuzzleState CurrentState = EPuzzleState::Inactive;

	FTimerHandle PuzzleTimeoutTimer;

	bool bBodyReady = false;
	bool bSoulReady = false;
};
