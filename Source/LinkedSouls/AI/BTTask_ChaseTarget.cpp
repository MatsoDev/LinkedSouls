#include "AI/BTTask_ChaseTarget.h"
#include "AI/LinkedSoulsBlackboardKeys.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "BrainComponent.h"
#include "GameFramework/Actor.h"

UBTTask_ChaseTarget::UBTTask_ChaseTarget()
{
	NodeName = TEXT("Chase Target");
}

EBTNodeResult::Type UBTTask_ChaseTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!AIC || !BB)
	{
		return EBTNodeResult::Failed;
	}

	// Prefer active sight; still chase last known TargetActor if set
	AActor* Target = Cast<AActor>(BB->GetValueAsObject(FLinkedSoulsBlackboardKeys::TargetActor));
	if (!IsValid(Target))
	{
		return EBTNodeResult::Failed;
	}

	APawn* Pawn = AIC->GetPawn();
	if (!Pawn)
	{
		return EBTNodeResult::Failed;
	}

	const float Dist = FVector::Dist(Pawn->GetActorLocation(), Target->GetActorLocation());
	if (Dist <= AcceptanceRadius)
	{
		return EBTNodeResult::Succeeded;
	}

	// Try pathfinding first, then direct move (works even if NavMesh is incomplete)
	FAIMoveRequest MoveReq(Target);
	MoveReq.SetAcceptanceRadius(AcceptanceRadius);
	MoveReq.SetUsePathfinding(true);
	MoveReq.SetAllowPartialPath(true);
	MoveReq.SetProjectGoalLocation(true);
	MoveReq.SetReachTestIncludesAgentRadius(true);
	MoveReq.SetCanStrafe(true);

	FPathFollowingRequestResult MoveResult = AIC->MoveTo(MoveReq);
	if (MoveResult.Code == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		return EBTNodeResult::Succeeded;
	}
	if (MoveResult.Code != EPathFollowingRequestResult::RequestSuccessful)
	{
		// Fallback: direct move without pathfinding
		MoveReq.SetUsePathfinding(false);
		MoveResult = AIC->MoveTo(MoveReq);
		if (MoveResult.Code == EPathFollowingRequestResult::AlreadyAtGoal)
		{
			return EBTNodeResult::Succeeded;
		}
		if (MoveResult.Code != EPathFollowingRequestResult::RequestSuccessful)
		{
			UE_LOG(LogTemp, Warning, TEXT("BTTask_ChaseTarget: MoveTo failed (dist=%.0f)"), Dist);
			return EBTNodeResult::Failed;
		}
	}

	WaitForMessage(OwnerComp, UBrainComponent::AIMessage_MoveFinished, MoveResult.MoveId);
	return EBTNodeResult::InProgress;
}

EBTNodeResult::Type UBTTask_ChaseTarget::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AAIController* AIC = OwnerComp.GetAIOwner())
	{
		AIC->StopMovement();
	}
	return EBTNodeResult::Aborted;
}
