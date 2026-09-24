#pragma once

#include "CoreMinimal.h"
#include "Enemy/LSEnemyBase.h"
#include "LSEnemy_Elite.generated.h"

/**
 * 元素精英怪 (ALSEnemy_Elite)
 */
UCLASS()
class LUMI_SPARK_API ALSEnemy_Elite : public ALSEnemyBase
{
	GENERATED_BODY()

public:
	ALSEnemy_Elite();

	virtual void BeginPlay() override;

	// 释放范围战技
	UFUNCTION(BlueprintCallable, Category = "Enemy|Elite")
	void CastElementalSkill(AActor* Target);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Skill")
	float SkillDamage = 85.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Skill")
	FGameplayTag SkillElementTag;
};