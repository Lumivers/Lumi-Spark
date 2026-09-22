#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LSStaminaComponent.generated.h"

// 体力变动广播：(当前体力, 最大体力)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLSStaminaChanged, float, CurrentStamina, float, MaxStamina);
// 体力耗尽广播
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLSStaminaExhausted);

/**
 * 体力管理组件 (ULSStaminaComponent)
 * 纳管冲刺持续消耗、闪避单次扣减、延迟自然恢复与按需使能 Tick
 */
UCLASS(ClassGroup=(Movement), meta=(BlueprintSpawnableComponent))
class LUMI_SPARK_API ULSStaminaComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSStaminaComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ═══ 消耗与恢复 ═══

	// 尝试消耗指定体力，返回是否消耗成功
	UFUNCTION(BlueprintCallable, Category = "Movement|Stamina")
	bool ConsumeStamina(float Amount);

	// 查询是否有充足体力
	UFUNCTION(BlueprintPure, Category = "Movement|Stamina")
	bool HasEnoughStamina(float Amount) const { return CurrentStamina >= Amount; }

	// 主动恢复体力
	UFUNCTION(BlueprintCallable, Category = "Movement|Stamina")
	void RestoreStamina(float Amount);

	// 属性重置/初始化
	UFUNCTION(BlueprintCallable, Category = "Movement|Stamina")
	void InitializeStamina(float InMaxStamina);

	// ═══ 属性查询 ═══

	UFUNCTION(BlueprintPure, Category = "Movement|Stamina")
	float GetCurrentStamina() const { return CurrentStamina; }

	UFUNCTION(BlueprintPure, Category = "Movement|Stamina")
	float GetMaxStamina() const { return MaxStamina; }

	UFUNCTION(BlueprintPure, Category = "Movement|Stamina")
	float GetStaminaRatio() const { return MaxStamina > 0.0f ? (CurrentStamina / MaxStamina) : 0.0f; }

	UFUNCTION(BlueprintPure, Category = "Movement|Stamina")
	float GetSprintCostPerSecond() const { return SprintCostPerSecond; }

	UFUNCTION(BlueprintPure, Category = "Movement|Stamina")
	float GetDashCost() const { return DashCost; }

public:
	// ═══ 委托广播 ═══

	UPROPERTY(BlueprintAssignable, Category = "Movement|Stamina|Events")
	FOnLSStaminaChanged OnStaminaChanged;

	UPROPERTY(BlueprintAssignable, Category = "Movement|Stamina|Events")
	FOnLSStaminaExhausted OnStaminaExhausted;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Stamina", meta = (ClampMin = "10.0"))
	float MaxStamina = 240.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Movement|Stamina")
	float CurrentStamina = 240.0f;

	// 自然回复速率（点/秒）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Stamina", meta = (ClampMin = "1.0"))
	float RecoveryRate = 30.0f;

	// 停止消耗后进入恢复的延迟等待时间（秒）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Stamina", meta = (ClampMin = "0.0"))
	float RecoveryDelay = 1.5f;

	// 冲刺每秒消耗体力
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Stamina", meta = (ClampMin = "1.0"))
	float SprintCostPerSecond = 18.0f;

	// 闪避单次消耗体力
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Stamina", meta = (ClampMin = "1.0"))
	float DashCost = 18.0f;

private:
	float TimeSinceLastConsume = 0.0f;
	bool bIsExhausted = false;
};