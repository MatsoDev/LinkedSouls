#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "InputAction.h"
#include "LinkedSoulsInputConfig.generated.h"

USTRUCT(BlueprintType)
struct FLinkedSoulsInputAction
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	const UInputAction* InputAction = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag InputTag;
};

UCLASS(BlueprintType, Const)
class LINKEDSOULS_API ULinkedSoulsInputConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TArray<FLinkedSoulsInputAction> NativeInputActions;

	const UInputAction* FindInputActionForTag(
		const FGameplayTag& Tag,
		bool bLogNotFound = true) const;
};
