#include "AI/BTTask_PatrolMove.h"
#include "AI/LinkedSoulsBlackboardKeys.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "BrainComponent.h"

UBTTask_PatrolMove::UBTTask_PatrolMove()
{
	NodeName = TEXT("Patrol Move");
	bNotifyTick = false;
}

EBTNodeResult::Type UBTTask_PatrolMove::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!AIC || !BB || !AIC->GetPawn())
	{
		return EBTNodeResult::Failed;
	}

	const FVector Home = BB->GetValueAsVector(FLinkedSoulsBlackboardKeys::HomeLocation);
	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(AIC->GetWorld());
	if (!NavSys)
	{
		return EBTNodeResult::Failed;
	}

	FNavLocation PatrolPoint;
	const FVector Origin = Home.IsNearlyZero() ? AIC->GetPawn()->GetActorLocation() : Home;
	if (!NavSys->GetRandomReachablePointInRadius(Origin, PatrolRadius, PatrolPoint))
	{
		return EBTNodeResult::Failed;
	}

	FAIMoveRequest MoveReq(PatrolPoint.Location);
	MoveReq.SetAcceptanceRadius(AcceptanceRadius);
	MoveReq.SetUsePathfinding(true);
	MoveReq.SetAllowPartialPath(true);
	MoveReq.SetProjectGoalLocation(true);
	MoveReq.SetReachTestIncludesAgentRadius(true);

	FPathFollowingRequestResult MoveResult = AIC->MoveTo(MoveReq);
	if (MoveResult.Code == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		return EBTNodeResult::Succeeded;
	}
	if (MoveResult.Code != EPathFollowingRequestResult::RequestSuccessful)
	{
		// Fallback direct move if nav query fails
		MoveReq.SetUsePathfinding(false);
		MoveResult = AIC->MoveTo(MoveReq);
		if (MoveResult.Code != EPathFollowingRequestResult::RequestSuccessful
			&& MoveResult.Code != EPathFollowingRequestResult::AlreadyAtGoal)
		{
			return EBTNodeResult::Failed;
		}
		if (MoveResult.Code == EPathFollowingRequestResult::AlreadyAtGoal)
		{
			return EBTNodeResult::Succeeded;
		}
	}

	WaitForMessage(OwnerComp, UBrainComponent::AIMessage_MoveFinished, MoveResult.MoveId);
	return EBTNodeResult::InProgress;
}

EBTNodeResult::Type UBTTask_PatrolMove::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AAIController* AIC = OwnerComp.GetAIOwner())
	{
		AIC->StopMovement();
	}
	return EBTNodeResult::Aborted;
}
