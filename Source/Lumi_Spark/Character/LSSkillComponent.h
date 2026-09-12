#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Core/LSTypes.h"
#include "LSSkillComponent.generated.h"

class ALSCharacterBase;
class ULSEventBus;

//委托定义
//E技能冷却计时器变更广播：(当前剩余冷却, 最大冷却)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLSSkillCooldownChanged, float, CurrentCooldown, float, MaxCooldown);

//Q技能能量槽变动
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLSEnergyChanged, float, CurrentEnergy, float, MaxEnergy);

// 技能/大招成功释放广播：(技能Tag, 施法者)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLSSkillCast, FGameplayTag, SkillTag, AActor*, Instigator);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLSBurstCast, FGameplayTag, BurstTag, AActor*, Instigator);

/**
 * 战术技能 (E) 与元素爆发大招 (Q) 中枢组件
 * 纳管角色独立冷却走表、攻击/反应触发大招充能，并支持后台静默流逝与蓝图逻辑扩展
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LUMI_SPARK_API ULSSkillComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:
	ULSSkillComponent();
	
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	//核心公共施法接口
	
	//尝试释放战术技能 (E)
	UFUNCTION(BlueprintCallable, Category = "Skill|Combat")
	virtual bool CastSkill();
	
	//尝试释放元素爆发大招 (Q)
	UFUNCTION(BlueprintCallable, Category = "Skill|Combat")
	virtual bool CastBurst();
	
	/**
	 * 主动注入大招能量（受击、打出反应、吃球时调用）
	 * @param EnergyAmount 充能点数
	 */
	UFUNCTION(BlueprintCallable, Category = "Skill|Energy")
	void AddEnergy(float EnergyAmount);
	
	//强制重置 E 技能 CD（供特定 Buff 或圣遗物机制使用）
	UFUNCTION(BlueprintCallable, Category = "Skill|Cooldown")
	void ResetSkillCooldown();
	
	//状态查询接口
	
	//当前是否满足释放战术技能 (E) 条件（CD结束、能量充足等）
	UFUNCTION(BlueprintPure, Category = "Skill")
	bool CanCastSkill() const;
	
	//当前是否满足释放元素爆发大招 (Q) 条件（能量槽满、未被禁用等）
	UFUNCTION(BlueprintPure, Category = "Skill")
	bool CanCastBurst() const;
	
	// 当前战术技能 (E) 冷却剩余时间
	UFUNCTION(BlueprintPure, Category = "Skill")
	float GetSkillCooldownRemaining() const { return SkillCooldownTimer; }
	
	// 当前 E 技能 CD 进度比率 (0.0 ~ 1.0, 0表示无CD可用)
	UFUNCTION(BlueprintPure, Category = "Skill")
	float GetSkillCooldownRatio() const;
	
	// 当前元素爆发大招 (Q) 能量槽充能比率
	UFUNCTION(BlueprintPure, Category = "Skill")
	float GetEnergyRatio() const;
	
	UFUNCTION(BlueprintPure, Category = "Skill")
	float GetCurrentEnergy() const { return CurrentEnergy; }
	
	UFUNCTION(BlueprintPure, Category = "Skill")
	float GetMaxEnergy() const { return MaxEnergy; }
	
public:
	// ─── 委托广播 ───
	UPROPERTY(BlueprintAssignable, Category = "Skill|Events")
	FOnLSSkillCooldownChanged OnSkillCooldownChanged;
	
	UPROPERTY(BlueprintAssignable, Category = "Skill|Events")
	FOnLSEnergyChanged OnEnergyChanged;
	
	UPROPERTY(BlueprintAssignable, Category = "Skill|Events")
	FOnLSSkillCast OnSkillCast;
	
	UPROPERTY(BlueprintAssignable, Category = "Skill|Events")
	FOnLSBurstCast OnBurstCast;
	
protected:
	// 扩展执行虚函数
	
	/**
	 * E 技能具体业务逻辑（如火球投掷、突进推掌、聚怪风场）
	 * 可在蓝图子类中直接连线实现特效、伤害碰撞体或召唤物！
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Skill|Execution")
	void ExecuteSkillLogic();
	virtual void ExecuteSkillLogic_Implementation();
	
	/**
	 * Q 大招具体业务逻辑（全屏天动万象、雷暴矩阵、水龙卷）
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Skill|Execution")
	void ExecuteBurstLogic();
	virtual void ExecuteBurstLogic_Implementation();
	
protected:
	// E技能配置
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Tactical")
	FGameplayTag SkillTag;
	
	// E 技能冷却时间（秒）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Tactical")
	float MaxSkillCooldown = 8.0f;
	
	// E技能冷却剩余计时器
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Skill|Tactical")
	float SkillCooldownTimer = 0.0f;
	
	// Q大招配置
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Burst")
	FGameplayTag BurstTag;
	
	// Q大招所需的能量上限
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Burst")
	float MaxEnergy = 60.0f;
	
	// Q大招当前能量值
	UPROPERTY(ReplicatedUsing = OnRep_CurrentEnergy, VisibleInstanceOnly, BlueprintReadOnly, Category = "Skill|Burst")
	float CurrentEnergy = 0.0f;
	
	// 触发一次元素反应时，奖励给玩家的大招能量点
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Burst")
	float EnergyPerReaction = 4.0f;
	
	//造成直伤转换大招能量的转化率（每造成 100 点直伤充能点数）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Burst")
	float EnergyDamageConversionRate = 0.02f;
	
	UFUNCTION()
	void OnRep_CurrentEnergy();
	
private:
	// 事件总线监听回调
	
	UFUNCTION()
	void HandleDamageDealt(const FLSDamageContext& DamageContext);
	
	UFUNCTION()
	void HandleReactionTriggered(AActor* Target, FGameplayTag InReactionTag, float ReactionDamage, AActor* Instigator);
};