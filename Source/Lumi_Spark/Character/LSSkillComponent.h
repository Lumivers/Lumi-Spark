#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Core/LSTypes.h"
#include "LSSkillComponent.generated.h"

class ALSCharacterBase;
class ULSEnergyComponent;
class ULSEventBus;

// E技能冷却计时器变更广播：(当前剩余冷却, 最大冷却)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLSSkillCooldownChanged, float, CurrentCooldown, float, MaxCooldown);

// 技能/大招成功释放广播：(技能Tag, 施法者)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLSSkillCast, FGameplayTag, SkillTag, AActor*, Instigator);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLSBurstCast, FGameplayTag, BurstTag, AActor*, Instigator);

/**
 * 战术技能 (E) 与元素爆发 (Q) 施法中枢
 * 专职管理 E 技能冷却流逝与 Q 技能施法判定，大招能量完全交由 ULSEnergyComponent 纳管
 */
UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class LUMI_SPARK_API ULSSkillComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:
	ULSSkillComponent();
	
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	// ═══ 施法接口 ═══
	UFUNCTION(BlueprintCallable, Category = "Skill|Combat")
	virtual bool CastSkill();
	
	UFUNCTION(BlueprintCallable, Category = "Skill|Combat")
	virtual bool CastBurst();
	
	// 强制重置 E 技能 CD
	UFUNCTION(BlueprintCallable, Category = "Skill|Cooldown")
	void ResetSkillCooldown();
	
	// ═══ 状态查询 ═══
	UFUNCTION(BlueprintPure, Category = "Skill")
	bool CanCastSkill() const;
	
	UFUNCTION(BlueprintPure, Category = "Skill")
	bool CanCastBurst() const;
	
	UFUNCTION(BlueprintPure, Category = "Skill")
	float GetSkillCooldownRemaining() const { return SkillCooldownTimer; }
	
	UFUNCTION(BlueprintPure, Category = "Skill")
	float GetSkillCooldownRatio() const;

	UFUNCTION(BlueprintPure, Category = "Skill")
	float GetBurstRequiredEnergy() const { return BurstRequiredEnergy; }

	UFUNCTION(BlueprintPure, Category = "Skill")
	float GetCurrentEnergy() const;

	UFUNCTION(BlueprintPure, Category = "Skill")
	float GetMaxEnergy() const;

public:
	UPROPERTY(BlueprintAssignable, Category = "Skill|Events")
	FOnLSSkillCooldownChanged OnSkillCooldownChanged;
	
	UPROPERTY(BlueprintAssignable, Category = "Skill|Events")
	FOnLSSkillCast OnSkillCast;
	
	UPROPERTY(BlueprintAssignable, Category = "Skill|Events")
	FOnLSBurstCast OnBurstCast;

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "Skill|Execution")
	void ExecuteSkillLogic();
	virtual void ExecuteSkillLogic_Implementation();
	
	UFUNCTION(BlueprintNativeEvent, Category = "Skill|Execution")
	void ExecuteBurstLogic();
	virtual void ExecuteBurstLogic_Implementation();
	
	// 获取所属角色的能量组件
	ULSEnergyComponent* GetOwnerEnergyComponent() const;

protected:
	// E 战技配置
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Tactical")
	FGameplayTag SkillTag;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Tactical")
	float MaxSkillCooldown = 8.0f;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Skill|Tactical")
	float SkillCooldownTimer = 0.0f;
	
	// Q 大招配置
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Burst")
	FGameplayTag BurstTag;
	
	// Q 大招所需能量门槛
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Burst", meta = (ClampMin = "10.0"))
	float BurstRequiredEnergy = 60.0f;
	
	// 触发元素反应奖励给玩家的能量
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Burst")
	float EnergyPerReaction = 4.0f;
	
	// 直伤转换能量比例
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Burst")
	float EnergyDamageConversionRate = 0.02f;

private:
	UFUNCTION()
	void HandleDamageDealt(const FLSDamageContext& DamageContext);
	
	UFUNCTION()
	void HandleReactionTriggered(AActor* Target, FGameplayTag InReactionTag, float ReactionDamage, AActor* Instigator);
};