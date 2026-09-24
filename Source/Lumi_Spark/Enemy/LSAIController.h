#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "Core/LSTypes.h"
#include "LSAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class UAISenseConfig_Damage;
class UBehaviorTreeComponent;
class UBlackboardComponent;
class ALSEnemyBase;

/**
 * 敌人战术 AI 控制器基类 (ALSAIController)
 */
UCLASS()
class LUMI_SPARK_API ALSAIController : public AAIController
{
	GENERATED_BODY()

public:
	ALSAIController();

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	// 感知组件全局更新回调
	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	// 获取当前锁定目标
	UFUNCTION(BlueprintPure, Category = "AI")
	AActor* GetTargetActor() const;

protected:
	// 动态决议最高仇恨目标并写入黑板
	void UpdateBestTarget();

	// 5秒脱失记忆计时到期处理
	void HandleLoseSightExpired();

protected:
	// ─── 感知配置组件 ───
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	TObjectPtr<UAIPerceptionComponent> AIPerceptionComp;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Damage> DamageConfig;

	// ─── 感知数值参数 ───
	UPROPERTY(EditDefaultsOnly, Category = "AI|Perception")
	float SightRadius = 2000.0f;           // 20米视距

	UPROPERTY(EditDefaultsOnly, Category = "AI|Perception")
	float LoseSightRadius = 2500.0f;       // 25米脱离视距

	UPROPERTY(EditDefaultsOnly, Category = "AI|Perception")
	float SightAngle = 90.0f;              // 90度视场半角

	UPROPERTY(EditDefaultsOnly, Category = "AI|Perception")
	float HearingRange = 3000.0f;          // 30米枪声感知

	UPROPERTY(EditDefaultsOnly, Category = "AI|Perception")
	float LoseSightDuration = 5.0f;        // 丢失视野后持续追踪记忆5秒

private:
	FTimerHandle LoseSightTimerHandle;

	// 弱引用被控制的敌人 Pawn
	TWeakObjectPtr<ALSEnemyBase> ControlledEnemy;
};