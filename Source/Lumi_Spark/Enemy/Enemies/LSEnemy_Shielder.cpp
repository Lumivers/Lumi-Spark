#include "LSEnemy_Shielder.h"
#include "DrawDebugHelpers.h"

ALSEnemy_Shielder::ALSEnemy_Shielder()
{
	ActiveGameplayTags.AddTag(LSTags::TAG_Enemy_Type_Shielder);
}

float ALSEnemy_Shielder::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead || DamageAmount <= 0.0f) return 0.0f;

	// 若处于瘫痪/冻结状态，盾牌防线崩溃，全额吃伤
	if (ActiveGameplayTags.HasTag(LSTags::TAG_State_Stunned))
	{
		return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	}

	// 判定攻击方向是否在正面盾牌格挡范围内
	if (DamageCauser)
	{
		const FVector DirToCauser = (DamageCauser->GetActorLocation() - GetActorLocation()).GetSafeNormal();
		const float ForwardDot = FVector::DotProduct(GetActorForwardVector(), DirToCauser);

		if (ForwardDot > BlockDotThreshold)
		{
			// 正面格挡,跳出蓝色格挡火花，完全免疫普通子弹伤害
			DrawDebugPoint(GetWorld(), GetActorLocation() + GetActorForwardVector() * 60.0f + FVector(0, 0, 50.0f), 15.0f, FColor::Cyan, false, 0.4f);
			return 0.0f;
		}
	}

	// 绕后射击或非正面伤害，正常结算
	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}