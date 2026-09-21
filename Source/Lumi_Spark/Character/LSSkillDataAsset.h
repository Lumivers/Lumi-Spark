#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Core/LSTypes.h"
#include "LSSkillDataAsset.generated.h"

class UAnimMontage;
class USoundBase;
class UParticleSystem;
class UTexture2D;

/**
 * 技能释放类型分类
 */
UENUM(BlueprintType)
enum class ELSSkillType : uint8
{
	Instant     UMETA(DisplayName = "瞬发释放"),
	Projectile  UMETA(DisplayName = "元素投射物"),
	AoE         UMETA(DisplayName = "范围伤害/领域"),
	Buff        UMETA(DisplayName = "自身/全队增益"),
	Summon      UMETA(DisplayName = "实体召唤"),
	Transform   UMETA(DisplayName = "形态/强化模式")
};

/**
 * 技能数据资产 (ULSSkillDataAsset)
 * 纳管单项技能的伤害倍率、CD、能量消耗、元素附着量级与技能表现软引用
 */
UCLASS(BlueprintType)
class LUMI_SPARK_API ULSSkillDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	ULSSkillDataAsset();

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	// ═══ 技能身份 ═══

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Identity")
	FGameplayTag SkillID;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Identity")
	FText SkillName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Identity")
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Identity")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Identity")
	ELSSkillType SkillType = ELSSkillType::Instant;

	// ═══ 战斗数值机制 ═══

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Combat")
	FGameplayTag ElementTag;

	// 技能伤害倍率（基于角色基础攻击力 ATK 的百分比，如 2.5 即 250%）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Combat", meta = (ClampMin = "0.0"))
	float DamageMultiplier = 2.5f;

	// 基础冷却时间（秒，E 技能使用；Q 技能可填 0 或转场 CD）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Combat", meta = (ClampMin = "0.0"))
	float Cooldown = 8.0f;

	// Q 大招释放能量消耗（E 技能为 0，Q 大招通常为 40/60/80）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Combat", meta = (ClampMin = "0.0"))
	float EnergyCost = 0.0f;

	// 附着的元素量级（默认强元素 2U，衰减 12s）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Combat")
	ELSElementGauge ElementGauge = ELSElementGauge::Heavy;

	// 范围参数（适用于 AoE / 领域型技能，单位：厘米）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Combat", meta = (ClampMin = "0.0"))
	float AoERadius = 300.0f;

	// 增益/领域持续时间（秒）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Combat", meta = (ClampMin = "0.0"))
	float Duration = 5.0f;

	// ═══ 施法视听软引用 ═══

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Visuals")
	TSoftObjectPtr<UAnimMontage> CastMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Audio")
	TSoftObjectPtr<USoundBase> CastSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Effects")
	TSoftObjectPtr<UParticleSystem> SkillFX;
};