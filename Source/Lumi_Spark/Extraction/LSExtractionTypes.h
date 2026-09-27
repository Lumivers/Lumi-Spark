#pragma once

#include "CoreMinimal.h"
#include "Equipment/LSDriveCoreTypes.h"
#include "LSExtractionTypes.generated.h"

// 战利品大分类（武器 / 装备 / 材料 / 收藏品 / 消耗品）
UENUM(BlueprintType)
enum class ELSExtractionItemType : uint8
{
	Weapon      UMETA(DisplayName = "武器/武器图纸"),
	Equipment   UMETA(DisplayName = "装备(驱动盘)"),
	Material    UMETA(DisplayName = "材料/科技废料"),
	Collectible UMETA(DisplayName = "高价值变现收藏品"),
	Consumable  UMETA(DisplayName = "战术消耗品")
};

// 物品品质
UENUM(BlueprintType)
enum class ELSExtractionRarity : uint8
{
	Standard        UMETA(DisplayName = "标准 (绿)"),
	Specialized     UMETA(DisplayName = "特化 (蓝)"),
	Precision       UMETA(DisplayName = "精密 (紫)"),
	Classified      UMETA(DisplayName = "机密 (金)"),
	Unique			UMETA(DisplayName = "绝密 (红)"),
};

// 背包扩容层级
UENUM(BlueprintType)
enum class ELSBackpackTier : uint8
{
	Default         UMETA(DisplayName = "初始战备包 (15格)"),
	StandardTier    UMETA(DisplayName = "标准扩容包 (20格, +5)"),
	SpecializedTier UMETA(DisplayName = "特化战术包 (30格, +10)"),
	PrecisionTier   UMETA(DisplayName = "精密军械包 (40格, +10)"),
	ClassifiedTier  UMETA(DisplayName = "机密次元包 (50格, +10)")
};

// 战区侵蚀危险与战斗强度分级
UENUM(BlueprintType)
enum class ELSHazardDifficulty : uint8
{
	Low     UMETA(DisplayName = "简单 (外围低危, -1耐久)"),
	Medium  UMETA(DisplayName = "中等 (常规探索, -1耐久)"),
	Hard    UMETA(DisplayName = "困难 (核心裂隙, -2耐久)"),
	Extreme UMETA(DisplayName = "高危 (终极巢穴/Boss, -3耐久)")
};

// 战利品背包单格物品实例
USTRUCT(BlueprintType)
struct LUMI_SPARK_API FLSInventoryItem
{
	GENERATED_BODY()

	// 物品全局唯一 GUID
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FGuid ItemUID;

	// 物品全局字典 ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FName ItemID = NAME_None;

	// 物品显示名称
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FText DisplayName;

	// 物品描述
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FText Description;

	// 物品大类
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	ELSExtractionItemType ItemType = ELSExtractionItemType::Material;

	// 物品品质
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	ELSExtractionRarity Rarity = ELSExtractionRarity::Standard;

	// 当前堆叠数量
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 Quantity = 1;

	// 最大堆叠上限
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 MaxStack = 100;

	// 商店基础单价
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	float UnitValue = 100.0f;

	// 若该物品是驱动盘装备，存储具体的驱动盘数值结构
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FLSDriveDisc DriveDiscData;

	// 是否处于安全箱保护中
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
	bool bIsSecured = false;
	
	// 当前耐久点数
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Durability")
	int32 CurrentDurability = 4;

	// 耐久上限 (绿4/蓝6/紫8/金10/红12)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Durability")
	int32 MaxDurability = 4;

	// 是否为制造图纸
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Crafting")
	bool bIsBlueprint = false;

	// 图纸对应的产物 ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Crafting")
	FName TargetCraftItemID = NAME_None;

	FLSInventoryItem() : ItemUID(FGuid::NewGuid())
	{}

	// 槽位是否有效
	bool IsValid() const
	{
		return ItemID != NAME_None && Quantity > 0;
	}

	// 预设工厂函数
	static FLSInventoryItem CreateMaterial(FName InID, const FText& InName, int32 InQty, float InValue);
	static FLSInventoryItem CreateCollectible(FName InID, const FText& InName, float InValue, ELSExtractionRarity InRarity);
	static FLSInventoryItem CreateDriveDiscItem(const FLSDriveDisc& InDisc);
	static FLSInventoryItem CreateWeaponBlueprint(FName InID, const FText& InName, float InValue);
};

// 制造所需材料单项
USTRUCT(BlueprintType)
struct LUMI_SPARK_API FLSItemIngredient
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	FName MaterialID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	int32 Count = 1;
};

// 制造工厂配方
USTRUCT(BlueprintType)
struct LUMI_SPARK_API FLSCraftingRecipe
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	FName RecipeID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	FText DisplayName;

	// 产出的物品原型
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	FLSInventoryItem ResultItem;

	// 是否需要解锁图纸
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	bool bRequiresBlueprint = false;

	// 对应必须解锁的图纸 ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	FName RequiredBlueprintID = NAME_None;

	// 所需原料清单
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	TArray<FLSItemIngredient> Ingredients;
};

// 异体刃特化分支
UENUM(BlueprintType)
enum class EXenoBladeVariant : uint8
{
	Parry      UMETA(DisplayName = "格挡型 (处决回血+减全队侵蚀)"),
	HeavySlash UMETA(DisplayName = "强斩型 (正面概率处决+增伤)"),
	PhaseDash  UMETA(DisplayName = "突进型 (超强破恶嗅盾+击杀Boss回次数)")
};

// 抗蚀器配置数据结构
USTRUCT(BlueprintType)
struct LUMI_SPARK_API FLSAntiCorrosionGearData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AntiCorrosion")
	ELSExtractionRarity Rarity = ELSExtractionRarity::Standard;

	// 耐久点数（绿4 / 蓝6 / 紫8 / 金10）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AntiCorrosion")
	int32 MaxDurability = 4;

	// 侵蚀积累降低比例（绿10% / 蓝25% / 紫40% / 金55%）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AntiCorrosion")
	float CorrosionMitigation = 0.10f;

	// 队伍承伤降低比例（绿10% / 蓝20% / 紫30% / 金45%）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AntiCorrosion")
	float DamageMitigation = 0.10f;

	// 是否需要图纸（绿/蓝为 false，紫/金为 true）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AntiCorrosion")
	bool bRequiresBlueprint = false;

	// 查表工厂函数：直接生成 4 档标准配置
	static FLSAntiCorrosionGearData GetStandardConfig(ELSExtractionRarity InRarity);
};

// 异体刃配置数据结构
USTRUCT(BlueprintType)
struct LUMI_SPARK_API FLSXenoBladeData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XenoBlade")
	EXenoBladeVariant Variant = EXenoBladeVariant::HeavySlash;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XenoBlade")
	ELSExtractionRarity Rarity = ELSExtractionRarity::Standard;

	// 耐久度（绿4 / 蓝6 / 紫8 / 金10）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XenoBlade")
	int32 MaxDurability = 4;

	// 强斩型：非背后正面处决概率（绿/蓝0%，紫35%，金75%）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XenoBlade")
	float FrontalExecutionChance = 0.0f;

	// 强斩型：处决额外增伤比例（50%）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XenoBlade")
	float ExtraDamageRatio = 0.0f;

	// 格挡型：处决命中降低侵蚀度比例（普通10% / 精英30% / 首领60%）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XenoBlade")
	float CleansedCorrosionRatio = 0.0f;

	// 格挡型：处决命中全队生命回复比例（紫2.5%~5%，金5%~10%）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XenoBlade")
	float TeamHealPercent = 0.0f;

	// 突进型：额外破盾倍率（普通100% / 精英50% / 首领30%）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XenoBlade")
	float ShieldShredBonus = 0.0f;

	// 是否需要图纸（绿/蓝为 false，紫/金为 true）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XenoBlade")
	bool bRequiresBlueprint = false;

	static FLSXenoBladeData GetStandardBladeConfig(EXenoBladeVariant InVariant, ELSExtractionRarity InRarity);
};

// 战局结算状态
UENUM(BlueprintType)
enum class ELSExtractionStatus : uint8
{
	Extracted       UMETA(DisplayName = "成功撤离"),
	KilledInAction  UMETA(DisplayName = "阵亡 (KIA)"),
	MissingInAction UMETA(DisplayName = "迷失/超时 (MIA)")
};

// 单次出击战报报告
USTRUCT(BlueprintType)
struct LUMI_SPARK_API FLSRaidReport
{
	GENERATED_BODY()

	// 结算状态
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RaidReport")
	ELSExtractionStatus Status = ELSExtractionStatus::Extracted;

	// 战区危险度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RaidReport")
	ELSHazardDifficulty HazardLevel = ELSHazardDifficulty::Medium;

	// 战局总耗时（秒）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RaidReport")
	float RaidDurationSeconds = 0.0f;

	// 成功带出的物资总估值（金币）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RaidReport")
	float TotalExtractedValue = 0.0f;

	// 成功带出的物品总格数
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RaidReport")
	int32 ExtractedItemCount = 0;

	// 本次撤离扣除的面罩耐久点数（1/1/2/3）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RaidReport")
	int32 DurabilityDeducted = 1;

	// 结算后剩余的面罩耐久点数
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RaidReport")
	int32 RemainingMaskDurability = 0;

	// 成功带出的全部物品清单（普通背包 + 安全箱）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RaidReport")
	TArray<FLSInventoryItem> ExtractedLoot;
};