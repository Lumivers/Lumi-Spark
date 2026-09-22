#pragma once

#include "CoreMinimal.h"
#include "Weapon/LSWeaponBase.h"
#include "LSSMG.generated.h"

/**
 * 微型冲锋枪 (ALSSMG)
 * 极高射速 (900 RPM)、40 发大弹匣、近距高爆发、边滑铲边射击精度惩罚极小、远距断崖式衰减
 */
UCLASS()
class LUMI_SPARK_API ALSSMG : public ALSWeaponBase
{
	GENERATED_BODY()

public:
	ALSSMG();

protected:
	// 重写距离衰减：近距离极高保真，超过 20 米断崖式下跌
	virtual float CalculateDamageDropoff(float Distance) const override;
};