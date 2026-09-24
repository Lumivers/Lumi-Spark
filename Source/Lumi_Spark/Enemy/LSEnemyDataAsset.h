#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Core/LSTypes.h"
#include "LSEnemyDataAsset.generated.h"

class UBehaviorTree;

/**
 * 敌人全局数据资产 (ULSEnemyDataAsset)
 */
UCLASS(BlueprintType)
class LUMI_SPARK_API ULSEnemyDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("EnemyData"), GetFName());
	}

public:
	// 敌人显示名称
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	FText EnemyDisplayName;

	// 敌人专属标识 Tag (如 Enemy.Type.Melee)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	FGameplayTag EnemyIDTag;

	// ─── 基础战斗数值 ───
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "10.0"))
	float MaxHealth = 300.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0.0"))
	float Defense = 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats|Movement")
	float PatrolWalkSpeed = 200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats|Movement")
	float CombatRunSpeed = 480.0f;

	// 固有是否自带霸体 (Boss / 重盾兵开启)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	bool bDefaultSuperArmor = false;

	// ─── 七大元素抗性表 (0.0=正常, 0.2=20%减免, 1.0=完全免疫) ───
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats|Resistances")
	TMap<FGameplayTag, float> Resistances;

	// ─── 掉落物与充能微粒 ───
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drops")
	int32 DroppedParticleCount = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drops")
	FGameplayTag DroppedParticleElement;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drops")
	float EnergyPerParticle = 6.0f;

	// ─── 挂接的行为树资产 ───
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UBehaviorTree> BehaviorTree;
};