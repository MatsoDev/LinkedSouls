// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "WorldPortal.generated.h"

class ABodyCharacter;
class ASoulCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPortalActivated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPortalDeactivated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerEnteredPortalRange, bool, bIsBody);

/**
 *  World Portal — visual + gameplay anchor where both players briefly
 *  see each other across worlds.
 *
 *  When Body and Soul are both within detection range the portal fully
 *  activates: cross-world ghost visibility is toggled on via the
 *  DualWorldManager and a light pulse fires. Deactivation restores
 *  normal world presence.
 */
UCLASS()
class LINKEDSOULS_API AWorldPortal : public AActor
{
	GENERATED_BODY()

public:

	AWorldPortal();

	// -- Components ---------------------------------------------------------

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	UStaticMeshComponent* PortalMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	USphereComponent* BodyDetectionVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	USphereComponent* SoulDetectionVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	UPointLightComponent* PortalLight;

	// -- Config --------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal", meta = (ClampMin = "100.0", ClampMax = "2000.0", Units = "cm"))
	float DetectionRadius = 400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GhostOpacity = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal")
	bool bRequiresBothForActivation = true;

	// -- State (replicated) -------------------------------------------------

	UPROPERTY(ReplicatedUsing = OnRep_PortalState)
	bool bBodyNearPortal = false;

	UPROPERTY(ReplicatedUsing = OnRep_PortalState)
	bool bSoulNearPortal = false;

	UPROPERTY(ReplicatedUsing = OnRep_PortalState)
	bool bFullyActivated = false;

	// -- Delegates ----------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "Portal")
	FOnPortalActivated OnPortalActivated;

	UPROPERTY(BlueprintAssignable, Category = "Portal")
	FOnPortalDeactivated OnPortalDeactivated;

	UPROPERTY(BlueprintAssignable, Category = "Portal")
	FOnPlayerEnteredPortalRange OnPlayerEnteredPortalRange;

protected:

	virtual void BeginPlay() override;

	// -- Overlap callbacks --------------------------------------------------

	UFUNCTION()
	void OnBodyVolumeEnter(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnBodyVolumeExit(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION()
	void OnSoulVolumeEnter(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnSoulVolumeExit(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// -- Activation logic ---------------------------------------------------

	void CheckActivationState();
	void ApplyGhostEffects();
	void RemoveGhostEffects();
	void PulseLightOnActivation();

	UFUNCTION()
	void OnLightPulseComplete();

	// -- Replication --------------------------------------------------------

	UFUNCTION()
	void OnRep_PortalState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:

	FTimerHandle PulseLightTimer;

	static constexpr float LightPulseIntensity = 5000.0f;
	static constexpr float LightNormalIntensity = 2000.0f;
	static constexpr float LightPulseDuration = 0.5f;
};
