#include "Extraction/LSExtractionTypes.h"

FLSInventoryItem FLSInventoryItem::CreateMaterial(FName InID, const FText& InName, int32 InQty, float InValue)
{
	FLSInventoryItem Item;
	Item.ItemID = InID;
	Item.DisplayName = InName;
	Item.Description = NSLOCTEXT("LSExtraction", "MatDesc", "地脉遗迹中回收的战术科技材料，用于在局外科技树研发终端永久点亮天赋，或维修枪械损耗。");
	Item.ItemType = ELSExtractionItemType::Material;
	Item.Rarity = ELSExtractionRarity::Standard;
	Item.Quantity = InQty;
	Item.MaxStack = 100;
	Item.UnitValue = InValue;
	return Item;
}

FLSInventoryItem FLSInventoryItem::CreateCollectible(FName InID, const FText& InName, float InValue, ELSExtractionRarity InRarity)
{
	FLSInventoryItem Item;
	Item.ItemID = InID;
	Item.DisplayName = InName;
	Item.Description = NSLOCTEXT("LSExtraction", "ColDesc", "旧文明遗留的高价值稀缺收藏品。在黑市军火商处可以兑换极其丰厚的通用货币。");
	Item.ItemType = ELSExtractionItemType::Collectible;
	Item.Rarity = InRarity;
	Item.Quantity = 1;
	Item.MaxStack = 5;
	Item.UnitValue = InValue;
	return Item;
}

FLSInventoryItem FLSInventoryItem::CreateDriveDiscItem(const FLSDriveDisc& InDisc)
{
	FLSInventoryItem Item;
	Item.ItemID = FName(*FString::Printf(TEXT("Item_Disc_%s_S%d"), *InDisc.DiscName.ToString(), static_cast<int32>(InDisc.Slot)));
	Item.DisplayName = InDisc.DiscName;
	Item.Description = NSLOCTEXT("LSExtraction", "DiscDesc", "地脉驱动盘战利品，安全带回后可装配至全队驱动核心 6 槽位，激活战术套装共鸣。");
	Item.ItemType = ELSExtractionItemType::Equipment;
	Item.Quantity = 1;
	Item.MaxStack = 1;
	Item.DriveDiscData = InDisc;

	switch (InDisc.Rarity)
	{
	case ELSDriveDiscRarity::Standard:
		Item.Rarity = ELSExtractionRarity::Standard;
		Item.UnitValue = 500.0f;
		break;
	case ELSDriveDiscRarity::Specialized:
		Item.Rarity = ELSExtractionRarity::Specialized;
		Item.UnitValue = 1500.0f;
		break;
	case ELSDriveDiscRarity::Precision:
		Item.Rarity = ELSExtractionRarity::Precision;
		Item.UnitValue = 5000.0f;
		break;
	case ELSDriveDiscRarity::Classified:
		Item.Rarity = ELSExtractionRarity::Classified;
		Item.UnitValue = 25000.0f;
		break;
	}

	return Item;
}

FLSInventoryItem FLSInventoryItem::CreateWeaponBlueprint(FName InID, const FText& InName, float InValue)
{
	FLSInventoryItem Item;
	Item.ItemID = InID;
	Item.DisplayName = InName;
	Item.Description = NSLOCTEXT("LSExtraction", "WpnDesc", "绝密武器原型设计图纸，成功撤离后可解锁军械库顶级特化武器制造。");
	Item.ItemType = ELSExtractionItemType::Weapon;
	Item.Rarity = ELSExtractionRarity::Classified;
	Item.Quantity = 1;
	Item.MaxStack = 1;
	Item.UnitValue = InValue;
	return Item;
}

FLSAntiCorrosionGearData FLSAntiCorrosionGearData::GetStandardConfig(ELSExtractionRarity InRarity)
{
	FLSAntiCorrosionGearData Data;
	Data.Rarity = InRarity;

	switch (InRarity)
	{
	case ELSExtractionRarity::Standard: // 绿
		Data.MaxDurability = 4;
		Data.CorrosionMitigation = 0.10f; // -10%
		Data.DamageMitigation = 0.10f;    // -10%
		Data.bRequiresBlueprint = false;  // 工坊直接制造
		break;

	case ELSExtractionRarity::Specialized: // 蓝
		Data.MaxDurability = 6;
		Data.CorrosionMitigation = 0.25f; // -25%
		Data.DamageMitigation = 0.20f;    // -20%
		Data.bRequiresBlueprint = false;  // 工坊直接制造
		break;

	case ELSExtractionRarity::Precision: // 紫
		Data.MaxDurability = 8;
		Data.CorrosionMitigation = 0.40f; // -40%
		Data.DamageMitigation = 0.30f;    // -30%
		Data.bRequiresBlueprint = true;   // 需图纸 (Lv.3工坊)
		break;

	case ELSExtractionRarity::Classified: // 金
	case ELSExtractionRarity::Unique:     // 红（若有）
	default:
		Data.MaxDurability = 10;
		Data.CorrosionMitigation = 0.55f; // -55%
		Data.DamageMitigation = 0.45f;    // -45%
		Data.bRequiresBlueprint = true;   // 需图纸 (幻境探索掉落)
		break;
	}
	return Data;
}

FLSXenoBladeData FLSXenoBladeData::GetStandardBladeConfig(EXenoBladeVariant InVariant, ELSExtractionRarity InRarity)
{
	FLSXenoBladeData Data;
	Data.Variant = InVariant;
	Data.Rarity = InRarity;

	// 耐久与图纸门槛
	switch (InRarity)
	{
	case ELSExtractionRarity::Standard:
		Data.MaxDurability = 4;
		Data.bRequiresBlueprint = false;
		break;
	case ELSExtractionRarity::Specialized:
		Data.MaxDurability = 6;
		Data.bRequiresBlueprint = false;
		break;
	case ELSExtractionRarity::Precision:
		Data.MaxDurability = 8;
		Data.bRequiresBlueprint = true;
		break;
	case ELSExtractionRarity::Classified:
	case ELSExtractionRarity::Unique:
	default:
		Data.MaxDurability = 10;
		Data.bRequiresBlueprint = true;
		break;
	}

	// 词条强化判定（紫/金生效）
	if (InVariant == EXenoBladeVariant::HeavySlash)
	{
		Data.ExtraDamageRatio = 0.50f;
		Data.FrontalExecutionChance = (InRarity >= ELSExtractionRarity::Classified) ? 0.75f : ((InRarity == ELSExtractionRarity::Precision) ? 0.35f : 0.0f);
	}
	else if (InVariant == EXenoBladeVariant::Parry)
	{
		Data.CleansedCorrosionRatio = 0.30f;
		Data.TeamHealPercent = (InRarity >= ELSExtractionRarity::Classified) ? 0.10f : 0.05f;
	}
	else if (InVariant == EXenoBladeVariant::PhaseDash)
	{
		Data.ShieldShredBonus = (InRarity >= ELSExtractionRarity::Classified) ? 0.50f : 0.30f;
	}

	return Data;
}