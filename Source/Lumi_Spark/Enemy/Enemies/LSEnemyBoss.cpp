#include "LSEnemyBoss.h"
#include "AIController.h"
#include "Character/LSHealthComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

ALSEnemyBoss::ALSEnemyBoss()
{
	ActiveGameplayTags.AddTag(LSTags::TAG_Enemy_Type_Boss);
	ActiveGameplayTags.AddTag(LSTags::TAG_State_SuperArmor); // Boss 全程自带免击飞霸体

	// 默认预置 3 阶段配置
	FLSBossPhase Phase1;
	Phase1.HPThreshold = 1.0f;
	Phase1.DamageMultiplier = 1.0f;
	Phase1.SpeedMultiplier = 1.0f;

	FLSBossPhase Phase2;
	Phase2.HPThreshold = 0.7f;
	Phase2.DamageMultiplier = 1.25f;
	Phase2.SpeedMultiplier = 1.2f;

	FLSBossPhase Phase3;
	Phase3.HPThreshold = 0.3f;
	Phase3.DamageMultiplier = 1.6f;
	Phase3.SpeedMultiplier = 1.4f;

	Phases.Add(Phase1);
	Phases.Add(Phase2);
	Phases.Add(Phase3);
}

float ALSEnemyBoss::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// 转场无敌帧期间，完全免疫伤害
	if (ActiveGameplayTags.HasTag(LSTags::TAG_State_Invincible))
	{
		return 0.0f;
	}

	const float ActualDmg = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	CheckPhaseTransition();

	return ActualDmg;
}

void ALSEnemyBoss::CheckPhaseTransition()
{
	if (!HealthComponent || CurrentPhaseIndex + 1 >= Phases.Num()) return;

	const float HPPercent = HealthComponent->GetHealthPercent();
	const int32 NextIndex = CurrentPhaseIndex + 1;

	if (HPPercent <= Phases[NextIndex].HPThreshold)
	{
		EnterPhase(NextIndex);
	}
}

void ALSEnemyBoss::EnterPhase(int32 NextPhaseIndex)
{
	const int32 OldIndex = CurrentPhaseIndex;
	CurrentPhaseIndex = NextPhaseIndex;

	// 1. 激活转场无敌状态（持续 2.0s 锁血并演出）
	ActiveGameplayTags.AddTag(LSTags::TAG_State_Invincible);
	GetWorldTimerManager().SetTimer(TransitionTimerHandle, this, &ALSEnemyBoss::EndTransitionInvincibility, 2.0f, false);

	// 2. 赋予狂暴移速加成
	const float BaseSpeed = 450.0f;
	GetCharacterMovement()->MaxWalkSpeed = BaseSpeed * Phases[CurrentPhaseIndex].SpeedMultiplier;

	// 3. 动态热切换下一阶段的行为树
	if (Phases[CurrentPhaseIndex].PhaseBehaviorTree)
	{
		if (AAIController* AICon = Cast<AAIController>(GetController()))
		{
			AICon->RunBehaviorTree(Phases[CurrentPhaseIndex].PhaseBehaviorTree);
		}
	}

	// 4. 广播转场委托给关卡和 HUD
	OnPhaseChange.Broadcast(OldIndex, CurrentPhaseIndex);
}

void ALSEnemyBoss::EndTransitionInvincibility()
{
	ActiveGameplayTags.RemoveTag(LSTags::TAG_State_Invincible);
}