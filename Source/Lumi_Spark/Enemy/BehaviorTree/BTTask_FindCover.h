#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_FindCover.generated.h"

UCLASS()
class LUMI_SPARK_API UBTTask_FindCover : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBTTask_FindCover();
	
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	
protected:
	UPROPERTY(EditAnywhere, Category = "Cover")
	float MaxSearchRadius = 1000.0f;
};