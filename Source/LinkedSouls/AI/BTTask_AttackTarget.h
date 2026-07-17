#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_AttackTarget.generated.h"

struct FBTAttackTargetMemory
{
	float RemainingCooldown = 0.f;
};

UCLASS()
class LINKEDSOULS_API UBTTask_AttackTarget : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_AttackTarget();

	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FBTAttackTargetMemory); }
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Attack")
	float AttackRange = 200.f;

	UPROPERTY(EditAnywhere, Category = "Attack")
	float AttackCooldown = 1.5f;
};
