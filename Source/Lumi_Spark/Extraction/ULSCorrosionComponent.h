#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Extraction/LSExtractionTypes.h"
#include "ULSCorrosionComponent.generated.h"

class ALSCharacterBase;
class ULSBackpackComponent;

// ─── 委托声明 ───
// 侵蚀度与滤芯数据刷新（供局内 HUD 侵蚀计量表与暗角后处理实时绑定）
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnLSCorrosionUpdated, float, CurrentCorrosion, float, MaxCorrosion, float, FilterDurability);

// 侵蚀过载（满 100%）状态进退广播
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLSCorrosionOverloadChanged, bool, bIsOverloaded);

// 滤芯耗尽告警广播
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLSFilterDepleted);

// 更换新滤芯广播
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLSFilterInstalled, float, AddedDurability);

/**
 * 地脉侵蚀与防护系统组件 (ULSCorrosionComponent)
 * 挂载于 ALSPlayerController（全队换人不换侵蚀度），掌管环境压力、滤芯消耗、过载惩罚与战术解药
 */
UCLASS(ClassGroup = (Extraction), meta = (BlueprintSpawnableComponent))
class LUMI_SPARK_API ULSCorrosionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSCorrosionComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ═══ 核心配置参数 ═══

	/** 基础侵蚀增长速率（点/秒，无滤芯加速前基准：0.5 点/秒，约 200 秒满溢） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Corrosion|Config")
	float BaseCorrosionRate = 0.5f;

	/** 滤芯耗尽后的侵蚀加速倍率（失去防护后加速 2.5 倍，达 1.25 点/秒） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Corrosion|Config")
	float DepletedCorrosionMultiplier = 2.5f;

	/** 区域地脉浓度加成系数（进入高危死生裂隙或富集区时动态提升，默认 1.0） */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Corrosion|Config")
	float ZoneCorrosionMultiplier = 1.0f;

	/** 滤芯耐久消耗速率（点/秒，默认 1.0 秒耐久对应真实 1 秒） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Corrosion|Config")
	float FilterConsumptionRate = 1.0f;

	/** 滤芯最大耐久容量（秒） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Corrosion|Config")
	float MaxFilterDurability = 180.0f;

	/** 侵蚀过载扣血周期（秒，默认每 2.0 秒扣除一次当前在场角色真实生命） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Corrosion|Overload")
	float OverloadDamageInterval = 2.0f;

	/** 侵蚀过载每次扣血占最大生命值百分比（默认 6%，即每 2 秒损失 6% 最大生命） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Corrosion|Overload")
	float OverloadDamagePercent = 0.06f;

	/** 侵蚀过载移速衰减倍率（默认 0.70x，降低 30% 机动性） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Corrosion|Overload")
	float OverloadSpeedMultiplier = 0.70f;

	// ═══ 运行时网络同步状态 ═══

	/** 当前侵蚀计量（0.0 ~ 100.0） */
	UPROPERTY(ReplicatedUsing = OnRep_CurrentCorrosion, VisibleInstanceOnly, BlueprintReadOnly, Category = "Corrosion|State")
	float CurrentCorrosion = 0.0f;

	/** 侵蚀计量上限 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Corrosion|State")
	float MaxCorrosion = 100.0f;

	/** 当前滤芯剩余耐久（秒） */
	UPROPERTY(ReplicatedUsing = OnRep_FilterDurability, VisibleInstanceOnly, BlueprintReadOnly, Category = "Corrosion|State")
	float CurrentFilterDurability = 120.0f;

	/** 是否处于侵蚀过载状态（Corrosion >= 100%） */
	UPROPERTY(ReplicatedUsing = OnRep_IsOverloaded, VisibleInstanceOnly, BlueprintReadOnly, Category = "Corrosion|State")
	bool bIsOverloaded = false;

	// ═══ 战术行为交互 ═══

	/**
	 * 使用净化针（瞬间清除侵蚀度）
	 * @param CleanseAmount 清除点数，默认 100.0f 全额清空
	 */
	UFUNCTION(BlueprintCallable, Category = "Corrosion|Action")
	bool UsePurificationInjector(float CleanseAmount = 100.0f);

	/**
	 * 更换/安装滤芯
	 * @param DurabilityAmount 补充的滤芯耐久秒数
	 */
	UFUNCTION(BlueprintCallable, Category = "Corrosion|Action")
	bool InstallFilter(float DurabilityAmount);

	/** 动态调整所处地脉区域的浓度倍率 */
	UFUNCTION(BlueprintCallable, Category = "Corrosion|Config")
	void SetZoneCorrosionMultiplier(float NewMultiplier);

	/** 从背包中检索并消耗 1 个净化针 */
	UFUNCTION(BlueprintCallable, Category = "Corrosion|Backpack")
	bool ConsumeInjectorFromBackpack();

	/** 从背包中检索并消耗 1 个抗侵蚀滤芯 */
	UFUNCTION(BlueprintCallable, Category = "Corrosion|Backpack")
	bool ConsumeFilterFromBackpack();

	// ═══ 状态查询接口 ═══

	/** 侵蚀度百分比 [0.0, 1.0] */
	UFUNCTION(BlueprintPure, Category = "Corrosion|Query")
	float GetCorrosionPercent() const { return MaxCorrosion > 0.0f ? (CurrentCorrosion / MaxCorrosion) : 0.0f; }

	/** 滤芯耐久百分比 [0.0, 1.0] */
	UFUNCTION(BlueprintPure, Category = "Corrosion|Query")
	float GetFilterPercent() const { return MaxFilterDurability > 0.0f ? (CurrentFilterDurability / MaxFilterDurability) : 0.0f; }

	/** 滤芯是否正在生效阻隔侵蚀 */
	UFUNCTION(BlueprintPure, Category = "Corrosion|Query")
	bool HasActiveFilter() const { return CurrentFilterDurability > 0.0f; }

	/** 是否过载 */
	UFUNCTION(BlueprintPure, Category = "Corrosion|Query")
	bool IsOverloaded() const { return bIsOverloaded; }

	/** 计算当前视效暗角/边缘泛红强度（0.0 无，1.0 满屏警示脉冲） */
	UFUNCTION(BlueprintPure, Category = "Corrosion|Query")
	float GetVignetteIntensity() const;

	/** 获取当前机动移速倍率（过载时返回 0.7，正常返回 1.0） */
	UFUNCTION(BlueprintPure, Category = "Corrosion|Query")
	float GetCorrosionSpeedMultiplier() const { return bIsOverloaded ? OverloadSpeedMultiplier : 1.0f; }

	// ═══ 委托事件 ═══
	UPROPERTY(BlueprintAssignable, Category = "Corrosion|Events")
	FOnLSCorrosionUpdated OnCorrosionUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Corrosion|Events")
	FOnLSCorrosionOverloadChanged OnCorrosionOverloadChanged;

	UPROPERTY(BlueprintAssignable, Category = "Corrosion|Events")
	FOnLSFilterDepleted OnFilterDepleted;

	UPROPERTY(BlueprintAssignable, Category = "Corrosion|Events")
	FOnLSFilterInstalled OnFilterInstalled;

protected:
	UFUNCTION()
	void OnRep_CurrentCorrosion();

	UFUNCTION()
	void OnRep_FilterDurability();

	UFUNCTION()
	void OnRep_IsOverloaded();

private:
	// 过载伤害周期计时器
	float OverloadDamageTimer = 0.0f;

	// 获取当前玩家控制的在场角色
	ALSCharacterBase* GetActiveCharacter() const;

	// 执行过载百分比扣血
	void ApplyOverloadDamage();
};