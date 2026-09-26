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
	Precision       UMETA(DisplayName = "精密 (金)"),
	Classified      UMETA(DisplayName = "绝密 (红)")
};

// 背包扩容层级
UENUM(BlueprintType)
enum class ELSBackpackTier : uint8
{
	Default         UMETA(DisplayName = "初始战备包 (15格)"),
	StandardTier    UMETA(DisplayName = "标准扩容包 (30格, +15)"),
	SpecializedTier UMETA(DisplayName = "特化战术包 (40格, +10)"),
	PrecisionTier   UMETA(DisplayName = "精密军械包 (50格, +10)"),
	ClassifiedTier  UMETA(DisplayName = "绝密次元包 (60格, +10)")
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