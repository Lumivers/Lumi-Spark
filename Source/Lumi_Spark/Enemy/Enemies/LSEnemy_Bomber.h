#pragma once

#include "CoreMinimal.h"
#include "Enemy/LSEnemyBase.h"
#include "LSEnemy_Bomber.generated.h"

/**
 * 元素自爆兵 (ALSEnemy_Bomber)
 */
UCLASS()
class LUMI_SPARK_API ALSEnemy_Bomber : public ALSEnemyBase
{
	GENERATED_BODY()

public:
	ALSEnemy_Bomber();

	virtual void OnDeath(AActor* Killer) override;

	// 触发行走引爆
	UFUNCTION(BlueprintCallable, Category = "Enemy|Bomber")
	void Detonate();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float ExplosionDamage = 180.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float ExplosionRadius = 450.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	FGameplayTag ExplosionElementTag;

private:
	bool bHasDetonated = false;
};