#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayTagContainer.h"
#include "Core/LSTypes.h"
#include "BTTask_ElementalAttack.generated.h"

UCLASS()
class LUMI_SPARK_API UBTTask_ElementalAttack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_ElementalAttack();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Combat")
	float AttackDamage = 60.0f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	FGameplayTag AttackElementTag;

	UPROPERTY(EditAnywhere, Category = "Combat")
	ELSElementGauge AttackElementGauge = ELSElementGauge::Light;
};