#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_PatrolMove.generated.h"

UCLASS()
class LINKEDSOULS_API UBTTask_PatrolMove : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_PatrolMove();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	UPROPERTY(EditAnywhere, Category = "Patrol")
	float PatrolRadius = 500.f;

	UPROPERTY(EditAnywhere, Category = "Patrol")
	float AcceptanceRadius = 50.f;
};
