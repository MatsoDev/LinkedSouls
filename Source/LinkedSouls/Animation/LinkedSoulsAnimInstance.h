#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "LinkedSoulsAnimInstance.generated.h"

UCLASS()
class LINKEDSOULS_API ULinkedSoulsAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	/** Movement speed (2D horizontal). */
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float Speed = 0.0f;

	/** True while the character is falling. */
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsInAir = false;

	/** True while performing an attack (set externally). */
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsAttacking = false;
};
