// Copyright Epic Games, Inc. All Rights Reserved.


#include "SoulEnergyComponent.h"

// -- Engine includes (kept out of the header) -------------------------------
#include "DualWorldManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

USoulEnergyComponent::USoulEnergyComponent()
{
	// shared pool component replicates to all clients
	SetIsReplicated(true);

	// tick is required for per-frame regen / drain resolution
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	// start at the designer-set starting value
	SoulEnergy = StartingEnergy;
}

// -- Static access -----------------------------------------------------------

USoulEnergyComponent* USoulEnergyComponent::GetSoulEnergyComponent(const UObject* WorldContextObject)
{
	// TODO: Requires ALinkedSoulsGameState that creates this component - add GameState in follow-up pass.
	// resolve the world from the supplied context
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		if (AGameStateBase* GameState = World->GetGameState())
		{
			return GameState->FindComponentByClass<USoulEnergyComponent>();
		}
	}

	return nullptr;
}

// -- Public API --------------------------------------------------------------

void USoulEnergyComponent::DrainEnergy(float Amount)
{
	// server-authoritative only
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	// warn on insufficient energy but still drain to zero - do not block
	if (SoulEnergy < Amount)
	{
		UE_LOG(LogTemp, Warning, TEXT("SoulEnergyComponent: DrainEnergy %.1f rejected - insufficient energy (%.1f)"), Amount, SoulEnergy);
	}

	SetEnergyInternal(SoulEnergy - Amount);
}

void USoulEnergyComponent::SetContinuousDrain(bool bEnabled)
{
	// server-authoritative only
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	// reference-count so multiple callers can request drain independently
	if (bEnabled)
	{
		++ActiveDrainCount;
	}
	else
	{
		ActiveDrainCount = FMath::Max(0, ActiveDrainCount - 1);
	}
}

// -- Replication -------------------------------------------------------------

void USoulEnergyComponent::OnRep_SoulEnergy(float OldValue)
{
	// re-broadcast the same delegates on clients so HUD / listeners stay in sync
	BroadcastEnergyDelegates(OldValue, SoulEnergy);
}

void USoulEnergyComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// replicate the shared pool to every client
	DOREPLIFETIME(USoulEnergyComponent, SoulEnergy);
}

// -- ActorComponent overrides ------------------------------------------------

void USoulEnergyComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// regen / drain is server-authoritative
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	// silently skip tick if DWM not ready yet (early BeginPlay frames)
	UDualWorldManager* DWM = UDualWorldManager::GetDualWorldManager(this);
	if (!DWM)
	{
		return;
	}

	// drain and regen are mutually exclusive per tick
	if (ActiveDrainCount > 0)
	{
		// at least one ability is active - drain, suppress regen
		SetEnergyInternal(SoulEnergy - (ContinuousDrainRate * DeltaTime));
	}
	else if (SoulEnergy < MaxEnergy)
	{
		// nobody is draining and the pool is not full - regenerate
		ACharacter* Body = DWM->GetBody();
		ACharacter* Soul = DWM->GetSoul();

		float RegenRate = BaseRegenRate;
		if (Body && Soul)
		{
			// TODO: optimize to timer-based check (not every tick)
			float Dist = FVector::Dist(Body->GetActorLocation(), Soul->GetActorLocation());
			RegenRate = (Dist <= ProximityDistance) ? ProximityRegenRate : BaseRegenRate;
		}

		SetEnergyInternal(SoulEnergy + (RegenRate * DeltaTime));
	}
}

// -- Internal helpers --------------------------------------------------------

void USoulEnergyComponent::SetEnergyInternal(float NewValue)
{
	const float OldValue = SoulEnergy;

	// clamp to the valid range
	const float ClampedValue = FMath::Clamp(NewValue, 0.0f, MaxEnergy);

	// nothing to do if the clamped value matches the current value
	if (FMath::IsNearlyEqual(ClampedValue, OldValue))
	{
		return;
	}

	SoulEnergy = ClampedValue;

	// fire change + threshold delegates
	BroadcastEnergyDelegates(OldValue, SoulEnergy);
}

void USoulEnergyComponent::BroadcastEnergyDelegates(float OldValue, float NewValue)
{
	// always broadcast the change
	OnEnergyChanged.Broadcast(NewValue);

	// crossed down to zero -> depleted
	if (OldValue > 0.0f && NewValue <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("SoulEnergyComponent: ENERGY DEPLETED - both players weakened (GAS debuff wired in System 4)"));
		OnEnergyDepleted.Broadcast();
	}

	// crossed up to maximum -> full
	if (OldValue < MaxEnergy && NewValue >= MaxEnergy)
	{
		UE_LOG(LogTemp, Log, TEXT("SoulEnergyComponent: ENERGY FULL - both players buffed (GAS buff wired in System 4)"));
		OnEnergyFull.Broadcast();
	}
}
