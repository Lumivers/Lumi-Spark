#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LSDriveCoreTypes.generated.h"

/**
 * 驱动核心品质等级 (T1~T4)
 */
UENUM(BlueprintType)
enum class ELSDriveDiscRarity : uint8
{
	Standard    = 0 UMETA(DisplayName = "标准 (绿色 T1, 上限 +8)"),
	Specialized = 1 UMETA(DisplayName = "特化 (蓝色 T2, 上限 +12)"),
	Precision   = 2 UMETA(DisplayName = "精密 (金色 T3, 上限 +16)"),
	Classified  = 3 UMETA(DisplayName = "绝密 (红色 T4, 上限 +20, 局内搜刮专属)")
};

/**
 * 获取对应品质的强化等级上限
 */
FORCEINLINE int32 GetMaxLevelForRarity(ELSDriveDiscRarity Rarity)
{
	switch (Rarity)
	{
	case ELSDriveDiscRarity::Standard:    return 8;
	case ELSDriveDiscRarity::Specialized: return 12;
	case ELSDriveDiscRarity::Precision:   return 16;
	case ELSDriveDiscRarity::Classified:  return 20;
	default: return 8;
	}
}

/**
 * 驱动核心6大装备槽位定义
 */
UENUM(BlueprintType)
enum class ELSDriveDiscSlot : uint8
{
	Slot1_Foundation UMETA(DisplayName = "1号位: 生存基石 (Foundation)"),
	Slot2_Power      UMETA(DisplayName = "2号位: 输出基石 (Power)"),
	Slot3_Armor      UMETA(DisplayName = "3号位: 承伤基石 (Armor)"),
	Slot4_Critical   UMETA(DisplayName = "4号位: 致命爆发 (Critical)"),
	Slot5_Elemental  UMETA(DisplayName = "5号位: 全队特化 (Elemental)"),
	Slot6_Efficiency UMETA(DisplayName = "6号位: 战术循环 (Efficiency)")
};

/**
 * 驱动核心词条属性类型枚举
 */
UENUM(BlueprintType)
enum class ELSDriveStatType : uint8
{
	None = 0,

	// ═══ 生存与承伤类 ═══
	FlatShield              UMETA(DisplayName = "基础能量护盾 (固定值)"),
	MaxHPPercent            UMETA(DisplayName = "生命值加成 (%)"),
	DefensePercent          UMETA(DisplayName = "防御力加成 (%)"),
	ElementalResist         UMETA(DisplayName = "全元素抗性 (%)"),

	// ═══ 基础攻击与直伤输出类 ═══
	AttackPercent           UMETA(DisplayName = "攻击力加成 (%)"),
	PenetrationRate         UMETA(DisplayName = "穿透率/无视防御 (%)"),
	CritRate                UMETA(DisplayName = "暴击率 (%)"),
	CritDamage              UMETA(DisplayName = "暴击伤害 (%)"),

	// ═══ 元素反应与特化增伤类 ═══
	ElementalMastery        UMETA(DisplayName = "元素精通 (EM)"),
	AllElementalDamageBonus UMETA(DisplayName = "全元素伤害加成 (%)"),
	ReactionDamageBonus     UMETA(DisplayName = "元素反应增伤 (%)"),

	// ═══ 战术循环与手感类 ═══
	EnergyRecharge          UMETA(DisplayName = "元素充能效率 (%)"),
	ReloadSpeed             UMETA(DisplayName = "战术换弹速度 (%)"),

	// ═══ 套装专属被动预留 ═══
	ShieldStrength          UMETA(DisplayName = "护盾强效 (%)"),
	
	// ═══ 基础固定数值 ═══
	FlatAttack              UMETA(DisplayName = "固定攻击力 (小攻击)"),
	FlatHP                  UMETA(DisplayName = "固定生命值 (小生命)"),
	FlatDefense             UMETA(DisplayName = "固定防御力 (小防御)")
};

/**
 * 单条词条属性快照（包含属性类型与数值）
 */
USTRUCT(BlueprintType)
struct LUMI_SPARK_API FLSDriveDiscStat
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DriveDisc|Stat")
	ELSDriveStatType StatType = ELSDriveStatType::None;

	// 词条数值（百分比按 0.15 = 15% 存储，固定值按实数如 800.0f 存储）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DriveDisc|Stat")
	float Value = 0.0f;

	FLSDriveDiscStat() = default;
	FLSDriveDiscStat(ELSDriveStatType InType, float InValue)
		: StatType(InType), Value(InValue) {}

	bool IsValid() const { return StatType != ELSDriveStatType::None && Value > 0.0f; }
};

/**
 * 驱动核心实体数据结构（FLSDriveDisc）
 */
USTRUCT(BlueprintType)
struct LUMI_SPARK_API FLSDriveDisc
{
	GENERATED_BODY()

	// 驱动核心唯一实例 GUID（用于背包管理、强化洗词条寻址）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DriveDisc")
	FGuid DiscID;

	// 驱动核心自定义显示名称
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DriveDisc")
	FText DiscName;

	// 所属套装 GameplayTag (如 TAG_DriveSet_ThunderBurst)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DriveDisc")
	FGameplayTag SetTag;

	// 归属槽位 (1~6 号位)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DriveDisc")
	ELSDriveDiscSlot Slot = ELSDriveDiscSlot::Slot1_Foundation;

	// 品质等级
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DriveDisc")
	ELSDriveDiscRarity Rarity = ELSDriveDiscRarity::Precision;

	// 强化等级 (0 ~ 20)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DriveDisc", meta = (ClampMin = "0", ClampMax = "20"))
	int32 Level = 0;

	// 主词条（槽位专属池）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DriveDisc")
	FLSDriveDiscStat MainStat;

	// 副词条列表（最多 4 条，且不与主词条重复、彼此互斥）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DriveDisc")
	TArray<FLSDriveDiscStat> SubStats;

	bool IsValid() const { return DiscID.IsValid() && MainStat.IsValid(); }
};

/**
 * 6 槽位全队装载配置（FLSDriveCoreLoadout）
 * 全队 3 人共享同一套装配
 */
USTRUCT(BlueprintType)
struct LUMI_SPARK_API FLSDriveCoreLoadout
{
	GENERATED_BODY()

	// 1号位：护盾 (+800) / 百分比生命 (+46.6%)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DriveCore|Slot1")
	FLSDriveDisc Slot1_Foundation;

	// 2号位：百分比攻击力 (+46.6%) / 穿透率 (+20.0%)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DriveCore|Slot2")
	FLSDriveDisc Slot2_Power;

	// 3号位：百分比防御力 (+46.6%) / 元素全抗性 (+25.0%)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DriveCore|Slot3")
	FLSDriveDisc Slot3_Armor;

	// 4号位：暴击率 (+31.1%) / 暴伤 (+62.2%) / 精通 (+187) / 百分比攻击力 (+46.6%)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DriveCore|Slot4")
	FLSDriveDisc Slot4_Critical;

	// 5号位：全元素增伤 (+30.0%) / 反应增伤 (+35.0%) / 精通 (+187) / 百分比攻击力 (+46.6%)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DriveCore|Slot5")
	FLSDriveDisc Slot5_Elemental;

	// 6号位：元素充能效率 (+51.8%) / 换弹速度 (+15.0%) / 百分比攻击力 (+46.6%)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DriveCore|Slot6")
	FLSDriveDisc Slot6_Efficiency;

	// 辅助方法：通过枚举获取槽位引用
	FLSDriveDisc* GetDiscBySlot(ELSDriveDiscSlot Slot)
	{
		switch (Slot)
		{
		case ELSDriveDiscSlot::Slot1_Foundation: return &Slot1_Foundation;
		case ELSDriveDiscSlot::Slot2_Power:      return &Slot2_Power;
		case ELSDriveDiscSlot::Slot3_Armor:      return &Slot3_Armor;
		case ELSDriveDiscSlot::Slot4_Critical:   return &Slot4_Critical;
		case ELSDriveDiscSlot::Slot5_Elemental:  return &Slot5_Elemental;
		case ELSDriveDiscSlot::Slot6_Efficiency: return &Slot6_Efficiency;
		default: return nullptr;
		}
	}

	const FLSDriveDisc* GetDiscBySlot(ELSDriveDiscSlot Slot) const
	{
		switch (Slot)
		{
		case ELSDriveDiscSlot::Slot1_Foundation: return &Slot1_Foundation;
		case ELSDriveDiscSlot::Slot2_Power:      return &Slot2_Power;
		case ELSDriveDiscSlot::Slot3_Armor:      return &Slot3_Armor;
		case ELSDriveDiscSlot::Slot4_Critical:   return &Slot4_Critical;
		case ELSDriveDiscSlot::Slot5_Elemental:  return &Slot5_Elemental;
		case ELSDriveDiscSlot::Slot6_Efficiency: return &Slot6_Efficiency;
		default: return nullptr;
		}
	}

	void SetDiscBySlot(ELSDriveDiscSlot Slot, const FLSDriveDisc& Disc)
	{
		if (FLSDriveDisc* Target = GetDiscBySlot(Slot))
		{
			*Target = Disc;
		}
	}

	void ClearSlot(ELSDriveDiscSlot Slot)
	{
		if (FLSDriveDisc* Target = GetDiscBySlot(Slot))
		{
			*Target = FLSDriveDisc();
		}
	}

	TArray<FLSDriveDisc> GetAllValidDiscs() const
	{
		TArray<FLSDriveDisc> ValidDiscs;
		const ELSDriveDiscSlot Slots[] = {
			ELSDriveDiscSlot::Slot1_Foundation,
			ELSDriveDiscSlot::Slot2_Power,
			ELSDriveDiscSlot::Slot3_Armor,
			ELSDriveDiscSlot::Slot4_Critical,
			ELSDriveDiscSlot::Slot5_Elemental,
			ELSDriveDiscSlot::Slot6_Efficiency
		};
		for (ELSDriveDiscSlot S : Slots)
		{
			const FLSDriveDisc* Disc = GetDiscBySlot(S);
			if (Disc && Disc->IsValid())
			{
				ValidDiscs.Add(*Disc);
			}
		}
		return ValidDiscs;
	}
};

/**
 * 角色最终全乘区战斗属性汇总快照 (FLSCombatAttributes)
 * 输入角色白值 + 武器加成 + 驱动核心 6 槽与套装被动，输入给伤害计算器与各资源组件
 */
USTRUCT(BlueprintType)
struct LUMI_SPARK_API FLSCombatAttributes
{
	GENERATED_BODY()

	// 最终攻击力 (角色白值 + 武器白值) * (1 + 攻击%)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
	float TotalAttack = 0.0f;

	// 最终生命上限 角色基础生命 * (1 + 生命%)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
	float TotalMaxHealth = 1000.0f;

	// 最终防御力 角色基础防御 * (1 + 防御%)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
	float TotalDefense = 100.0f;

	// 最终能量护盾上限 (基础护盾 + 驱动核心固定护盾)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
	float TotalShield = 0.0f;

	// 最终暴击率 (基础 0.05 + 武器 + 驱动核心)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TotalCritRate = 0.05f;

	// 最终暴击伤害 (基础 0.50 + 武器 + 驱动核心)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes", meta = (ClampMin = "0.5"))
	float TotalCritDamage = 0.5f;

	// 最终元素精通 (基础 0 + 驱动核心)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes", meta = (ClampMin = "0.0"))
	float TotalElementalMastery = 0.0f;

	// 穿透率 / 无视防御比例 (0.0 ~ 1.0)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PenetrationRate = 0.0f;

	// 全元素伤害加成比例 (0.0 ~ ...)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
	float AllElementalDamageBonus = 0.0f;

	// 元素反应伤害加成比例 (0.0 ~ ...)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
	float ReactionDamageBonus = 0.0f;

	// 元素全抗性加成比例 (0.0 ~ 1.0)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
	float ElementalResistance = 0.0f;

	// 元素充能效率倍率 (1.0 = 100% 基础)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes", meta = (ClampMin = "1.0"))
	float EnergyRecharge = 1.0f;

	// 战术换弹提速比例 (0.0 = 基础，0.15 = 换弹速度提升 15%)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes", meta = (ClampMin = "0.0", ClampMax = "0.8"))
	float ReloadSpeedBonus = 0.0f;
	
	// 战技冷却缩减比例 (0.12 = E 技能冷却缩减 12%)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float SkillCooldownReduction = 0.0f;

	// 护盾强效比例 (0.0 = 基础，0.35 = 护盾吸收量/耐久提升 35%)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
	float ShieldStrength = 0.0f;
};
