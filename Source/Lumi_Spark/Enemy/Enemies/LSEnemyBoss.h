#pragma once

#include "CoreMinimal.h"
#include "Enemy/LSEnemyBase.h"
#include "LSEnemyBoss.generated.h"

class UBehaviorTree;

/**
 * Boss 阶段配置结构体
 */
USTRUCT(BlueprintType)
struct LUMI_SPARK_API FLSBossPhase
{
	GENERATED_BODY()

	// 触发此阶段的血量比例门槛 (如 0.7 代表血量跌破 70% 转入)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Phase")
	float HPThreshold = 0.5f;

	// 该阶段切换的独立行为树
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Phase")
	TObjectPtr<UBehaviorTree> PhaseBehaviorTree = nullptr;

	// 狂暴伤害放大系数
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Phase")
	float DamageMultiplier = 1.0f;

	// 狂暴移速放大系数
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Phase")
	float SpeedMultiplier = 1.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLSBossPhaseChanged, int32, OldPhase, int32, NewPhase);

/**
 * 多阶段首领 Boss 基类 (ALSEnemyBoss)
 */
UCLASS()
class LUMI_SPARK_API ALSEnemyBoss : public ALSEnemyBase
{
	GENERATED_BODY()

public:
	ALSEnemyBoss();

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	UPROPERTY(BlueprintAssignable, Category = "Boss|Events")
	FOnLSBossPhaseChanged OnPhaseChange;

protected:
	void CheckPhaseTransition();
	void EnterPhase(int32 NextPhaseIndex);
	void EndTransitionInvincibility();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Phases")
	TArray<FLSBossPhase> Phases;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Boss|Phases")
	int32 CurrentPhaseIndex = 0;

private:
	FTimerHandle TransitionTimerHandle;
};