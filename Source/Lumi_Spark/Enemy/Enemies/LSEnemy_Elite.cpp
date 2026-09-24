#include "LSEnemy_Elite.h"
#include "Combat/LSShieldComponent.h"
#include "Element/LSElementComponent.h"
#include "Engine/DamageEvents.h"

ALSEnemy_Elite::ALSEnemy_Elite()
{
	ActiveGameplayTags.AddTag(LSTags::TAG_Enemy_Type_Elite);
	ActiveGameplayTags.AddTag(LSTags::TAG_State_SuperArmor); // 精英怪固有霸体
	SkillElementTag = LSTags::TAG_Element_Electro;
}

void ALSEnemy_Elite::BeginPlay()
{
	Super::BeginPlay();

	// 装配初始复合护盾（例如 400 点雷盾）
	if (ShieldComponent)
	{
		ShieldComponent->AddShieldLayer(LSTags::TAG_Element_Electro, 400.0f);
	}
}

void ALSEnemy_Elite::CastElementalSkill(AActor* Target)
{
	if (bIsDead || !Target) return;

	if (ULSElementComponent* TargetElem = Target->FindComponentByClass<ULSElementComponent>())
	{
		TargetElem->ApplyElement(this, SkillElementTag, ELSElementGauge::Heavy);
	}

	Target->TakeDamage(SkillDamage, FDamageEvent(), GetController(), this);
}