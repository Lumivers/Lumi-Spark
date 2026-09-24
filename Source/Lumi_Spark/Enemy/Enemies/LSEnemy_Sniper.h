#pragma once

#include "CoreMinimal.h"
#include "Enemy/LSEnemyBase.h"
#include "LSEnemy_Sniper.generated.h"

/**
 * 红外狙击手 (ALSEnemy_Sniper)
 */
UCLASS()
class LUMI_SPARK_API ALSEnemy_Sniper : public ALSEnemyBase
{
	GENERATED_BODY()

public:
	ALSEnemy_Sniper();

	virtual void Tick(float DeltaTime) override;

	// 开始狙击锁定（开启红外预警线）
	UFUNCTION(BlueprintCallable, Category = "Enemy|Sniper")
	void StartAimingAt(AActor* Target);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Sniper")
	void CancelAiming();

protected:
	void FireSniperShot();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float SniperDamage = 110.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float AimDuration = 1.5f;

private:
	bool bIsAiming = false;
	FTimerHandle AimTimerHandle;
	TWeakObjectPtr<AActor> AimTarget;
};