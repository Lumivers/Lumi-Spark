#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Equipment/LSDriveCoreTypes.h"
#include "ULSDriveDiscDataAsset.generated.h"


//驱动核心套装加成数据结构
USTRUCT(BlueprintType)
struct LUMI_SPARK_API FLSDriveSetBonus
{
	GENERATED_BODY()

	// 触发所需件数（通常为 2 或 4）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SetBonus")
	int32 RequiredPieces = 2;

	// 效果文本描述（UI 展示用）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SetBonus")
	FText BonusDescription;

	// 被动属性数值加成列表（如 2件套精通 +80、战技CDR +12% 等）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SetBonus")
	TArray<FLSDriveDiscStat> StatBonuses;

	// 4件套机制赋予的特性 GameplayTag（用于驱动在场 Buff 判定，如 TAG_DriveSet_TacticalSwap）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SetBonus")
	FGameplayTag MechanismTag;
};


//槽位主词条生成权重项
USTRUCT(BlueprintType)
struct LUMI_SPARK_API FLSMainStatWeightEntry
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Weight")
	ELSDriveStatType StatType = ELSDriveStatType::None;

	// 生成权重（相对比值）
	UPROPERTY(EditDefaultsOnly, Category = "Weight")
	int32 Weight = 100;

	// 0 级初始基础值
	UPROPERTY(EditDefaultsOnly, Category = "Weight")
	float BaseValue = 0.0f;

	// 满级 (+20) 最终目标值
	UPROPERTY(EditDefaultsOnly, Category = "Weight")
	float MaxValue = 0.0f;
};


//驱动盘流派数据资产 (ULSDriveDiscDataAsset)
UCLASS(BlueprintType)
class LUMI_SPARK_API ULSDriveDiscDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	ULSDriveDiscDataAsset();

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	// 套装唯一标识 Tag
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	FGameplayTag SetTag;

	// 套装显示名称（如：战术连携、元素共振、精准射手）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	FText SetName;

	// 套装背景与战术描述
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	FText SetDescription;

	// 2 件套效果配置
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SetBonus")
	FLSDriveSetBonus TwoPieceBonus;

	// 4 件套效果配置
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SetBonus")
	FLSDriveSetBonus FourPieceBonus;

	// 1~6 槽位专属主词条配置表
	UPROPERTY(EditDefaultsOnly, Category = "Generation|MainStats")
	TMap<ELSDriveDiscSlot, FLSMainStatWeightEntry> MainStatPool;

	
	//辅助工具函数：根据槽位与品质，随机/生成一个规范的驱动盘实例
	UFUNCTION(BlueprintCallable, Category = "DriveDisc|Factory")
	static FLSDriveDisc GenerateDisc(ULSDriveDiscDataAsset* SetAsset, ELSDriveDiscSlot Slot, ELSDriveDiscRarity Rarity);
};