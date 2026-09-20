#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Core/LSTypes.h"
#include "LSShieldComponent.generated.h"

/**
 * 单层护盾配置与运行时快照
 */
USTRUCT(BlueprintType)
struct LUMI_SPARK_API FLSShieldLayer
{
	GENERATED_BODY()

	/** 护盾元素类型（如 Element.Cryo 冰甲、Element.Electro 雷核、Element.Geo 岩盾） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shield")
	FGameplayTag ShieldElement;

	/** 当前剩余护盾量（U值或吸收点数） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shield")
	float CurrentShield = 2000.0f;

	/** 护盾上限值 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shield")
	float MaxShield = 2000.0f;

	/** 伤害吸收比例（默认 1.0f 即 100% 免伤全由护盾承担；0.8f 表示 20% 穿透直伤打入本体） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shield")
	float AbsorptionRatio = 1.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnLSShieldDamaged, FGameplayTag, ShieldElement, float, CurrentShield, float, MaxShield);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLSShieldLayerBroken, FGameplayTag, BrokenElement);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLSAllShieldsDepleted);

/**
 * 复合多层元素破盾组件 (Compound Multi-Layer Shield Component)
 * 挂载于精英怪、Boss 或训练靶上，负责元素克制判定、伤害吸收与破盾硬直虚弱
 */
UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class LUMI_SPARK_API ULSShieldComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSShieldComponent();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * 吸收伤害并根据元素克制执行破盾消耗（服务端权威裁决）
	 * @param InDamage 传入的基础武器/技能伤害
	 * @param DamageElement 攻击方所附带的元素 Tag（火/水/草/雷等）
	 * @param DamageCauser 攻击来源 Actor
	 * @return 护盾完全抵消后剩余的、需由本体肉身承受的溢出穿透伤害
	 */
	UFUNCTION(BlueprintCallable, Category = "Shield")
	float AbsorbDamage(float InDamage, const FGameplayTag& DamageElement, AActor* DamageCauser);

	/** 是否拥有任意层处于激活生效状态的护盾 */
	UFUNCTION(BlueprintPure, Category = "Shield")
	bool HasActiveShield() const;

	/** 获取当前最外层生效护盾的元素标签 */
	UFUNCTION(BlueprintPure, Category = "Shield")
	FGameplayTag GetActiveShieldElement() const;

	/** 获取当前生效护盾的剩余百分比 (0.0 ~ 1.0) */
	UFUNCTION(BlueprintPure, Category = "Shield")
	float GetActiveShieldRatio() const;

	/** 手动追加一层新护盾（如 Boss 二阶段转阶段套盾） */
	UFUNCTION(BlueprintCallable, Category = "Shield")
	void AddShieldLayer(const FGameplayTag& ShieldElement, float ShieldAmount, float AbsorptionRatio = 1.0f);

	/** 重置回满所有护盾 */
	UFUNCTION(BlueprintCallable, Category = "Shield")
	void RestoreAllShields();

public:
	// ─── 委托事件 ───
	UPROPERTY(BlueprintAssignable, Category = "Shield|Events")
	FOnLSShieldDamaged OnShieldDamaged;

	UPROPERTY(BlueprintAssignable, Category = "Shield|Events")
	FOnLSShieldLayerBroken OnShieldLayerBroken;

	UPROPERTY(BlueprintAssignable, Category = "Shield|Events")
	FOnLSAllShieldsDepleted OnAllShieldsDepleted;

protected:
	/** 复合多层护盾队列（由外至内排列，多端网络同步） */
	UPROPERTY(ReplicatedUsing = OnRep_ShieldLayers, EditAnywhere, BlueprintReadWrite, Category = "Shield")
	TArray<FLSShieldLayer> ShieldLayers;

	/** 全层护盾被击破后的虚弱瘫痪时间（秒） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shield|Config")
	float ShieldBreakStunDuration = 4.0f;

	UFUNCTION()
	void OnRep_ShieldLayers();

private:
	/** 计算攻击元素对目标护盾的克制倍率（火打冰 2.0x, 草打雷 2.0x 等） */
	float CalculateElementalBreakMultiplier(const FGameplayTag& AttackElement, const FGameplayTag& ShieldElement) const;

	/** 触发破盾瘫痪虚弱 */
	void ApplyShieldBreakStun();
	void RecoverFromStun();

	FTimerHandle StunRecoveryTimerHandle;
};