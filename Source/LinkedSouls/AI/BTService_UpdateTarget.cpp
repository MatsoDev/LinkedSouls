#include "AI/BTService_UpdateTarget.h"
#include "AI/LinkedSoulsBlackboardKeys.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Actor.h"

UBTService_UpdateTarget::UBTService_UpdateTarget()
{
	NodeName = TEXT("Update Target");
	Interval = 0.5f;
	RandomDeviation = 0.1f;
}

void UBTService_UpdateTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB)
	{
		return;
	}

	const bool bCanSee = BB->GetValueAsBool(FLinkedSoulsBlackboardKeys::CanSeeTarget);
	AActor* Target = Cast<AActor>(BB->GetValueAsObject(FLinkedSoulsBlackboardKeys::TargetActor));

	if (bCanSee && IsValid(Target))
	{
		BB->SetValueAsFloat(FLinkedSoulsBlackboardKeys::TimeSinceLostSight, 0.f);
		BB->SetValueAsVector(FLinkedSoulsBlackboardKeys::LastSeenLocation, Target->GetActorLocation());
		return;
	}

	// Lost sight — accumulate time, then clear target
	if (IsValid(Target))
	{
		float Elapsed = BB->GetValueAsFloat(FLinkedSoulsBlackboardKeys::TimeSinceLostSight);
		Elapsed += Interval;
		BB->SetValueAsFloat(FLinkedSoulsBlackboardKeys::TimeSinceLostSight, Elapsed);

		if (Elapsed >= LoseTargetDelay)
		{
			BB->SetValueAsObject(FLinkedSoulsBlackboardKeys::TargetActor, nullptr);
			BB->SetValueAsBool(FLinkedSoulsBlackboardKeys::CanSeeTarget, false);
			BB->SetValueAsFloat(FLinkedSoulsBlackboardKeys::TimeSinceLostSight, 0.f);
			UE_LOG(LogTemp, Warning, TEXT("LinkedSouls AI: Target cleared after %.1fs lost sight"), LoseTargetDelay);
		}
	}
}
