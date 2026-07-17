#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "LinkedSoulsAIController.generated.h"

class UBehaviorTreeComponent;
class UBlackboardComponent;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UBehaviorTree;

UCLASS()
class LINKEDSOULS_API ALinkedSoulsAIController : public AAIController
{
	GENERATED_BODY()

public:
	ALinkedSoulsAIController();

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	UBehaviorTreeComponent* BehaviorTreeComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	UBlackboardComponent* BlackboardComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	UAIPerceptionComponent* PerceptionComp;

	/** Start the assigned behavior tree (called from enemy BeginPlay if needed). */
	bool StartBehaviorTree(UBehaviorTree* Tree);

	/**
	 * If Tree has no RootNode, builds the LinkedSouls enemy graph in-memory:
	 * Root Selector + UpdateTarget service
	 *   Sequence: Chase → Attack
	 *   Patrol
	 */
	static void EnsureTreeGraph(UBehaviorTree* Tree);

protected:
	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	UPROPERTY()
	UAISenseConfig_Sight* SightConfig;

	void InitBlackboardKeys(APawn* InPawn);
};
