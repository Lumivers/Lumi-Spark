#pragma once

#include "CoreMinimal.h"
#include "Enemy/LSEnemyBase.h"
#include "LSEnemy_Shielder.generated.h"

/**
 * 防弹盾兵 (ALSEnemy_Shielder)
 */
UCLASS()
class LUMI_SPARK_API ALSEnemy_Shielder : public ALSEnemyBase
{
	GENERATED_BODY()

public:
	ALSEnemy_Shielder();

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

protected:
	// 正面格挡点积阈值（0.3 约对应正面 145 度扇区）
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Shield")
	float BlockDotThreshold = 0.3f;
};