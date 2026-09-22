#pragma once

#include "CoreMinimal.h"
#include "Weapon/LSWeaponBase.h"
#include "LSShotgun.generated.h"

/**
 * 破片重型霰弹枪 (ALSShotgun)
 * 单次扣动扳机射出 8 颗独立锥形散布弹丸，每颗弹丸独立命中结算与弹道表现，近身全中毁天灭地
 */
UCLASS()
class LUMI_SPARK_API ALSShotgun : public ALSWeaponBase
{
	GENERATED_BODY()

public:
	ALSShotgun();

	UFUNCTION(BlueprintPure, Category = "Weapon|Shotgun")
	int32 GetPelletCount() const { return PelletCount; }

protected:
	virtual void FireOnce() override;

protected:
	// 单发弹丸数量
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Shotgun", meta = (ClampMin = "4", ClampMax = "16"))
	int32 PelletCount = 8;
};