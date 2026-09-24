#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_FlankPlayer.generated.h"

UCLASS()
class LUMI_SPARK_API UBTTask_FlankPlayer : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBTTask_FlankPlayer();
	
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	
protected:
	//期望包抄夹角
	UPROPERTY(EditAnywhere, Category = "Flank", meta = (ClampMin = "60.0", ClampMax = "120.0"))
	float DesiredFlankAngle = 75.0f;
	
	//包抄半径
	UPROPERTY(EditAnywhere, Category = "Flank")
	float FlankRadius = 900.0f;
};