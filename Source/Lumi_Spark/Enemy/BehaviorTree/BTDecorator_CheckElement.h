#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "GameplayTagContainer.h"
#include "BTDecorator_CheckElement.generated.h"

/**
 * 目标元素附着检测装饰器 (UBTDecorator_CheckElement)
 */
UCLASS()
class LUMI_SPARK_API UBTDecorator_CheckElement : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_CheckElement();

	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

protected:
	// 期望检测的元素类型（如 TAG_Element_Hydro，当玩家潮湿时触发）
	UPROPERTY(EditAnywhere, Category = "Element")
	FGameplayTag RequiredElementTag;

	// 是否反转条件（如检测“目标身上没有火”）
	UPROPERTY(EditAnywhere, Category = "Element")
	bool bInvertCondition = false;
};