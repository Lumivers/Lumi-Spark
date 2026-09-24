#pragma once

#include "CoreMinimal.h"
#include "Enemy/LSEnemyBase.h"
#include "LSEnemy_Ranged.generated.h"

/**
 * 远程步枪射手 (ALSEnemy_Ranged)
 */
UCLASS()
class LUMI_SPARK_API ALSEnemy_Ranged : public ALSEnemyBase
{
	GENERATED_BODY()

public:
	ALSEnemy_Ranged();

	// 触发三连发点射
	UFUNCTION(BlueprintCallable, Category = "Enemy|Ranged")
	void FireBurst(AActor* Target);

	// 距离过近判定（供行为树触发后撤）
	UFUNCTION(BlueprintPure, Category = "Enemy|Ranged")
	bool ShouldRetreatFrom(const AActor* Target) const;

protected:
	void ExecuteSingleShot();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float BulletDamage = 18.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float MinKeepDistance = 600.0f; // 6米红线，贴近必须后撤

private:
	int32 RemainingBurstShots = 0;
	FTimerHandle BurstTimerHandle;
	TWeakObjectPtr<AActor> CurrentTarget;
};