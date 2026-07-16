#include "Animation/LinkedSoulsAnimInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void ULinkedSoulsAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
}

void ULinkedSoulsAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	ACharacter* Owner = Cast<ACharacter>(TryGetPawnOwner());
	if (!Owner)
	{
		Speed = 0.0f;
		bIsInAir = false;
		return;
	}

	UCharacterMovementComponent* Movement = Owner->GetCharacterMovement();
	if (!Movement)
	{
		return;
	}

	Speed = Owner->GetVelocity().Size2D();
	bIsInAir = Movement->IsFalling();
}
