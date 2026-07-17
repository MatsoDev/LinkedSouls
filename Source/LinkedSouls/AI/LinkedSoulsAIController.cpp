#include "AI/LinkedSoulsAIController.h"
#include "AI/LinkedSoulsBlackboardKeys.h"
#include "AI/BTTask_PatrolMove.h"
#include "AI/BTTask_ChaseTarget.h"
#include "AI/BTTask_AttackTarget.h"
#include "AI/BTService_UpdateTarget.h"
#include "Enemies/BaseEnemy.h"
#include "Player/LinkedSoulsPlayerCharacter.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Composites/BTComposite_Selector.h"
#include "BehaviorTree/Composites/BTComposite_Sequence.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Sight.h"
#include "UObject/ConstructorHelpers.h"

ALinkedSoulsAIController::ALinkedSoulsAIController()
{
	BehaviorTreeComp = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorTreeComp"));
	BlackboardComp = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComp"));
	BrainComponent = BehaviorTreeComp;

	PerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComp"));
	SetPerceptionComponent(*PerceptionComp);

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 800.f;
	SightConfig->LoseSightRadius = 1000.f;
	SightConfig->PeripheralVisionAngleDegrees = 90.f;
	SightConfig->SetMaxAge(3.f);
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

	PerceptionComp->ConfigureSense(*SightConfig);
	PerceptionComp->SetDominantSense(SightConfig->GetSenseImplementation());
}

void ALinkedSoulsAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(
		this, &ALinkedSoulsAIController::OnTargetPerceptionUpdated);

	if (ABaseEnemy* Enemy = Cast<ABaseEnemy>(InPawn))
	{
		// Placed instances often serialize null BehaviorTree — always resolve asset path
		if (!Enemy->BehaviorTree)
		{
			Enemy->BehaviorTree = LoadObject<UBehaviorTree>(nullptr,
				TEXT("/Game/AI/BT_LinkedSoulsEnemy.BT_LinkedSoulsEnemy"));
		}

		if (Enemy->BehaviorTree)
		{
			StartBehaviorTree(Enemy->BehaviorTree);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("LinkedSoulsAI: No BehaviorTree on %s and asset load failed"),
				*Enemy->GetName());
		}
	}
}

void ALinkedSoulsAIController::OnUnPossess()
{
	if (PerceptionComp)
	{
		PerceptionComp->OnTargetPerceptionUpdated.RemoveDynamic(
			this, &ALinkedSoulsAIController::OnTargetPerceptionUpdated);
	}

	if (BehaviorTreeComp)
	{
		BehaviorTreeComp->StopTree();
	}

	Super::OnUnPossess();
}

void ALinkedSoulsAIController::EnsureTreeGraph(UBehaviorTree* Tree)
{
	if (!Tree || Tree->RootNode)
	{
		return;
	}

	// Root Selector
	//   Service: UpdateTarget (interval set in UBTService_UpdateTarget ctor)
	//   Child0 Sequence: Chase → Attack  (fails if no TargetActor → falls through)
	//   Child1 Patrol
	UBTComposite_Selector* Root = NewObject<UBTComposite_Selector>(Tree, TEXT("RootSelector"));
	Tree->RootNode = Root;

	UBTService_UpdateTarget* UpdateService = NewObject<UBTService_UpdateTarget>(Tree, TEXT("UpdateTargetService"));
	UpdateService->LoseTargetDelay = 3.f;
	Root->Services.Add(UpdateService);

	UBTComposite_Sequence* CombatSeq = NewObject<UBTComposite_Sequence>(Tree, TEXT("CombatSequence"));

	UBTTask_ChaseTarget* ChaseTask = NewObject<UBTTask_ChaseTarget>(Tree, TEXT("ChaseTask"));
	ChaseTask->AcceptanceRadius = 150.f;

	UBTTask_AttackTarget* AttackTask = NewObject<UBTTask_AttackTarget>(Tree, TEXT("AttackTask"));
	AttackTask->AttackRange = 200.f;
	AttackTask->AttackCooldown = 1.5f;

	{
		FBTCompositeChild ChaseChild;
		ChaseChild.ChildTask = ChaseTask;
		CombatSeq->Children.Add(ChaseChild);

		FBTCompositeChild AttackChild;
		AttackChild.ChildTask = AttackTask;
		CombatSeq->Children.Add(AttackChild);
	}

	UBTTask_PatrolMove* PatrolTask = NewObject<UBTTask_PatrolMove>(Tree, TEXT("PatrolTask"));
	PatrolTask->PatrolRadius = 500.f;
	PatrolTask->AcceptanceRadius = 50.f;

	{
		FBTCompositeChild CombatChild;
		CombatChild.ChildComposite = CombatSeq;
		Root->Children.Add(CombatChild);

		FBTCompositeChild PatrolChild;
		PatrolChild.ChildTask = PatrolTask;
		Root->Children.Add(PatrolChild);
	}

	UE_LOG(LogTemp, Warning, TEXT("LinkedSoulsAI: Built runtime BT graph on %s"), *Tree->GetName());
}

bool ALinkedSoulsAIController::StartBehaviorTree(UBehaviorTree* Tree)
{
	if (!Tree)
	{
		UE_LOG(LogTemp, Error, TEXT("LinkedSoulsAI: BehaviorTree is null"));
		return false;
	}

	// Wire empty asset graphs so PIE works without hand-editing the BT editor
	EnsureTreeGraph(Tree);

	if (!Tree->RootNode)
	{
		UE_LOG(LogTemp, Error, TEXT("LinkedSoulsAI: Tree has no RootNode after EnsureTreeGraph"));
		return false;
	}

	if (!Tree->BlackboardAsset)
	{
		// Fallback: load BB asset by path
		UBlackboardData* BB = LoadObject<UBlackboardData>(nullptr,
			TEXT("/Game/AI/BB_LinkedSoulsEnemy.BB_LinkedSoulsEnemy"));
		if (BB)
		{
			Tree->BlackboardAsset = BB;
		}
	}

	if (!Tree->BlackboardAsset)
	{
		UE_LOG(LogTemp, Error, TEXT("LinkedSoulsAI: BlackboardAsset missing on tree"));
		return false;
	}

	if (!UseBlackboard(Tree->BlackboardAsset, BlackboardComp))
	{
		UE_LOG(LogTemp, Error, TEXT("LinkedSoulsAI: UseBlackboard failed"));
		return false;
	}

	InitBlackboardKeys(GetPawn());

	const bool bStarted = RunBehaviorTree(Tree);
	UE_LOG(LogTemp, Warning, TEXT("LinkedSoulsAI: RunBehaviorTree %s"), bStarted ? TEXT("OK") : TEXT("FAILED"));
	return bStarted;
}

void ALinkedSoulsAIController::InitBlackboardKeys(APawn* InPawn)
{
	if (!BlackboardComp || !InPawn)
	{
		return;
	}

	BlackboardComp->SetValueAsObject(FLinkedSoulsBlackboardKeys::TargetActor, nullptr);
	BlackboardComp->SetValueAsVector(FLinkedSoulsBlackboardKeys::HomeLocation, InPawn->GetActorLocation());
	BlackboardComp->SetValueAsBool(FLinkedSoulsBlackboardKeys::CanSeeTarget, false);
	BlackboardComp->SetValueAsBool(FLinkedSoulsBlackboardKeys::IsAttacking, false);
	BlackboardComp->SetValueAsVector(FLinkedSoulsBlackboardKeys::LastSeenLocation, InPawn->GetActorLocation());
	BlackboardComp->SetValueAsFloat(FLinkedSoulsBlackboardKeys::TimeSinceLostSight, 0.f);
}

void ALinkedSoulsAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!BlackboardComp || !Actor)
	{
		return;
	}

	if (!Cast<ALinkedSoulsPlayerCharacter>(Actor))
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		BlackboardComp->SetValueAsObject(FLinkedSoulsBlackboardKeys::TargetActor, Actor);
		BlackboardComp->SetValueAsBool(FLinkedSoulsBlackboardKeys::CanSeeTarget, true);
		BlackboardComp->SetValueAsVector(FLinkedSoulsBlackboardKeys::LastSeenLocation, Actor->GetActorLocation());
		BlackboardComp->SetValueAsFloat(FLinkedSoulsBlackboardKeys::TimeSinceLostSight, 0.f);
		UE_LOG(LogTemp, Warning, TEXT("LinkedSouls AI: Target acquired [%s]"), *Actor->GetName());
	}
	else
	{
		BlackboardComp->SetValueAsBool(FLinkedSoulsBlackboardKeys::CanSeeTarget, false);
		BlackboardComp->SetValueAsVector(FLinkedSoulsBlackboardKeys::LastSeenLocation, Stimulus.StimulusLocation);
		UE_LOG(LogTemp, Warning, TEXT("LinkedSouls AI: Target lost [%s]"), *Actor->GetName());
	}
}
