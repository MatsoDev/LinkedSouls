// Copyright Epic Games, Inc. All Rights Reserved.

#include "LinkedSoulsGameMode.h"
#include "LinkedSoulsGameState.h"
#include "SoulEnergy/SoulEnergyComponent.h"
#include "HUD/LinkedSoulsHUD.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"

ALinkedSoulsGameMode::ALinkedSoulsGameMode()
{
	// use the custom game state that owns the SoulEnergyComponent
	GameStateClass = ALinkedSoulsGameState::StaticClass();

	// register the HUD class
	HUDClass = ALinkedSoulsHUD::StaticClass();

	DefaultPawnClass = nullptr;
	// PostLogin handles spawning Body/Soul explicitly
	// Setting nullptr prevents GameMode from auto-spawning
	// BP_LinkedSoulsCharacter before PostLogin runs
}

void ALinkedSoulsGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (!GetWorld()) return;

	// In listen server, the host player's controller is created during
	// world init and may not trigger PostLogin. Catch any that were missed.
	TArray<APlayerController*> Uninitialized;
	for (FConstPlayerControllerIterator It =
		GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (IsValid(PC) && PC->GetPawn() == nullptr)
		{
			Uninitialized.Add(PC);
		}
	}

	for (APlayerController* PC : Uninitialized)
	{
		PostLogin(PC);
	}
}

void ALinkedSoulsGameMode::PostLogin(
	APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (!NewPlayer || !GetWorld()) return;

	if (APawn* ExistingPawn = NewPlayer->GetPawn())
	{
		ExistingPawn->Destroy();
		NewPlayer->UnPossess();
	}

	SpawnPlayerPawn(NewPlayer, NumInitializedPlayers == 0);

	if (NumInitializedPlayers >= 2)
	{
		LinkPartners();
	}
}

void ALinkedSoulsGameMode::LinkPartners()
{
	ALinkedSoulsPlayerCharacter* Body = nullptr;
	ALinkedSoulsPlayerCharacter* Soul = nullptr;

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC) continue;

		ALinkedSoulsPlayerCharacter* Char = Cast<ALinkedSoulsPlayerCharacter>(PC->GetPawn());
		if (!Char) continue;

		if (Char->GetPlayerWorld() == EDualWorld::RealWorld)
		{
			Body = Char;
		}
		else
		{
			Soul = Char;
		}
	}

	if (Body && Soul)
	{
		Body->SetLinkedPartner(Soul);
		Soul->SetLinkedPartner(Body);

		ABodyCharacter* BodyChar = Cast<ABodyCharacter>(Body);
		ASoulCharacter* SoulChar = Cast<ASoulCharacter>(Soul);
		if (BodyChar && SoulChar)
		{
			BodyChar->SetLinkedSoul(SoulChar);
			SoulChar->SetLinkedBody(BodyChar);
		}

		UE_LOG(LogTemp, Warning, TEXT("LinkedSouls: Partners linked — Body <-> Soul"));
	}
}

void ALinkedSoulsGameMode::SpawnPlayerPawn(
	APlayerController* NewPlayer, bool bIsBody)
{
	FVector SpawnLocation = bIsBody
		? FVector(-300.f, 0.f, 200.f)
		: FVector(300.f, 0.f, 200.f);

	TSubclassOf<ACharacter> CharClass = bIsBody
		? ABodyCharacter::StaticClass()
		: ASoulCharacter::StaticClass();

	UE_LOG(LogTemp, Warning,
		TEXT("SpawnPlayerPawn: %s -> %s"),
		bIsBody ? TEXT("Body") : TEXT("Soul"),
		*CharClass->GetName())

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::
		AdjustIfPossibleButAlwaysSpawn;
	SpawnParams.Owner = NewPlayer;

	ACharacter* NewChar = GetWorld()->SpawnActor<ACharacter>(
		CharClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);

	if (NewChar)
	{
		NewPlayer->Possess(NewChar);
		NumInitializedPlayers++;
		UE_LOG(LogTemp, Warning,
			TEXT("SpawnPlayerPawn: Possessed %s"),
			*NewChar->GetName())
	}
	else
	{
		UE_LOG(LogTemp, Error,
			TEXT("SpawnPlayerPawn: FAILED for %s"),
			bIsBody ? TEXT("Body") : TEXT("Soul"))
	}
}

// -- Soul Energy ----------------------------------------------------------------

USoulEnergyComponent* ALinkedSoulsGameMode::GetSoulEnergyComponent() const
{
	ALinkedSoulsGameState* GS = GetGameState<ALinkedSoulsGameState>();
	return GS ? GS->SoulEnergyComponent : nullptr;
}

bool ALinkedSoulsGameMode::ConsumeSoulEnergy(float Amount)
{
	USoulEnergyComponent* SEC = GetSoulEnergyComponent();
	return SEC ? SEC->ConsumeSoulEnergy(Amount) : false;
}

void ALinkedSoulsGameMode::RestoreSoulEnergy(float Amount)
{
	USoulEnergyComponent* SEC = GetSoulEnergyComponent();
	if (SEC) SEC->RestoreSoulEnergy(Amount);
}

float ALinkedSoulsGameMode::GetSoulEnergy() const
{
	USoulEnergyComponent* SEC = GetSoulEnergyComponent();
	return SEC ? SEC->GetSoulEnergy() : 0.0f;
}
