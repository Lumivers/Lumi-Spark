#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "LSCharacterDataAsset.generated.h"

class ALSCharacterBase;
class ULSWeaponDataAsset;
class ULSSkillDataAsset;
class UTexture2D;

/**
 * 角色数据资产 (ULSCharacterDataAsset)
 * 定义英雄白值属性、待命 Pawn 蓝图、E/Q 技能资产与默认武器配置
 */
UCLASS(BlueprintType)
class LUMI_SPARK_API ULSCharacterDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	ULSCharacterDataAsset();

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	// ═══ 基础信息 ═══

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Identity")
	FGameplayTag CharacterID;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Identity")
	FText CharacterName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Identity")
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Identity")
	FGameplayTag ElementTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Identity")
	TSoftObjectPtr<UTexture2D> PortraitIcon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Identity", meta = (ClampMin = "1", ClampMax = "5"))
	int32 Rarity = 4;

	// ═══ 角色白值基础属性 ═══

	// 基础生命上限
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|BaseStats", meta = (ClampMin = "100.0"))
	float BaseHealth = 1000.0f;

	// 基础体力上限
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|BaseStats", meta = (ClampMin = "50.0"))
	float BaseStamina = 240.0f;

	// 基础元素大招能量上限
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|BaseStats", meta = (ClampMin = "20.0"))
	float BaseEnergy = 60.0f;

	// 基础防御力（用于伤害公式减免）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|BaseStats", meta = (ClampMin = "0.0"))
	float BaseDefense = 100.0f;

	// 基础行走速度
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|BaseStats")
	float BaseWalkSpeed = 600.0f;

	// 基础冲刺速度
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|BaseStats")
	float BaseSprintSpeed = 950.0f;

	// ═══ 蓝图类与关联配置 ═══

	// 运行时生成或待命切人使用的角色 Pawn 蓝图类
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Blueprint")
	TSubclassOf<ALSCharacterBase> CharacterClass;

	// 角色绑定的默认武器数据资产
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Equipment")
	TObjectPtr<ULSWeaponDataAsset> DefaultWeaponData;

	// 战术技能 (E) 数据资产
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Skills")
	TObjectPtr<ULSSkillDataAsset> TacticalSkillData;

	// 元素爆发大招 (Q) 数据资产
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Skills")
	TObjectPtr<ULSSkillDataAsset> BurstSkillData;
};