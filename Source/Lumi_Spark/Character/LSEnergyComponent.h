#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "LSEnergyComponent.generated.h"

// 能量槽变动广播：(当前能量, 最大能量)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLSEnergyChanged, float, CurrentEnergy, float, MaxEnergy);
// 能量已充满广播
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLSEnergyFull);

/**
 * 元素大招能量组件 (ULSEnergyComponent)
 * 纳管 Q 技能能量池、同色微粒 3.0x 加成、后台被动微量充能与网络属性同步
 */
UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class LUMI_SPARK_API ULSEnergyComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSEnergyComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ═══ 充能与消耗 ═══

	// 主动注入能量（受元素充能效率加成）
	UFUNCTION(BlueprintCallable, Category = "Combat|Energy")
	void AddEnergy(float Amount);

	// 消耗能量释放大招
	UFUNCTION(BlueprintCallable, Category = "Combat|Energy")
	bool ConsumeEnergy(float Amount);

	// 拾取元素微粒（同色 3.0x / 异色 1.0x / 无色 2.0x 加成）
	UFUNCTION(BlueprintCallable, Category = "Combat|Energy")
	void CollectParticle(FGameplayTag ParticleElement, float BaseEnergy);

	// 初始化能量配置
	UFUNCTION(BlueprintCallable, Category = "Combat|Energy")
	void InitializeEnergy(float InMaxEnergy, float InRechargeRate = 1.0f);

	// ═══ 查询接口 ═══

	UFUNCTION(BlueprintPure, Category = "Combat|Energy")
	float GetCurrentEnergy() const { return CurrentEnergy; }

	UFUNCTION(BlueprintPure, Category = "Combat|Energy")
	float GetMaxEnergy() const { return MaxEnergy; }

	UFUNCTION(BlueprintPure, Category = "Combat|Energy")
	float GetEnergyRatio() const { return MaxEnergy > 0.0f ? (CurrentEnergy / MaxEnergy) : 0.0f; }

	UFUNCTION(BlueprintPure, Category = "Combat|Energy")
	bool IsEnergyFull() const { return CurrentEnergy >= MaxEnergy; }

public:
	// ═══ 委托广播 ═══

	UPROPERTY(BlueprintAssignable, Category = "Combat|Energy|Events")
	FOnLSEnergyChanged OnEnergyChanged;

	UPROPERTY(BlueprintAssignable, Category = "Combat|Energy|Events")
	FOnLSEnergyFull OnEnergyFull;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_CurrentEnergy, VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat|Energy")
	float CurrentEnergy = 0.0f;

	UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Energy", meta = (ClampMin = "10.0"))
	float MaxEnergy = 60.0f;

	// 元素充能效率（1.0 = 100%）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Energy", meta = (ClampMin = "0.5"))
	float EnergyRechargeRate = 1.0f;

	// 后台待命时的被动微量回能（点/秒）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Energy", meta = (ClampMin = "0.0"))
	float PassiveRechargeRate = 0.5f;

	// 是否开启被动充能
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Energy")
	bool bEnablePassiveRecharge = true;

	UFUNCTION()
	void OnRep_CurrentEnergy();
};