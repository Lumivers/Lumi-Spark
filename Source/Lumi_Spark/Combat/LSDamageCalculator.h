#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "Core/LSTypes.h"
#include "LSDamageCalculator.generated.h"

/**
* 攻击者战斗属性包
*/
USTRUCT(BlueprintType)
struct LUMI_SPARK_API FLSAttackerStats
{
    Generated_BODY()

    //攻击者等级（影响防御力减伤比与剧变反应伤害基数）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    int32 level = 90;

    //基础攻击力
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float Attack = 1000.f;

    //暴击率（0~1）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float CritRate = 0.05f;

    //暴击伤害倍率（0.5-~）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats", meta = (ClampMin = "1.0", ClampMax = "2.0"))
    float CritDamage = 0.5f;

    //元素精通（影响元素反应伤害）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats", meta = (ClampMin = "0.0"))
    float ElementalMastery = 0.f;

    //对应属性伤害加成
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float DamageBonus = 0.0f;

    //无视防御力比例（0~1）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float IgnoreDefense = 0.0f;
};

/**
 * 防御者/受击目标战斗属性包
 */
USTRUCT(BlueprintType)
struct LUMI_SPARK_API FLSDefenderStats
{
	GENERATED_BODY()

	// 防御者等级
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 Level = 90;
	
    // 基础防御力
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float Defense = 500.0f;
	
    // 当前伤害属性对应的基础抗性（默认基础抗性 10%，即 0.1）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float ElementalResistance = 0.1f;
	
    // 削减防御力比例 [0.0, 1.0]（破甲技能）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DefShredRate = 0.0f;
	
    // 削减抗性数值（如超导反应 -40% 物理抗性则填 0.4）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float ResShredRate = 0.0f;
};

/**
 * 伤害结算输出快照（方便 UI 跳字、战斗日志与回放统计）
 */
USTRUCT(BlueprintType)
struct LUMI_SPARK_API FLSDamageResult
{
    GENERATED_BODY()

    // 最终伤害数值
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Result")
    float FinalDamage = 0.0f;

    // 是否暴击
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Result")
    bool bIsCritical = false;

    // 乘区细分倍率快照
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Result")
    float CritMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Result")
    float DefMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Result")
    float ResMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Result")
    float ReactionMultiplier = 1.0f;

    //本次攻击触发的反应Tag
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Result")
    FGameplayTag TriggeredReactionTag;
};

/**
 * 战斗伤害计算引擎 (ULSDamageCalculator)
 * 纯静态无状态数学公式库，严格实现高等元素论七乘区模型
 */
UCLASS()
class LUMI_SPARK_API ULSDamageCalculator : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
	 * 核心主计算入口：计算直接攻击伤害，回填 FLSDamageContext 并输出明细快照
	 * @param InOutContext 伤害上下文（由武器/手雷构建，计算完成后将更新 FinalDamage 与 bIsCritical）
	 * @param AttackerStats 攻击者属性包
	 * @param DefenderStats 受击者属性包
	 * @param AuraElementTag 目标身上当前附着的底元素 Tag（若有）
	 */
    UFUNCTION(BlueprintCallable, Category = "Combat|Damage")
    static FLSDamageResult CalculateDamage(UPARAM(ref) FLSDamageContext& InOutContext, const FLSAttackerStats& AttackerStats, const FLSDefenderStats& DefenderStats, FGameplayTag& AuraElementTag = FGameplayTag());

    /**
	 * 1. 防御区减免系数公式 (基于等级与防御穿透)
	 * 公式：(AtkLv + 100) / [ (AtkLv + 100) + (DefLv + 100) * (1 - DefShred) * (1 - DefIgnore) ]
	 */
    UFUNCTION(BlueprintPure, Category = "Combat|ForMula")
    static float CalculateDefenseMultiplier(int32 AttackerLevel, int32 DefenderLevel, float DefShredRate, float DefIgnoreRate);

    /**
	 * 2. 抗性区减免系数公式 (三段分段函数)
	 * 净抗性 NetRes = BaseResistance - ResShred
	 * - NetRes < 0: 收益折半 1 - (NetRes / 2)
	 * - 0 <= NetRes < 0.75: 正常承伤 1 - NetRes
	 * - NetRes >= 0.75: 高抗性稀释 1 / (1 + 4 * NetRes)
	 */
    UFUNCTION(BlueprintPure, Category = "Combat|ForMula")
    static float CalculateResistanceFactor(float BaseResistance, float ResShredRate);

    /**
	 * 3. 元素精通 (EM) 对增幅反应 (蒸发/融化) 的提升百分比：2.78 * EM / (EM + 1400)
	 */
	UFUNCTION(BlueprintPure, Category = "Combat|Formula")
	static float CalculateAmplifyingReactionBonus(float ElementalMastery);

    /**
	 * 4. 元素精通 (EM) 对剧变反应 (超载/感电/超导/绽放/超绽放/扩散) 的提升百分比：16.0 * EM / (EM + 2000)
	 */
	UFUNCTION(BlueprintPure, Category = "Combat|Formula")
	static float CalculateTransformativeReactionBonus(float ElementalMastery);

    /**
	 * 5. 查询增幅反应基础倍率与反应类型（顺向 2.0x，反向 1.5x）
	 */
	UFUNCTION(BlueprintPure, Category = "Combat|Formula")
	static float GetAmplifyingReactionBaseMultiplier(const FGameplayTag& TriggerElement, const FGameplayTag& AuraElement, FGameplayTag& OutReactionTag);
    
	/**
	 * 6. 计算独立剧变反应伤害（无视目标防御力，由等级基数、倍率与抗性决定）
	 */
	UFUNCTION(BlueprintPure, Category = "Combat|Formula")
	static float CalculateTransformativeReactionDamage(
		const FGameplayTag& ReactionTag,
		int32 AttackerLevel,
		float ElementalMastery,
		float TargetResistance,
		float ResShredRate
	);

	/**
	 * 7. 角色等级基础剧变伤害插值（1级约 17，90级约 1447）
	 */
	static float GetLevelBaseTransformativeDamage(int32 Level);
};