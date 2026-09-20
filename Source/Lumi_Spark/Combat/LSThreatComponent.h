#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LSThreatComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLSThreatTargetChanged, AActor*, OldTarget, AActor*, NewTarget);

/**
 * 多目标动态仇恨积分表组件 (Threat / Aggro Table)
 * 挂载于怪物或 Boss 身上，追踪所有玩家单位时间内的伤害、反应与救援行为仇恨
 */
UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class LUMI_SPARK_API ULSThreatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSThreatComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	//直接增加指定目标的仇恨值
	UFUNCTION(BlueprintCallable, Category = "Threat")
	void AddThreat(AActor* ThreatCauser, float ThreatAmount);

	/**
	 * 根据造成的伤害增加仇恨
	 * @param DamageCauser 攻击方
	 * @param Damage 实际造成的伤害
	 * @param bIsReaction 是否为元素反应伤害（反应伤害享有 1.5x 仇恨加权）
	 */
	UFUNCTION(BlueprintCallable, Category = "Threat")
	void AddThreatFromDamage(AActor* DamageCauser, float Damage, bool bIsReaction);

	//救援动作产生巨额仇恨
	UFUNCTION(BlueprintCallable, Category = "Threat")
	void AddThreatFromRevive(AActor* Reviver, float DeltaSeconds);

	//获取当前全场仇恨最高的合法目标
	UFUNCTION(BlueprintPure, Category = "Threat")
	AActor* GetHighestThreatTarget() const;

	//清空仇恨表（目标死亡或怪物重置时）
	UFUNCTION(BlueprintCallable, Category = "Threat")
	void ClearAllThreat();

public:
	UPROPERTY(BlueprintAssignable, Category = "Threat|Events")
	FOnLSThreatTargetChanged OnThreatTargetChanged;

protected:
	//仇恨积分映射表 (Actor -> 累计仇恨值)
	TMap<TWeakObjectPtr<AActor>, float> ThreatMap;

	//当前仇恨最高锁定目标
	TWeakObjectPtr<AActor> CurrentHighestTarget;

	//仇恨自然衰减速率（每秒衰减百分比，默认 5%/s）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Threat|Config")
	float ThreatDecayRate = 0.05f;

	//救援拉怪仇恨系数（每秒产生的基准仇恨值）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Threat|Config")
	float ReviveThreatPerSecond = 300.0f;

private:
	void EvaluateHighestThreatTarget();
};