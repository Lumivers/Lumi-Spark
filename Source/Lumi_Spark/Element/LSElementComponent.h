#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Core/LSTypes.h"
#include "LSElementComponent.generated.h"

class ULSEventBus;

/**
 * 附着元素快照结构体
 */
USTRUCT(BlueprintType)
struct LUMI_SPARK_API FLSElementAura
{
	GENERATED_BODY()

	// 元素类型标签 (火/水/雷/冰/草/冻结/激化)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Element")
	FGameplayTag ElementTag;

	// 当前剩余附着量级 (U)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Element")
	float CurrentGauge = 0.0f;

	// 线性衰减速率 (U/s)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Element")
	float DecayRate = 0.1053f;

	// 初始标称量级 (1.0f, 2.0f, 4.0f)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Element")
	float InitialGauge = 1.0f;
};

// 内部 ICD 追踪记录
struct FLSICDEntry
{
	float LastApplyTime = -100.0f;
	int32 HitCount = 0;
};

// 元素附着变动与反应委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLSAuraChanged, const TArray<FGameplayTag>&, ActiveElements);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnLSElementReactionEvent, FGameplayTag, ReactionTag, float, ReactionDamage, AActor*, InstigatorActor, AActor*, TargetActor);

/**
 * 高等元素论附着与反应规则引擎中枢
 * 可挂载于角色、敌人或场景交互物上，仲裁元素衰减与 16 种反应触发
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class LUMI_SPARK_API ULSElementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSElementComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ─── 核心公共接口 ───

	/**
	 * 核心入口：对本组件拥有者施加元素（服务端权威裁决）
	 * @param InstigatorActor 攻击来源施加者
	 * @param IncomingElement 施加的元素 Tag
	 * @param Gauge 元素量级（弱1U / 强2U / 超强4U）
	 */
	UFUNCTION(BlueprintCallable, Category = "Element|Combat")
	void ApplyElement(AActor* InstigatorActor, const FGameplayTag& IncomingElement, ELSElementGauge Gauge);

	// 获取当前体表活跃元素列表（供头顶 UI 状态图标渲染）
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Element")
	TArray<FGameplayTag> GetActiveElementTags() const;

	// 查询是否存在某种特定元素附着
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Element")
	bool HasElementAura(const FGameplayTag& ElementTag) const;

	// 查询主导底元素（供伤害计算器判定增幅反应倍率）
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Element")
	FGameplayTag GetPrimaryAuraTag() const;

	// ─── 委托广播 ───
	UPROPERTY(BlueprintAssignable, Category = "Element|Events")
	FOnLSAuraChanged OnAuraChanged;

	UPROPERTY(BlueprintAssignable, Category = "Element|Events")
	FOnLSElementReactionEvent OnReactionTriggered;

protected:
	// 当前活跃附着元素池（多端网络同步）
	UPROPERTY(ReplicatedUsing = OnRep_ActiveAuras, VisibleInstanceOnly, BlueprintReadOnly, Category = "Element")
	TArray<FLSElementAura> ActiveAuras;

	UFUNCTION()
	void OnRep_ActiveAuras();

	// 是否受附着内置冷却 (ICD) 限制（玩家枪械受限，Boss技能/大招可豁免）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Element|ICD")
	bool bApplyICD = true;

private:
	// ICD 追踪映射表 (攻击者 ID -> 计数记录)
	TMap<uint32, FLSICDEntry> ICDMap;

	// 感电 / 燃烧持续判定计时器
	float ElectroChargedTimer = 0.0f;
	float BurningTimer = 0.0f;

	// ─── 内部仲裁子流程 ───

	// 检查并更新攻击者的 ICD，返回本次攻击是否能成功挂元素
	bool CheckAndUpdateICD(AActor* InstigatorActor);

	// 将枚举量级转换为浮点数值与自然衰减速率
	void ConvertGaugeData(ELSElementGauge Gauge, float& OutInitialGauge, float& OutDecayRate);

	// 仲裁触发具体反应，返回扣除后剩余的触发元素量
	float ArbitrateReaction(AActor* InstigatorActor, const FGameplayTag& IncomingElement, float IncomingGauge, FLSElementAura& TargetAura);

	// 监听全局事件总线的元素施加广播
	UFUNCTION()
	void HandleGlobalElementApplied(AActor* Target, FGameplayTag ElementTag, ELSElementGauge Gauge);

	// 刷新或追加纯净底元素（扣除 0.8x 税率）
	void AttachNewAura(const FGameplayTag& ElementTag, float InitialGauge, float DecayRate);

	// 广播附着改变
	void NotifyAuraChanged();
	
	// ─── 次生反应内部处理 ───
	// 触发感电周期跳电 DoT
	void ProcessElectroChargedTick(float DeltaTime);

	// 触发风系扩散范围传染
	void TriggerSwirlSpread(AActor* InstigatorActor, const FGameplayTag& AuraToSpread);

	//触发超载范围爆轰与物理击退
	void TriggerOverloadedExplosion(AActor* InstigatorActor);
};