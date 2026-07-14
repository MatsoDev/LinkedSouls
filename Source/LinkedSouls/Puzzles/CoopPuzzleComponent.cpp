// Copyright Epic Games, Inc. All Rights Reserved.


#include "CoopPuzzleComponent.h"
#include "Components/BoxComponent.h"
#include "Player/BodyCharacter.h"
#include "Player/SoulCharacter.h"
#include "Elements/ElementComponent.h"
#include "DualWorldManager.h"
#include "Engine/Engine.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

UCoopPuzzleComponent::UCoopPuzzleComponent()
{
	SetIsReplicated(true);

	PrimaryComponentTick.bCanEverTick = false;

	BodyTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("BodyTrigger"));
	BodyTrigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	BodyTrigger->SetGenerateOverlapEvents(true);
	BodyTrigger->SetBoxExtent(FVector(100.0f, 100.0f, 50.0f));

	SoulTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("SoulTrigger"));
	SoulTrigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	SoulTrigger->SetGenerateOverlapEvents(true);
	SoulTrigger->SetBoxExtent(FVector(100.0f, 100.0f, 50.0f));
}

// -- Lifecycle ---------------------------------------------------------------

void UCoopPuzzleComponent::BeginPlay()
{
	Super::BeginPlay();

	if (BodyTrigger)
	{
		BodyTrigger->OnComponentBeginOverlap.AddDynamic(this, &UCoopPuzzleComponent::OnBodyTriggerEnter);
		BodyTrigger->OnComponentEndOverlap.AddDynamic(this, &UCoopPuzzleComponent::OnBodyTriggerExit);
	}

	if (SoulTrigger)
	{
		SoulTrigger->OnComponentBeginOverlap.AddDynamic(this, &UCoopPuzzleComponent::OnSoulTriggerEnter);
		SoulTrigger->OnComponentEndOverlap.AddDynamic(this, &UCoopPuzzleComponent::OnSoulTriggerExit);
	}
}

// -- Overlap callbacks -------------------------------------------------------

void UCoopPuzzleComponent::OnBodyTriggerEnter(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!Cast<ABodyCharacter>(OtherActor))
	{
		return;
	}

	bBodyReady = true;
	CheckPuzzleCondition();
}

void UCoopPuzzleComponent::OnBodyTriggerExit(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!Cast<ABodyCharacter>(OtherActor))
	{
		return;
	}

	bBodyReady = false;
	CheckPuzzleCondition();
}

void UCoopPuzzleComponent::OnSoulTriggerEnter(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!Cast<ASoulCharacter>(OtherActor))
	{
		return;
	}

	bSoulReady = true;
	CheckPuzzleCondition();
}

void UCoopPuzzleComponent::OnSoulTriggerExit(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!Cast<ASoulCharacter>(OtherActor))
	{
		return;
	}

	bSoulReady = false;
	CheckPuzzleCondition();
}

// -- State logic -------------------------------------------------------------

void UCoopPuzzleComponent::CheckPuzzleCondition()
{
	if (CurrentState == EPuzzleState::Solved)
	{
		return;
	}

	if (!bBodyReady && !bSoulReady)
	{
		return;
	}

	// first activation — start the countdown timer
	if (!GetWorld()->GetTimerManager().IsTimerActive(PuzzleTimeoutTimer))
	{
		GetWorld()->GetTimerManager().SetTimer(PuzzleTimeoutTimer, this,
			&UCoopPuzzleComponent::OnPuzzleTimeout, TimeWindow, false);
	}

	if (bBodyReady && bSoulReady)
	{
		TrySolvePuzzle();
	}
	else if (bBodyReady)
	{
		CurrentState = EPuzzleState::BodyReady;
		OnPuzzleStateChanged.Broadcast(CurrentState);
	}
	else if (bSoulReady)
	{
		CurrentState = EPuzzleState::SoulReady;
		OnPuzzleStateChanged.Broadcast(CurrentState);
	}
}

void UCoopPuzzleComponent::TrySolvePuzzle()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (CurrentState == EPuzzleState::Solved)
	{
		return;
	}

	if (!bRepeatable && CurrentState == EPuzzleState::Solved)
	{
		return;
	}

	// clear timer before state change
	GetWorld()->GetTimerManager().ClearTimer(PuzzleTimeoutTimer);

	CurrentState = EPuzzleState::Solved;

	switch (PuzzleType)
	{
	case EPuzzleType::SyncPressure:
		UE_LOG(LogTemp, Log, TEXT("CoopPuzzle: SyncPressure SOLVED — both on pads"));
		break;
	case EPuzzleType::EchoMirror:
		UE_LOG(LogTemp, Log, TEXT("CoopPuzzle: EchoMirror SOLVED — pattern matched (full impl System 5+)"));
		break;
	case EPuzzleType::WorldBridge:
		UE_LOG(LogTemp, Log, TEXT("CoopPuzzle: WorldBridge SOLVED — platform created (full impl System 5+)"));
		break;
	case EPuzzleType::ElementLink:
	{
		UDualWorldManager* DWM = UDualWorldManager::GetDualWorldManager(this);
		if (DWM)
		{
			ACharacter* BodyChar = DWM->GetBody();
			ACharacter* SoulChar = DWM->GetSoul();
			if (BodyChar && SoulChar)
			{
				UElementComponent* BodyEC = BodyChar->FindComponentByClass<UElementComponent>();
				UElementComponent* SoulEC = SoulChar->FindComponentByClass<UElementComponent>();
				if (BodyEC && SoulEC)
				{
					FName ComboResult = BodyEC->TryCombineWith(SoulEC);
					if (ComboResult != NAME_None)
					{
						UE_LOG(LogTemp, Log, TEXT("CoopPuzzle: ElementLink SOLVED — %s"), *ComboResult.ToString());
					}
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("CoopPuzzle: ElementLink — wrong element combo, resetting"));
						CurrentState = EPuzzleState::Inactive;
						bBodyReady = false;
						bSoulReady = false;
						OnPuzzleReset.Broadcast();
						OnPuzzleStateChanged.Broadcast(CurrentState);
						return;
					}
				}
			}
		}
		break;
	}
	}

	OnPuzzleSolved.Broadcast();
	OnPuzzleStateChanged.Broadcast(CurrentState);
}

void UCoopPuzzleComponent::OnPuzzleTimeout()
{
	bBodyReady = false;
	bSoulReady = false;
	CurrentState = EPuzzleState::Inactive;

	UE_LOG(LogTemp, Warning, TEXT("CoopPuzzle: PUZZLE TIMEOUT — reset"));

	OnPuzzleReset.Broadcast();
	OnPuzzleStateChanged.Broadcast(CurrentState);
}

void UCoopPuzzleComponent::ResetPuzzle()
{
	if (!bRepeatable && IsSolved())
	{
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(PuzzleTimeoutTimer);

	bBodyReady = false;
	bSoulReady = false;
	CurrentState = EPuzzleState::Inactive;

	OnPuzzleReset.Broadcast();
	OnPuzzleStateChanged.Broadcast(CurrentState);
}

// -- Replication -------------------------------------------------------------

void UCoopPuzzleComponent::OnRep_PuzzleState(EPuzzleState OldState)
{
	OnPuzzleStateChanged.Broadcast(CurrentState);
}

void UCoopPuzzleComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCoopPuzzleComponent, CurrentState);
}
