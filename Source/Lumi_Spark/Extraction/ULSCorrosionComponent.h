#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Extraction/LSExtractionTypes.h"
#include "ULSCorrosionComponent.generated.h"

class ALSCharacterBase;

// ─── 委托声明 ───
// 侵蚀度数据刷新（供局内 HUD 进度条与脉冲警告绑定）
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLSCorrosionUpdated, float, CurrentCorrosion, float, MaxCorrosion);

// 侵蚀过载（满 100% 软狂暴）状态进退广播
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLSCorrosionOverloadChanged, bool, bIsOverloaded);

/**
 * 战术地脉侵蚀倒计时中枢组件 (ULSCorrosionComponent)
 * 挂载于 ALSPlayerController
 * 
 * 核心机制（90 秒 / 4 分钟切片高光心流）：
 * 1. 侵蚀进度条 (0% ~ 100%)：每秒自动累加 0.4%（生存基准 250 秒）；
 * 2. 负向加速：受变异怪酸液攻击额外累加 5% ~ 10%；
 * 3. 正向解压：击杀变异怪或拾取地脉净化晶体回退 20%；
 * 4. 80% 警戒：视效泛绿脉冲警告；
 * 5. 100% 软狂暴：每秒扣除 8% 最大生命值。
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

	/** 基础侵蚀增长速率（点/秒，默认 0.4 点/秒，250 秒满溢） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Corrosion|Config")
	float BaseCorrosionRate = 0.4f;

	/** 侵蚀过载扣血周期（秒，默认每 1.0 秒执行一次生命流失） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Corrosion|Overload")
	float OverloadDamageInterval = 1.0f;

	/** 侵蚀过载每次扣血占最大生命值百分比（默认 8%，软狂暴真实伤害） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Corrosion|Overload")
	float OverloadDamagePercent = 0.08f;

	/** 侵蚀满溢时最大生命回复上限削减幅度（默认 0.70，即 100% 侵蚀时只能回复至 30% 生命） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Corrosion|Overload")
	float MaxHealthCapReduction = 0.70f;

	// ═══ 运行时网络同步状态 ═══

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

	// ═══ 战术行为与交互接口 ═══

	/**
	 * 增加侵蚀度（如受变异怪酸液喷吐打击增加 5%~10%）
	 */
	UFUNCTION(BlueprintCallable, Category = "Corrosion|Action")
	void AddCorrosion(float Amount);

	/**
	 * 削减/回退侵蚀度（如击杀变异精英怪、拾取净化光球回退 20%）
	 */
	UFUNCTION(BlueprintCallable, Category = "Corrosion|Action")
	void ReduceCorrosion(float Amount);

	/**
	 * 使用战术净化针
	 */
	UFUNCTION(BlueprintCallable, Category = "Corrosion|Action")
	bool UsePurificationInjector(float CleanseAmount = 20.0f);

	/** 设置当前战区侵蚀环境开关 */
	UFUNCTION(BlueprintCallable, Category = "Corrosion|Config")
	void SetInCorrosionZone(bool bInZone) { bIsInCorrosionZone = bInZone; }

	// ═══ 状态查询接口 ═══

	/** 侵蚀度百分比 [0.0, 1.0] */
	UFUNCTION(BlueprintPure, Category = "Corrosion|Query")
	float GetCorrosionPercent() const { return MaxCorrosion > 0.0f ? (CurrentCorrosion / MaxCorrosion) : 0.0f; }

	/** 是否过载（软狂暴） */
	UFUNCTION(BlueprintPure, Category = "Corrosion|Query")
	bool IsOverloaded() const { return bIsOverloaded; }

	/** 计算视效暗角与警示强度（0.0 ~ 1.0，80% 起脉冲警告） */
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

protected:
	UFUNCTION()
	void OnRep_CurrentCorrosion();

	UFUNCTION()
	void OnRep_IsOverloaded();

private:
	float OverloadDamageTimer = 0.0f;

	ALSCharacterBase* GetActiveCharacter() const;
	void ApplyOverloadDamage();
};
