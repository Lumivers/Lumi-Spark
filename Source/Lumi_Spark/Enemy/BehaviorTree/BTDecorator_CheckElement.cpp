#include "BTDecorator_CheckElement.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Element/LSElementComponent.h"
#include "Core/LSTypes.h"

UBTDecorator_CheckElement::UBTDecorator_CheckElement()
{
	NodeName = TEXT("目标元素检测 (Check Element)");
	RequiredElementTag = LSTags::TAG_Element_Hydro; // 默认查水附着
}

bool UBTDecorator_CheckElement::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return false;

	AActor* TargetActor = Cast<AActor>(BB->GetValueAsObject(LSBlackboardKeys::TargetActor));
	if (!TargetActor) return false;

	bool bHasElement = false;
	if (ULSElementComponent* ElemComp = TargetActor->FindComponentByClass<ULSElementComponent>())
	{
		bHasElement = ElemComp->HasElementAura(RequiredElementTag);
	}

	return bInvertCondition ? !bHasElement : bHasElement;
}