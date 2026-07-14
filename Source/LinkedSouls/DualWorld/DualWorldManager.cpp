// Copyright Epic Games, Inc. All Rights Reserved.


#include "DualWorldManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

UDualWorldManager::UDualWorldManager()
{
	// the Body world is the default focus when a level starts
	FocusWorld = EDualWorld::RealWorld;
}

UDualWorldManager* UDualWorldManager::GetDualWorldManager(const UObject* WorldContextObject)
{
	// resolve the world from the supplied context
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		return World->GetSubsystem<UDualWorldManager>();
	}

	return nullptr;
}

void UDualWorldManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// start each level focused on the Body (Real World)
	FocusWorld = EDualWorld::RealWorld;
}

void UDualWorldManager::Deinitialize()
{
	// drop every reference so nothing keeps actors alive past shutdown
	BodyCharacter = nullptr;
	SoulCharacter = nullptr;
	TrackedActors.Reset();

	Super::Deinitialize();
}

bool UDualWorldManager::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	// only run inside real game / PIE worlds, not in editor preview worlds
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

// -- Player registration ----------------------------------------------------

void UDualWorldManager::RegisterBody(ACharacter* InBody)
{
	BodyCharacter = InBody;
}

void UDualWorldManager::RegisterSoul(ACharacter* InSoul)
{
	SoulCharacter = InSoul;
}

// -- Actor world presence ---------------------------------------------------

void UDualWorldManager::RegisterActor(AActor* Actor, EWorldPresence Presence)
{
	// reject null actors
	if (!Actor)
	{
		return;
	}

	// drop any stale entries before mutating the list
	CleanTrackedActors();

	// update the presence if the actor is already tracked
	for (FActorWorldEntry& Entry : TrackedActors)
	{
		if (Entry.Actor.Get() == Actor)
		{
			Entry.Presence = Presence;
			ApplyVisibilityFor(Actor);
			return;
		}
	}

	// brand new entry
	TrackedActors.Add(FActorWorldEntry{Actor, Presence});
	ApplyVisibilityFor(Actor);
}

void UDualWorldManager::UnregisterActor(AActor* Actor)
{
	if (!Actor)
	{
		return;
	}

	// remove every entry pointing at this actor (handles stale duplicates too)
	TrackedActors.RemoveAll([Actor](const FActorWorldEntry& Entry)
	{
		return Entry.Actor.Get() == Actor;
	});
}

void UDualWorldManager::SetActorPresence(AActor* Actor, EWorldPresence NewPresence)
{
	if (!Actor)
	{
		return;
	}

	// flip the presence of the tracked entry, then refresh that actor's visibility
	for (FActorWorldEntry& Entry : TrackedActors)
	{
		if (Entry.Actor.Get() == Actor)
		{
			Entry.Presence = NewPresence;
			ApplyVisibilityFor(Actor);
			return;
		}
	}
}

EWorldPresence UDualWorldManager::GetActorPresence(const AActor* Actor) const
{
	if (!Actor)
	{
		return EWorldPresence::BothWorlds;
	}

	// look up the stored presence
	for (const FActorWorldEntry& Entry : TrackedActors)
	{
		if (Entry.Actor.Get() == Actor)
		{
			return Entry.Presence;
		}
	}

	// unknown actors are treated as existing in both worlds
	return EWorldPresence::BothWorlds;
}

// -- Focus world ------------------------------------------------------------

void UDualWorldManager::SetFocusWorld(EDualWorld NewFocusWorld)
{
	// nothing to do if the focus is already set
	if (FocusWorld == NewFocusWorld)
	{
		return;
	}

	FocusWorld = NewFocusWorld;

	// recompute every actor's visibility for the new focus
	RefreshVisibility();

	// let listeners (HUD, portals) react to the switch
	OnFocusWorldChanged.Broadcast(FocusWorld);
}

// -- Visibility queries -----------------------------------------------------

bool UDualWorldManager::IsPresenceVisibleInWorld(EWorldPresence Presence, EDualWorld World)
{
	// BothWorlds actors are always visible regardless of the world
	if (Presence == EWorldPresence::BothWorlds)
	{
		return true;
	}

	// RealOnly <-> RealWorld, SpiritOnly <-> SpiritWorld
	if (Presence == EWorldPresence::RealOnly)
	{
		return World == EDualWorld::RealWorld;
	}

	return World == EDualWorld::SpiritWorld;
}

bool UDualWorldManager::IsActorVisible(const AActor* Actor) const
{
	if (!Actor)
	{
		return false;
	}

	// resolve the actor's presence against the current focus world
	return IsPresenceVisibleInWorld(GetActorPresence(Actor), FocusWorld);
}

void UDualWorldManager::RefreshVisibility()
{
	// drop dead references first so we don't touch destroyed actors
	CleanTrackedActors();

	for (const FActorWorldEntry& Entry : TrackedActors)
	{
		if (AActor* Actor = Entry.Actor.Get())
		{
			ApplyVisibilityFor(Actor);
		}
	}
}

// -- Internal helpers -------------------------------------------------------

void UDualWorldManager::ApplyVisibilityFor(AActor* Actor) const
{
	if (!Actor)
	{
		return;
	}

	// hide the actor when its presence does not match the current focus world
	const bool bShouldBeVisible = IsPresenceVisibleInWorld(GetActorPresence(Actor), FocusWorld);
	Actor->SetActorHiddenInGame(!bShouldBeVisible);
}

void UDualWorldManager::CleanTrackedActors()
{
	// remove entries whose actor has been destroyed or garbage collected
	TrackedActors.RemoveAll([](const FActorWorldEntry& Entry)
	{
		return !Entry.Actor.IsValid();
	});
}
