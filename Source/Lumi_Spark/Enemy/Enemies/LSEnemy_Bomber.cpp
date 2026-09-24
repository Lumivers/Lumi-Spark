#include "LSEnemy_Bomber.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Element/LSElementComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/OverlapResult.h"
#include "DrawDebugHelpers.h"

ALSEnemy_Bomber::ALSEnemy_Bomber()
{
	ActiveGameplayTags.AddTag(LSTags::TAG_Enemy_Type_Bomber);
	GetCharacterMovement()->MaxWalkSpeed = 650.0f; // 高速冲刺奔跑
	ExplosionElementTag = LSTags::TAG_Element_Pyro; // 默认火系自爆伤害
}

void ALSEnemy_Bomber::OnDeath(AActor* Killer)
{
	if (!bHasDetonated)
	{
		Detonate();
	}
	Super::OnDeath(Killer);
}

void ALSEnemy_Bomber::Detonate()
{
	if (bHasDetonated) return;
	bHasDetonated = true;

	const FVector Center = GetActorLocation();
	DrawDebugSphere(GetWorld(), Center, ExplosionRadius, 16, FColor::Orange, false, 2.0f);

	// 范围搜寻所有 Pawn（无差别伤害：包括玩家与友军怪物，产生连锁爆炸！）
	TArray<FOverlapResult> Overlaps;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(ExplosionRadius);
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	GetWorld()->OverlapMultiByChannel(Overlaps, Center, FQuat::Identity, ECC_Pawn, Sphere, QueryParams);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		if (AActor* Victim = Overlap.GetActor())
		{
			// 施加元素附着
			if (ULSElementComponent* ElemComp = Victim->FindComponentByClass<ULSElementComponent>())
			{
				ElemComp->ApplyElement(this, ExplosionElementTag, ELSElementGauge::Heavy);
			}

			// 扣血
			Victim->TakeDamage(ExplosionDamage, FDamageEvent(), GetController(), this);
		}
	}

	SetLifeSpan(0.1f);
}