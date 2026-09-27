#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Extraction/LSExtractionTypes.h"
#include "ULSCorrosionComponent.generated.h"

class ALSCharacterBase;

// ─── 委托声明 ───
// 侵蚀度与面罩数据刷新（供局内 HUD 与暗角后处理绑定）
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnLSCorrosionUpdated, float, CurrentCorrosion, float, MaxCorrosion, int32, MaskDurability);

// 侵蚀过载（满 100%）状态进退广播
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLSCorrosionOverloadChanged, bool, bIsOverloaded);

// 面罩彻底报废损坏广播
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLSMaskDepleted);

/**
 * 地脉侵蚀与抗蚀器系统组件 (ULSCorrosionComponent)
 * 挂载于 ALSPlayerController（全队换人不换侵蚀度）
 * 规则：
 * 1. 局内只算侵蚀度累积（抗蚀器完好时按品质降低 10%/25%/40%/55% 累积速度，损坏则全额侵蚀）；
 * 2. 队伍承伤减免（抗蚀器完好时提供 10%/20%/30%/45% 全队减伤）；
 * 3. 耐久采用离散点数制（绿4/蓝6/紫8/金10）
 * 4. 局外撤离结算时统一扣点（简单-1、中等-1、困难-2、高危-3）
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

	/** 基础侵蚀增长速率（点/秒，无抗蚀器阻隔时：1.0 点/秒，100 秒满溢） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Corrosion|Config")
	float BaseCorrosionRate = 1.0f;

	/** 侵蚀过载扣血周期（秒，默认每 2.0 秒扣除一次当前出战角色真实生命） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Corrosion|Overload")
	float OverloadDamageInterval = 2.0f;

	/** 侵蚀过载每次扣血占最大生命值百分比（默认 6%） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Corrosion|Overload")
	float OverloadDamagePercent = 0.06f;

	/** 侵蚀满溢时最大生命回复上限削减幅度（默认 0.70，即 100% 侵蚀时只能回复至 30% 生命） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Corrosion|Overload")
	float MaxHealthCapReduction = 0.70f;

	// ═══ 运行时网络同步状态 ═══

	/** 当前装备的抗蚀器完整数据快照（包含品级、减侵蚀%、减伤%） */
	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Corrosion|Gear")
	FLSAntiCorrosionGearData EquippedAntiCorrosionGear;

	/** 当前抗蚀器剩余耐久点数（默认 0 代表空装/跑刀状态，局外出击扣减） */
	UPROPERTY(ReplicatedUsing = OnRep_MaskDurability, VisibleInstanceOnly, BlueprintReadOnly, Category = "Corrosion|Gear")
	int32 CurrentMaskDurability = 0;

	/** 当前侵蚀计量（0.0 ~ 100.0） */
	UPROPERTY(ReplicatedUsing = OnRep_CurrentCorrosion, VisibleInstanceOnly, BlueprintReadOnly, Category = "Corrosion|State")
	float CurrentCorrosion = 0.0f;

	/** 侵蚀计量上限 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Corrosion|State")
	float MaxCorrosion = 100.0f;

	/** 当前是否身处地脉侵蚀区域 */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Corrosion|State")
	bool bIsInCorrosionZone = true;

	/** 是否处于侵蚀过载状态（Corrosion >= 100%） */
	UPROPERTY(ReplicatedUsing = OnRep_IsOverloaded, VisibleInstanceOnly, BlueprintReadOnly, Category = "Corrosion|State")
	bool bIsOverloaded = false;

	// ═══ 战术行为与结算接口 ═══

	/**
	 * 局内应急：使用战术净化针（扣除指定点数侵蚀度，解除过载）
	 * @param CleanseAmount 清除点数，默认 60.0f
	 */
	UFUNCTION(BlueprintCallable, Category = "Corrosion|Action")
	bool UsePurificationInjector(float CleanseAmount = 60.0f);

	/**
	 * 局外结算：出击结束一次性扣减抗蚀器耐久点数（简单-1、中等-1、困难-2、高危-3）
	 */
	UFUNCTION(BlueprintCallable, Category = "Corrosion|Durability")
	void ApplyRaidDurabilityDeduction(ELSHazardDifficulty RaidHazard);

	/** 局外装备全新抗蚀器（由智造工坊生产后装配） */
	UFUNCTION(BlueprintCallable, Category = "Corrosion|Gear")
	void EquipAntiCorrosionGear(ELSExtractionRarity InRarity);

	/** 调试/快捷设置面罩耐久点数 */
	UFUNCTION(BlueprintCallable, Category = "Corrosion|Gear")
	void EquipNewMask(int32 InDurability);

	/** 设置当前战区侵蚀环境开关 */
	UFUNCTION(BlueprintCallable, Category = "Corrosion|Config")
	void SetInCorrosionZone(bool bInZone) { bIsInCorrosionZone = bInZone; }

	// ═══ 状态查询接口 ═══

	/** 抗蚀器是否完好可用（耐久 > 0） */
	UFUNCTION(BlueprintPure, Category = "Corrosion|Query")
	bool HasActiveMask() const { return CurrentMaskDurability > 0; }

	/** 查询当前队伍受到的伤害减免比例（完好时生效，如金色减伤 45%） */
	UFUNCTION(BlueprintPure, Category = "Corrosion|Query")
	float GetTeamDamageMitigationRate() const
	{
		return (CurrentMaskDurability > 0) ? EquippedAntiCorrosionGear.DamageMitigation : 0.0f;
	}

	/** 侵蚀度百分比 [0.0, 1.0] */
	UFUNCTION(BlueprintPure, Category = "Corrosion|Query")
	float GetCorrosionPercent() const { return MaxCorrosion > 0.0f ? (CurrentCorrosion / MaxCorrosion) : 0.0f; }

	/** 抗蚀器耐久百分比 [0.0, 1.0] */
	UFUNCTION(BlueprintPure, Category = "Corrosion|Query")
	float GetMaskDurabilityPercent() const
	{
		return EquippedAntiCorrosionGear.MaxDurability > 0 ? (static_cast<float>(CurrentMaskDurability) / EquippedAntiCorrosionGear.MaxDurability) : 0.0f;
	}

	/** 获取当前抗蚀器最大耐久上限 */
	UFUNCTION(BlueprintPure, Category = "Corrosion|Query")
	int32 GetMaxMaskDurability() const { return EquippedAntiCorrosionGear.MaxDurability; }

	/** 是否过载 */
	UFUNCTION(BlueprintPure, Category = "Corrosion|Query")
	bool IsOverloaded() const { return bIsOverloaded; }

	/** 计算视效暗角与警示强度（0.0 ~ 1.0） */
	UFUNCTION(BlueprintPure, Category = "Corrosion|Query")
	float GetVignetteIntensity() const;

	/** 获取当前生命回复上限比例（随侵蚀度 0%~100% 从 1.0 线性压制到 0.3） */
	UFUNCTION(BlueprintPure, Category = "Corrosion|Query")
	float GetHealthRecoveryCapPercent() const
	{
		return FMath::Clamp(1.0f - (GetCorrosionPercent() * MaxHealthCapReduction), 0.20f, 1.0f);
	}

	// ═══ 委托事件 ═══
	UPROPERTY(BlueprintAssignable, Category = "Corrosion|Events")
	FOnLSCorrosionUpdated OnCorrosionUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Corrosion|Events")
	FOnLSCorrosionOverloadChanged OnCorrosionOverloadChanged;

	UPROPERTY(BlueprintAssignable, Category = "Corrosion|Events")
	FOnLSMaskDepleted OnMaskDepleted;

protected:
	UFUNCTION()
	void OnRep_CurrentCorrosion();

	UFUNCTION()
	void OnRep_MaskDurability();

	UFUNCTION()
	void OnRep_IsOverloaded();

private:
	float OverloadDamageTimer = 0.0f;

	ALSCharacterBase* GetActiveCharacter() const;
	void ApplyOverloadDamage();
};