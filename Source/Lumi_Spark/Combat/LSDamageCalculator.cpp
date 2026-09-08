#include "Combat/LSDamageCalculator.h"
#include "Core/LSTypes.h"

FLSDamageResult ULSDamageCalculator::CalculateDamage(
	FLSDamageContext& InOutContext,
	const FLSAttackerStats& AttackerStats,
	const FLSDefenderStats& DefenderStats,
	FGameplayTag AuraElementTag)
{
	FLSDamageResult Result;

	// 1. 基础伤害区（若已由枪械/手雷计算了距离衰减，则以 InOutContext.FinalDamage 为基底，否则使用 BaseDamage）
	const float StartingDamage = (InOutContext.FinalDamage > 0.0f) ? InOutContext.FinalDamage : InOutContext.BaseDamage;
	
	// 2. 增伤区 (1 + DamageBonus)
	const float DamageBonusFactor = FMath::Max(0.0f, 1.0f + AttackerStats.DamageBonus);

	// 3. 暴击区 (Crit)
	// 若射击已判定爆头 (bIsHeadshot)，则直接强制暴击；否则按暴击率概率掷骰
	bool bCritSuccess = InOutContext.bIsHeadshot;
	if (!bCritSuccess && AttackerStats.CritRate > 0.0f)
	{
		bCritSuccess = (FMath::FRand() < AttackerStats.CritRate);
	}

	Result.bIsCritical = bCritSuccess;
	Result.CritMultiplier = bCritSuccess ? (1.0f + AttackerStats.CritDamage) : 1.0f;

	// 4. 增幅反应区 (Amplifying Reactions: 蒸发 / 融化)
	Result.ReactionMultiplier = 1.0f;
	if (InOutContext.ElementTag.IsValid() && AuraElementTag.IsValid())
	{
		FGameplayTag ReactionTag;
		const float BaseAmpMult = GetAmplifyingReactionBaseMultiplier(InOutContext.ElementTag, AuraElementTag, ReactionTag);
		
		if (ReactionTag.IsValid() && BaseAmpMult > 1.0f)
		{
			Result.TriggeredReactionTag = ReactionTag;
			const float EMBonus = CalculateAmplifyingReactionBonus(AttackerStats.ElementalMastery);
			Result.ReactionMultiplier = BaseAmpMult * (1.0f + EMBonus);
			InOutContext.bIsReactionDamage = true;
		}
	}

	// 5. 防御力区 (Defense Factor)
	Result.DefMultiplier = CalculateDefenseFactor(
		AttackerStats.Level,
		DefenderStats.Level,
		DefenderStats.DefShredRate,
		AttackerStats.DefIgnoreRate
	);

	// 6. 抗性区 (Resistance Factor)
	Result.ResMultiplier = CalculateResistanceFactor(
		DefenderStats.ElementalResistance,
		DefenderStats.ResShredRate
	);

	// 7. 全乘区汇总计算最终实际伤害
	Result.FinalDamage = StartingDamage 
		* DamageBonusFactor 
		* Result.CritMultiplier 
		* Result.ReactionMultiplier 
		* Result.DefMultiplier 
		* Result.ResMultiplier;

	// 向上取整/保留两位精度防负数
	Result.FinalDamage = FMath::Max(0.0f, Result.FinalDamage);

	// 回填伤害上下文供全局事件总线广播
	InOutContext.FinalDamage = Result.FinalDamage;
	InOutContext.bIsCritical = Result.bIsCritical;

	return Result;
}

float ULSDamageCalculator::CalculateDefenseFactor(int32 AttackerLevel, int32 DefenderLevel, float DefShredRate, float DefIgnoreRate)
{
	const float AttackerCoeff = static_cast<float>(FMath::Max(1, AttackerLevel)) + 100.0f;
	
	// 减防与无视防御上限锁定在 90%，防止分母为 0 导致溢出
	const float ClampedDefShred = FMath::Clamp(DefShredRate, 0.0f, 0.90f);
	const float ClampedDefIgnore = FMath::Clamp(DefIgnoreRate, 0.0f, 1.0f);
	const float DefMultiplier = (1.0f - ClampedDefShred) * (1.0f - ClampedDefIgnore);

	const float DefenderCoeff = (static_cast<float>(FMath::Max(1, DefenderLevel)) + 100.0f) * DefMultiplier;

	return AttackerCoeff / (AttackerCoeff + DefenderCoeff);
}

float ULSDamageCalculator::CalculateResistanceFactor(float BaseResistance, float ResShredRate)
{
	const float NetRes = BaseResistance - ResShredRate;

	if (NetRes < 0.0f)
	{
		// 净抗性为负时：增益收益折半（例如超导 -40% 抗性，打 10% 基础抗性怪净为 -30%，获得 1 + 15% = 1.15x 伤害）
		return 1.0f - (NetRes * 0.5f);
	}
	else if (NetRes < 0.75f)
	{
		// 净抗性在 [0.0, 0.75) 区间：线性承伤
		return 1.0f - NetRes;
	}
	else
	{
		// 极高抗性区间 (>= 75%)：边际稀释抑制
		return 1.0f / (1.0f + 4.0f * NetRes);
	}
}

float ULSDamageCalculator::CalculateAmplifyingReactionBonus(float ElementalMastery)
{
	if (ElementalMastery <= 0.0f) return 0.0f;
	return (2.78f * ElementalMastery) / (ElementalMastery + 1400.0f);
}

float ULSDamageCalculator::CalculateTransformativeReactionBonus(float ElementalMastery)
{
	if (ElementalMastery <= 0.0f) return 0.0f;
	return (16.0f * ElementalMastery) / (ElementalMastery + 2000.0f);
}

float ULSDamageCalculator::GetAmplifyingReactionBaseMultiplier(const FGameplayTag& TriggerElement, const FGameplayTag& AuraElement, FGameplayTag& OutReactionTag)
{
	OutReactionTag = FGameplayTag();

	// 1. 蒸发反应 (水 + 火)
	if (TriggerElement == LSTags::TAG_Element_Hydro && AuraElement == LSTags::TAG_Element_Pyro)
	{
		OutReactionTag = LSTags::TAG_Reaction_Vaporize;
		return 2.0f; // 水打火：顺向蒸发 2.0x
	}
	if (TriggerElement == LSTags::TAG_Element_Pyro && AuraElement == LSTags::TAG_Element_Hydro)
	{
		OutReactionTag = LSTags::TAG_Reaction_Vaporize;
		return 1.5f; // 火打水：反向蒸发 1.5x
	}

	// 2. 融化反应 (火 + 冰)
	if (TriggerElement == LSTags::TAG_Element_Pyro && AuraElement == LSTags::TAG_Element_Cryo)
	{
		OutReactionTag = LSTags::TAG_Reaction_Melt;
		return 2.0f; // 火打冰：顺向融化 2.0x
	}
	if (TriggerElement == LSTags::TAG_Element_Cryo && AuraElement == LSTags::TAG_Element_Pyro)
	{
		OutReactionTag = LSTags::TAG_Reaction_Melt;
		return 1.5f; // 冰打火：反向融化 1.5x
	}

	return 1.0f;
}

float ULSDamageCalculator::CalculateTransformativeReactionDamage(
	const FGameplayTag& ReactionTag,
	int32 AttackerLevel,
	float ElementalMastery,
	float TargetResistance,
	float ResShredRate)
{
	// 1. 基础等级基数
	const float BaseDamage = GetLevelBaseTransformativeDamage(AttackerLevel);

	// 2. 剧变反应类型系数
	float ReactionCoeff = 1.0f;
	if (ReactionTag == LSTags::TAG_Reaction_Superconduct)      ReactionCoeff = 0.5f;  // 超导 (冰+雷)
	else if (ReactionTag == LSTags::TAG_Reaction_Swirl)        ReactionCoeff = 0.6f;  // 扩散 (风+元素)
	else if (ReactionTag == LSTags::TAG_Reaction_ElectroCharged)ReactionCoeff = 1.2f; // 感电 (水+雷)
	else if (ReactionTag == LSTags::TAG_Reaction_Shatter)      ReactionCoeff = 1.5f;  // 碎冰
	else if (ReactionTag == LSTags::TAG_Reaction_Overload)     ReactionCoeff = 2.0f;  // 超载 (火+雷)
	else if (ReactionTag == LSTags::TAG_Reaction_Bloom)        ReactionCoeff = 2.0f;  // 原绽放
	else if (ReactionTag == LSTags::TAG_Reaction_Burgeon)      ReactionCoeff = 3.0f;  // 烈绽放 (火引爆种子)
	else if (ReactionTag == LSTags::TAG_Reaction_Hyperbloom)   ReactionCoeff = 3.0f;  // 超绽放 (雷引爆种子)

	// 3. 元素精通提升系数
	const float EMBonus = CalculateTransformativeReactionBonus(ElementalMastery);

	// 4. 目标抗性区减免 (注意：剧变反应完全无视目标防御力 Def !)
	const float ResFactor = CalculateResistanceFactor(TargetResistance, ResShredRate);

	return BaseDamage * ReactionCoeff * (1.0f + EMBonus) * ResFactor;
}

float ULSDamageCalculator::GetLevelBaseTransformativeDamage(int32 Level)
{
	const float ClampedLevel = FMath::Clamp(static_cast<float>(Level), 1.0f, 90.0f);
	// 拟合曲线：1级基准 17.0f，90级基准 1446.85f (指数增长曲线)
	const float Alpha = (ClampedLevel - 1.0f) / 89.0f;
	return 17.0f + (1446.85f - 17.0f) * FMath::Pow(Alpha, 1.85f);
}