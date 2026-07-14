#include "Input/LinkedSoulsInputConfig.h"

const UInputAction* ULinkedSoulsInputConfig::FindInputActionForTag(
	const FGameplayTag& Tag,
	bool bLogNotFound) const
{
	for (const FLinkedSoulsInputAction& Action : NativeInputActions)
	{
		if (Action.InputAction && Action.InputTag == Tag)
		{
			return Action.InputAction;
		}
	}

	if (bLogNotFound)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("InputConfig: no action found for tag [%s]"),
			*Tag.ToString());
	}

	return nullptr;
}
