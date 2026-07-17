// Copyright Epic Games, Inc. All Rights Reserved.


#include "WorldPortal/WorldPortal.h"
#include "Player/BodyCharacter.h"
#include "Player/SoulCharacter.h"
#include "DualWorld/DualWorldManager.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"
#include "TimerManager.h"

AWorldPortal::AWorldPortal()
{
	bReplicates = true;

	PrimaryActorTick.bCanEverTick = false;

	// -- Portal mesh --------------------------------------------------------

	PortalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalMesh"));
	RootComponent = PortalMesh;

	/*
	 * TEAM ASSET HANDOFF
	 * System: World Portal
	 * Replace with: portal arch/gate mesh
	 * Delivered by:
	 * Format: .uasset (StaticMesh)
	 * Priority: Medium
	 */
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
		TEXT("/Engine/BasicShapes/Sphere"));
	if (SphereMesh.Succeeded())
	{
		PortalMesh->SetStaticMesh(SphereMesh.Object);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("WorldPortal: Failed to load placeholder sphere mesh"));
	}

	PortalMesh->SetRelativeScale3D(FVector(0.5f, 0.5f, 1.5f));

	// -- Detection volumes --------------------------------------------------

	BodyDetectionVolume = CreateDefaultSubobject<USphereComponent>(TEXT("BodyDetectionVolume"));
	BodyDetectionVolume->SetupAttachment(RootComponent);
	BodyDetectionVolume->SetSphereRadius(DetectionRadius);
	BodyDetectionVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	BodyDetectionVolume->SetGenerateOverlapEvents(true);

	SoulDetectionVolume = CreateDefaultSubobject<USphereComponent>(TEXT("SoulDetectionVolume"));
	SoulDetectionVolume->SetupAttachment(RootComponent);
	SoulDetectionVolume->SetSphereRadius(DetectionRadius);
	SoulDetectionVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	SoulDetectionVolume->SetGenerateOverlapEvents(true);

	// -- Portal light -------------------------------------------------------

	PortalLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PortalLight"));
	PortalLight->SetupAttachment(RootComponent);
	PortalLight->SetLightColor(FLinearColor(0.4f, 0.6f, 1.0f));
	PortalLight->SetIntensity(2000.0f);
	PortalLight->SetAttenuationRadius(800.0f);

	// -- Bind overlaps ------------------------------------------------------

	BodyDetectionVolume->OnComponentBeginOverlap.AddDynamic(this, &AWorldPortal::OnBodyVolumeEnter);
	BodyDetectionVolume->OnComponentEndOverlap.AddDynamic(this, &AWorldPortal::OnBodyVolumeExit);

	SoulDetectionVolume->OnComponentBeginOverlap.AddDynamic(this, &AWorldPortal::OnSoulVolumeEnter);
	SoulDetectionVolume->OnComponentEndOverlap.AddDynamic(this, &AWorldPortal::OnSoulVolumeExit);
}

// -- Lifecycle ---------------------------------------------------------------

void AWorldPortal::BeginPlay()
{
	Super::BeginPlay();
}

// -- Overlap callbacks -------------------------------------------------------

void AWorldPortal::OnBodyVolumeEnter(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!Cast<ABodyCharacter>(OtherActor))
	{
		return;
	}

	// Guard against duplicate overlap events — BeginOverlap can fire multiple
	// times per entry (multi-component actors, sweep quirks). If Body is
	// already considered near, this is a duplicate; skip it.
	if (bBodyNearPortal)
	{
		return;
	}

	bBodyNearPortal = true;

	OnPlayerEnteredPortalRange.Broadcast(true);

	UE_LOG(LogTemp, Log, TEXT("WorldPortal [%s]: Body entered portal range"), *GetName());

	CheckActivationState();
}

void AWorldPortal::OnBodyVolumeExit(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!Cast<ABodyCharacter>(OtherActor))
	{
		return;
	}

	// Guard against duplicate end-overlap events — only act if we actually
	// considered Body near the portal.
	if (!bBodyNearPortal)
	{
		return;
	}

	bBodyNearPortal = false;

	CheckActivationState();
}

void AWorldPortal::OnSoulVolumeEnter(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!Cast<ASoulCharacter>(OtherActor))
	{
		return;
	}

	// Guard against duplicate overlap events — see OnBodyVolumeEnter.
	if (bSoulNearPortal)
	{
		return;
	}

	bSoulNearPortal = true;

	OnPlayerEnteredPortalRange.Broadcast(false);

	UE_LOG(LogTemp, Log, TEXT("WorldPortal [%s]: Soul entered portal range"), *GetName());

	CheckActivationState();
}

void AWorldPortal::OnSoulVolumeExit(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!Cast<ASoulCharacter>(OtherActor))
	{
		return;
	}

	// Guard against duplicate end-overlap events — see OnBodyVolumeExit.
	if (!bSoulNearPortal)
	{
		return;
	}

	bSoulNearPortal = false;

	CheckActivationState();
}

// -- Activation logic --------------------------------------------------------

void AWorldPortal::CheckActivationState()
{
	if (!HasAuthority())
	{
		return;
	}

	const bool bShouldActivate = bBodyNearPortal && bSoulNearPortal;

	if (bShouldActivate && !bFullyActivated)
	{
		bFullyActivated = true;
		OnPortalActivated.Broadcast();
		ApplyGhostEffects();
		PulseLightOnActivation();
		UE_LOG(LogTemp, Log, TEXT("WorldPortal [%s]: FULLY ACTIVATED — both players in range"), *GetName());
	}
	else if (!bShouldActivate && bFullyActivated)
	{
		bFullyActivated = false;
		OnPortalDeactivated.Broadcast();
		RemoveGhostEffects();
		UE_LOG(LogTemp, Log, TEXT("WorldPortal [%s]: deactivated"), *GetName());
	}
}

// -- Ghost effects -----------------------------------------------------------

void AWorldPortal::ApplyGhostEffects()
{
	UDualWorldManager* DWM = UDualWorldManager::GetDualWorldManager(this);
	if (!DWM)
	{
		UE_LOG(LogTemp, Warning, TEXT("WorldPortal [%s]: DualWorldManager not found — ghost effects skipped"), *GetName());
		return;
	}

	if (bBodyNearPortal)
	{
		if (ASoulCharacter* Soul = Cast<ASoulCharacter>(DWM->GetSoul()))
		{
			DWM->SetActorPresence(Soul, EWorldPresence::BothWorlds);
		}
	}

	if (bSoulNearPortal)
	{
		if (ABodyCharacter* Body = Cast<ABodyCharacter>(DWM->GetBody()))
		{
			DWM->SetActorPresence(Body, EWorldPresence::BothWorlds);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("WorldPortal [%s]: Ghost effects applied — cross-world visibility ON"), *GetName());
}

void AWorldPortal::RemoveGhostEffects()
{
	UDualWorldManager* DWM = UDualWorldManager::GetDualWorldManager(this);
	if (!DWM)
	{
		return;
	}

	// restore Soul to SpiritOnly if not manifested
	if (AActor* SoulActor = DWM->GetSoul())
	{
		if (ASoulCharacter* SC = Cast<ASoulCharacter>(SoulActor))
		{
			if (!SC->IsManifested())
			{
				DWM->SetActorPresence(SoulActor, EWorldPresence::SpiritOnly);
			}
		}
	}

	// restore Body to RealOnly if not shifted
	if (AActor* BodyActor = DWM->GetBody())
	{
		if (ABodyCharacter* BC = Cast<ABodyCharacter>(BodyActor))
		{
			if (!BC->IsInSpiritWorld())
			{
				DWM->SetActorPresence(BodyActor, EWorldPresence::RealOnly);
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("WorldPortal [%s]: Ghost effects removed — cross-world visibility OFF"), *GetName());
}

// -- Light pulse -------------------------------------------------------------

void AWorldPortal::PulseLightOnActivation()
{
	if (!PortalLight)
	{
		return;
	}

	PortalLight->SetIntensity(LightPulseIntensity);

	GetWorldTimerManager().SetTimer(PulseLightTimer, this,
		&AWorldPortal::OnLightPulseComplete, LightPulseDuration, false);
}

void AWorldPortal::OnLightPulseComplete()
{
	if (PortalLight)
	{
		PortalLight->SetIntensity(LightNormalIntensity);
	}
}

// -- Replication -------------------------------------------------------------

void AWorldPortal::OnRep_PortalState()
{
	if (!PortalLight)
	{
		return;
	}

	if (bFullyActivated)
	{
		PortalLight->SetLightColor(FLinearColor(1.0f, 1.0f, 1.0f));
		PortalLight->SetIntensity(4000.0f);
		UE_LOG(LogTemp, Log, TEXT("WorldPortal [%s]: Portal fully active (client)"), *GetName());
	}
	else if (bBodyNearPortal || bSoulNearPortal)
	{
		PortalLight->SetLightColor(FLinearColor(0.6f, 0.8f, 1.0f));
		PortalLight->SetIntensity(3000.0f);
		UE_LOG(LogTemp, Log, TEXT("WorldPortal [%s]: Portal partial (client)"), *GetName());
	}
	else
	{
		PortalLight->SetLightColor(FLinearColor(0.4f, 0.6f, 1.0f));
		PortalLight->SetIntensity(LightNormalIntensity);
	}
}

void AWorldPortal::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWorldPortal, bBodyNearPortal);
	DOREPLIFETIME(AWorldPortal, bSoulNearPortal);
	DOREPLIFETIME(AWorldPortal, bFullyActivated);
}
