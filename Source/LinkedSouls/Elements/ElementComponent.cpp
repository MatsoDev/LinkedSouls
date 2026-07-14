// Copyright Epic Games, Inc. All Rights Reserved.


#include "ElementComponent.h"
#include "Elements/ElementCombinationTable.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"

UElementComponent::UElementComponent()
{
	SetIsReplicated(true);

	PrimaryComponentTick.bCanEverTick = false;

	// safe default: if AllowedElements is empty we fall back to Fire
	if (AllowedElements.Num() > 0)
	{
		ActiveElement = AllowedElements[0];
	}
	else
	{
		ActiveElement = ELinkedSoulsElement::Fire;
	}
}

// -- Lifecycle ---------------------------------------------------------------

void UElementComponent::BeginPlay()
{
	Super::BeginPlay();

	// ensure ActiveElement is set to the first allowed element after
	// AllowedElements is populated by the owning character's constructor
	if (AllowedElements.Num() > 0 && !AllowedElements.Contains(ActiveElement))
	{
		ActiveElement = AllowedElements[0];
	}
}

// -- API ---------------------------------------------------------------------

bool UElementComponent::SetActiveElement(ELinkedSoulsElement NewElement)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}

	if (!AllowedElements.Contains(NewElement))
	{
		UE_LOG(LogTemp, Warning, TEXT("ElementComponent: %s not in AllowedElements"),
			*UEnum::GetValueAsString(NewElement));
		return false;
	}

	ActiveElement = NewElement;
	OnElementChanged.Broadcast(ActiveElement);
	return true;
}

FName UElementComponent::TryCombineWith(UElementComponent* OtherComponent)
{
	if (!OtherComponent)
	{
		return NAME_None;
	}

	const ELinkedSoulsElement MyElement = GetActiveElement();
	const ELinkedSoulsElement OtherElement = OtherComponent->GetActiveElement();

	float OutMagnitude = 0.0f;
	const FName Result = GetCombinationResult(MyElement, OtherElement, OutMagnitude);

	if (Result != NAME_None)
	{
		UE_LOG(LogTemp, Log, TEXT("ElementCombination: %s + %s = %s (%.1f)"),
			*UEnum::GetValueAsString(MyElement),
			*UEnum::GetValueAsString(OtherElement),
			*Result.ToString(), OutMagnitude);

		OnElementCombined.Broadcast(Result, OutMagnitude);
		return Result;
	}

	UE_LOG(LogTemp, Warning, TEXT("ElementCombination: no combo for %s + %s"),
		*UEnum::GetValueAsString(MyElement),
		*UEnum::GetValueAsString(OtherElement));

	return NAME_None;
}

// -- Replication -------------------------------------------------------------

void UElementComponent::OnRep_ActiveElement(ELinkedSoulsElement OldElement)
{
	OnElementChanged.Broadcast(ActiveElement);
}

void UElementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UElementComponent, ActiveElement);
}
