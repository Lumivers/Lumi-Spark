#pragma once

#include "CoreMinimal.h"
#include "Weapon/LSWeaponBase.h"
#include "LSLauncher.generated.h"

class ALSLauncherProjectile;

/**
 * 榴弹发射器 (ALSLauncher)
 * 物理弹道抛物线投射武器，权威生成带有引信的榴弹实体，支持范围轰炸与大面积铺场
 */
UCLASS()
class LUMI_SPARK_API ALSLauncher : public ALSWeaponBase
{
	GENERATED_BODY()

public:
	ALSLauncher();

protected:
	virtual void FireOnce() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Launcher")
	TSubclassOf<ALSLauncherProjectile> ProjectileClass;
};