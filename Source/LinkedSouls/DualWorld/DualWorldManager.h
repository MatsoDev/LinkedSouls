// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "DualWorldManager.generated.h"

class ACharacter;
class AActor;

/** The two parallel worlds that coexist in LinkedSouls. */
UENUM(BlueprintType)
enum class EDualWorld : uint8
{
	/** The physical world. The Body player lives here. */
	RealWorld UMETA(DisplayName = "Real World"),

	/** The spiritual world. The Soul player lives here. */
	SpiritWorld UMETA(DisplayName = "Spirit World")
};

/** Which world(s) an actor exists and is rendered in. */
UENUM(BlueprintType)
enum class EWorldPresence : uint8
{
	/** Present only in the Real World (e.g. Body-side enemies / geometry). */
	RealOnly UMETA(DisplayName = "Real Only"),

	/** Present only in the Spirit World (e.g. Soul-side enemies / roots). */
	SpiritOnly UMETA(DisplayName = "Spirit Only"),

	/** Present and visible in both worlds simultaneously. */
	BothWorlds UMETA(DisplayName = "Both Worlds")
};

/** Broadcast whenever the focused (primary rendered) world changes. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDualWorldFocusChanged, EDualWorld, NewFocusWorld);

/**
 * Core manager for the dual-world mechanic of LinkedSouls.
 *
 * The game runs two parallel worlds at the same time:
 *   - Real World    -> inhabited by the Body player.
 *   - Spirit World  -> inhabited by the Soul player.
 *
 * Actors register their world presence; the manager resolves whether they
 * should be visible/active given the current focus world and applies the
 * actor hidden state accordingly. It also keeps a handle to each player
 * character so other systems (HUD, enemies, portals) can resolve the
 * Body/Soul pair quickly.
 */
UCLASS()
class LINKEDSOULS_API UDualWorldManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	/** Default constructor. */
	UDualWorldManager();

	/** Static accessor for the manager that owns the given world. */
	UFUNCTION(BlueprintPure, Category = "LinkedSouls|Dual World", meta = (WorldContext = "WorldContextObject"))
	static UDualWorldManager* GetDualWorldManager(const UObject* WorldContextObject);

	// -- Player registration -------------------------------------------------

	/** Registers the Body player character (lives in the Real World). */
	UFUNCTION(BlueprintCallable, Category = "LinkedSouls|Dual World")
	void RegisterBody(ACharacter* InBody);

	/** Registers the Soul player character (lives in the Spirit World). */
	UFUNCTION(BlueprintCallable, Category = "LinkedSouls|Dual World")
	void RegisterSoul(ACharacter* InSoul);

	/** @returns the Body player character, or nullptr if not registered. */
	UFUNCTION(BlueprintPure, Category = "LinkedSouls|Dual World")
	ACharacter* GetBody() const { return BodyCharacter.Get(); }

	/** @returns the Soul player character, or nullptr if not registered. */
	UFUNCTION(BlueprintPure, Category = "LinkedSouls|Dual World")
	ACharacter* GetSoul() const { return SoulCharacter.Get(); }

	// -- Actor world presence -----------------------------------------------

	/** Registers an actor with a specific world presence and refreshes visibility. */
	UFUNCTION(BlueprintCallable, Category = "LinkedSouls|Dual World")
	void RegisterActor(AActor* Actor, EWorldPresence Presence);

	/** Removes an actor from tracking (e.g. when it is destroyed). */
	UFUNCTION(BlueprintCallable, Category = "LinkedSouls|Dual World")
	void UnregisterActor(AActor* Actor);

	/** Changes the presence of an already registered actor and refreshes visibility. */
	UFUNCTION(BlueprintCallable, Category = "LinkedSouls|Dual World")
	void SetActorPresence(AActor* Actor, EWorldPresence NewPresence);

	/** @returns the registered presence for an actor, or BothWorlds if unknown. */
	UFUNCTION(BlueprintPure, Category = "LinkedSouls|Dual World")
	EWorldPresence GetActorPresence(const AActor* Actor) const;

	// -- Focus world ---------------------------------------------------------

	/** @returns the world that is currently the primary rendered focus. */
	UFUNCTION(BlueprintPure, Category = "LinkedSouls|Dual World")
	EDualWorld GetFocusWorld() const { return FocusWorld; }

	/** Switches the primary focus world and refreshes actor visibility. */
	UFUNCTION(BlueprintCallable, Category = "LinkedSouls|Dual World")
	void SetFocusWorld(EDualWorld NewFocusWorld);

	// -- Visibility queries --------------------------------------------------

	/** True if an actor with the given presence is visible from the supplied world. */
	UFUNCTION(BlueprintPure, Category = "LinkedSouls|Dual World")
	static bool IsPresenceVisibleInWorld(EWorldPresence Presence, EDualWorld World);

	/** True if the supplied actor is currently meant to be visible. */
	UFUNCTION(BlueprintPure, Category = "LinkedSouls|Dual World")
	bool IsActorVisible(const AActor* Actor) const;

	/** Recomputes and applies visibility for every tracked actor. */
	UFUNCTION(BlueprintCallable, Category = "LinkedSouls|Dual World")
	void RefreshVisibility();

public:

	/** Raised when the focus world changes. */
	UPROPERTY(BlueprintAssignable, Category = "LinkedSouls|Dual World")
	FOnDualWorldFocusChanged OnFocusWorldChanged;

protected:

	// -- UWorldSubsystem interface ------------------------------------------

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

	/** Applies a single actor's hidden state based on the current focus world. */
	void ApplyVisibilityFor(AActor* Actor) const;

	/** Removes any destroyed / null tracked actors. */
	void CleanTrackedActors();

private:

	/** One tracked actor plus its declared world presence. */
	struct FActorWorldEntry
	{
		/** Tracked actor (weak so destruction clears it automatically). */
		TWeakObjectPtr<AActor> Actor;

		/** Which world(s) the actor belongs to. */
		EWorldPresence Presence;
	};

	/** The Body player character (Real World). */
	TWeakObjectPtr<ACharacter> BodyCharacter;

	/** The Soul player character (Spirit World). */
	TWeakObjectPtr<ACharacter> SoulCharacter;

	/** All actors registered with the dual-world system. */
	TArray<FActorWorldEntry> TrackedActors;

	/** The world that is currently the primary rendered focus. */
	UPROPERTY(VisibleAnywhere, Category = "Dual World")
	EDualWorld FocusWorld;
};
