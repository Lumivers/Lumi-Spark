#include "LSEnemy_Melee.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/DamageEvents.h"

ALSEnemy_Melee::ALSEnemy_Melee()
{
	ActiveGameplayTags.AddTag(LSTags::TAG_Enemy_Type_Melee);
	GetCharacterMovement()->MaxWalkSpeed = 500.0f;
}

void ALSEnemy_Melee::StartChargeAttack(AActor* Target)
{
	if (bIsDead || bIsWindingUp || !Target) return;

	bIsWindingUp = true;
	AttackTarget = Target;

	// 蓄力前摇动作，1.2s 后执行挥砍
	GetWorldTimerManager().SetTimer(WindupTimerHandle, this, &ALSEnemy_Melee::ExecuteMeleeStrike, ChargeWindupDuration, false);
}

float ALSEnemy_Melee::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float FinalDmg = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	// 蓄力期间受到足额单发伤害，直接打断破招
	if (bIsWindingUp && FinalDmg >= InterruptDamageThreshold)
	{
		InterruptAttack();
	}

	return FinalDmg;
}

void ALSEnemy_Melee::InterruptAttack()
{
	if (!bIsWindingUp) return;

	bIsWindingUp = false;
	GetWorldTimerManager().ClearTimer(WindupTimerHandle);

	// 打断后短暂踉跄僵直 0.8s
	GetCharacterMovement()->StopMovementImmediately();
	OnAttackInterrupted.Broadcast();
}

void ALSEnemy_Melee::ExecuteMeleeStrike()
{
	bIsWindingUp = false;
	if (bIsDead || !AttackTarget.IsValid()) return;

	const float Distance = FVector::Dist(GetActorLocation(), AttackTarget->GetActorLocation());
	if (Distance <= 250.0f)
	{
		AttackTarget->TakeDamage(MeleeDamage, FDamageEvent(), GetController(), this);
	}
}