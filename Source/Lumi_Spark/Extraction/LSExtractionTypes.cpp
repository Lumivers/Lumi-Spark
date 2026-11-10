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