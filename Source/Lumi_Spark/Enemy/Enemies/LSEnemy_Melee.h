#pragma once

#include "CoreMinimal.h"
#include "Enemy/LSEnemyBase.h"
#include "LSEnemy_Melee.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLSMeleeAttackInterrupted);

/**
 * 近战冲锋兵 (ALSEnemy_Melee)
 */
UCLASS()
class LUMI_SPARK_API ALSEnemy_Melee : public ALSEnemyBase
{
	GENERATED_BODY()

public:
	ALSEnemy_Melee();

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	// 开始近战蓄力前摇
	UFUNCTION(BlueprintCallable, Category = "Enemy|Melee")
	void StartChargeAttack(AActor* Target);

	// 强制打断蓄力
	UFUNCTION(BlueprintCallable, Category = "Enemy|Melee")
	void InterruptAttack();

	UPROPERTY(BlueprintAssignable, Category = "Enemy|Melee|Events")
	FOnLSMeleeAttackInterrupted OnAttackInterrupted;

protected:
	void ExecuteMeleeStrike();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float MeleeDamage = 45.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float ChargeWindupDuration = 1.2f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float InterruptDamageThreshold = 50.0f;

private:
	bool bIsWindingUp = false;
	FTimerHandle WindupTimerHandle;
	TWeakObjectPtr<AActor> AttackTarget;
};