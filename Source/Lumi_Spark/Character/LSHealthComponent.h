#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "LSHealthComponent.generated.h"

class ULSShieldComponent;

// 生命组件广播委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnLSCompHealthChanged, float, CurrentHealth, float, MaxHealth, bool, bIsDamage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLSCompDeath);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLSCompLowHealth, float, CurrentHealth, float, MaxHealth);

/**
 * 独立生命组件 (ULSHealthComponent)
 */
UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class LUMI_SPARK_API ULSHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSHealthComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 承伤结算入口（带护盾拦截）
	UFUNCTION(BlueprintCallable, Category = "Combat|Health")
	float TakeDamage(float DamageAmount, FGameplayTag DamageElement = FGameplayTag(), AActor* DamageCauser = nullptr, AController* InstigatorController = nullptr);

	// 治疗恢复
	UFUNCTION(BlueprintCallable, Category = "Combat|Health")
	float Heal(float HealAmount);

	// 属性初始化
	UFUNCTION(BlueprintCallable, Category = "Combat|Health")
	void InitializeHealth(float InMaxHealth, float InCurrentHealth = -1.0f);

	// 状态查询
	UFUNCTION(BlueprintPure, Category = "Combat|Health")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "Combat|Health")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Combat|Health")
	float GetHealthPercent() const { return MaxHealth > 0.0f ? (CurrentHealth / MaxHealth) : 0.0f; }

	UFUNCTION(BlueprintPure, Category = "Combat|Health")
	bool IsDead() const { return bIsDead; }

	UFUNCTION(BlueprintCallable, Category = "Combat|Health")
	void SetDead(bool bNewDead) { bIsDead = bNewDead; }

public:
	UPROPERTY(BlueprintAssignable, Category = "Combat|Health|Events")
	FOnLSCompHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Combat|Health|Events")
	FOnLSCompDeath OnDeath;

	UPROPERTY(BlueprintAssignable, Category = "Combat|Health|Events")
	FOnLSCompLowHealth OnLowHealth;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth, VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat|Health")
	float CurrentHealth = 1000.0f;

	UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Health", meta = (ClampMin = "1.0"))
	float MaxHealth = 1000.0f;

	UPROPERTY(ReplicatedUsing = OnRep_IsDead, VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat|Health")
	bool bIsDead = false;

	// 脱战自然回血速度（点/秒，0为不回血）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Health|Regen", meta = (ClampMin = "0.0"))
	float RegenRate = 0.0f;

	// 脱战回血延迟等待时间（秒）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Health|Regen", meta = (ClampMin = "0.0"))
	float RegenDelay = 5.0f;

	// 低血量告警阈值比例（默认 20%）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Health|Regen", meta = (ClampMin = "0.05", ClampMax = "0.5"))
	float LowHealthThreshold = 0.2f;

	UFUNCTION()
	void OnRep_CurrentHealth();

	UFUNCTION()
	void OnRep_IsDead();

private:
	float TimeSinceLastDamage = 0.0f;
	bool bLowHealthWarningTriggered = false;
};