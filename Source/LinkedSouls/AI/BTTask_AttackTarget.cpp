#include "AI/BTTask_AttackTarget.h"
#include "AI/LinkedSoulsBlackboardKeys.h"
#include "Enemies/BaseEnemy.h"
#include "Player/LinkedSoulsPlayerCharacter.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_AttackTarget::UBTTask_AttackTarget()
{
	NodeName = TEXT("Attack Target");
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_AttackTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FBTAttackTargetMemory* Memory = reinterpret_cast<FBTAttackTargetMemory*>(NodeMemory);
	Memory->RemainingCooldown = 0.f;

	AAIController* AIC = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!AIC || !BB || !AIC->GetPawn())
	{
		return EBTNodeResult::Failed;
	}

	AActor* TargetActor = Cast<AActor>(BB->GetValueAsObject(FLinkedSoulsBlackboardKeys::TargetActor));
	ALinkedSoulsPlayerCharacter* Target = Cast<ALinkedSoulsPlayerCharacter>(TargetActor);
	ABaseEnemy* Enemy = Cast<ABaseEnemy>(AIC->GetPawn());
	if (!Target || !Enemy)
	{
		return EBTNodeResult::Failed;
	}

	const float Dist = FVector::Dist(Enemy->GetActorLocation(), Target->GetActorLocation());
	if (Dist > AttackRange)
	{
		return EBTNodeResult::Failed;
	}

	Enemy->PerformAttack(Target);
	BB->SetValueAsBool(FLinkedSoulsBlackboardKeys::IsAttacking, true);
	Memory->RemainingCooldown = AttackCooldown;
	return EBTNodeResult::InProgress;
}

void UBTTask_AttackTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FBTAttackTargetMemory* Memory = reinterpret_cast<FBTAttackTargetMemory*>(NodeMemory);
	Memory->RemainingCooldown -= DeltaSeconds;
	if (Memory->RemainingCooldown <= 0.f)
	{
		if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
		{
			BB->SetValueAsBool(FLinkedSoulsBlackboardKeys::IsAttacking, false);
		}
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}
