#include "BTTask_ElementalAttack.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Element/LSElementComponent.h"
#include "Core/LSEventBus.h"
#include "Engine/DamageEvents.h"
#include "Core/LSTypes.h"

UBTTask_ElementalAttack::UBTTask_ElementalAttack()
{
	NodeName = TEXT("释放元素战技 (Elemental Attack)");
	AttackElementTag = LSTags::TAG_Element_Pyro; // 默认火系
}

EBTNodeResult::Type UBTTask_ElementalAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AICon = OwnerComp.GetAIOwner();
	APawn* AIPawn = AICon ? AICon->GetPawn() : nullptr;
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!AIPawn || !BB) return EBTNodeResult::Failed;

	AActor* TargetActor = Cast<AActor>(BB->GetValueAsObject(LSBlackboardKeys::TargetActor));
	if (!TargetActor) return EBTNodeResult::Failed;

	// 锁定朝向
	AICon->SetFocus(TargetActor);

	// 施加元素附着
	if (ULSElementComponent* TargetElemComp = TargetActor->FindComponentByClass<ULSElementComponent>())
	{
		TargetElemComp->ApplyElement(AIPawn, AttackElementTag, AttackElementGauge);
	}

	// 扣除伤害
	TargetActor->TakeDamage(AttackDamage, FDamageEvent(), AICon, AIPawn);

	// 广播全局总线
	if (ULSEventBus* Bus = ULSEventBus::Get(this))
	{
		FLSDamageContext Context;
		Context.DamageCauser = AIPawn;
		Context.TargetActor = TargetActor;
		Context.BaseDamage = AttackDamage;
		Context.FinalDamage = AttackDamage;
		Context.ElementTag = AttackElementTag;
		Context.DamageTypeTag = LSTags::TAG_Damage_Type_Skill;
		Bus->OnDamageDealt.Broadcast(Context);
	}

	return EBTNodeResult::Succeeded;
}