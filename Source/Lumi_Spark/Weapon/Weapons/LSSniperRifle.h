#pragma once

#include "CoreMinimal.h"
#include "Weapon/LSWeaponBase.h"
#include "LSSniperRifle.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLSSniperChargeChanged, float, ChargeRatio);

/**
 * 重型狙击步枪 (ALSSniperRifle)
 * 开镜瞄准光学蓄力增伤 (最高 2.2x)、3.0x 弱点爆头倍率、开镜极窄 FOV、远距离零衰减
 */
UCLASS()
class LUMI_SPARK_API ALSSniperRifle : public ALSWeaponBase
{
	GENERATED_BODY()

public:
	ALSSniperRifle();

	virtual void Tick(float DeltaTime) override;

	// 获取当前蓄力进度比例 (0.0 ~ 1.0)
	UFUNCTION(BlueprintPure, Category = "Weapon|Sniper")
	float GetCurrentChargeRatio() const { return MaxChargeTime > 0.0f ? FMath::Clamp(CurrentChargeTime / MaxChargeTime, 0.0f, 1.0f) : 0.0f; }

	// 获取当前蓄力伤害放大乘率 (1.0 ~ 2.2)
	UFUNCTION(BlueprintPure, Category = "Weapon|Sniper")
	float GetCurrentChargeMultiplier() const;

	UPROPERTY(BlueprintAssignable, Category = "Weapon|Sniper|Events")
	FOnLSSniperChargeChanged OnChargeRatioChanged;

protected:
	virtual void FireOnce() override;
	virtual void ProcessHit(const FHitResult& Hit) override;

protected:
	// 满蓄力所需时长（秒）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Sniper", meta = (ClampMin = "0.5"))
	float MaxChargeTime = 1.5f;

	// 满蓄力伤害倍率 (默认 2.2x)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Sniper", meta = (ClampMin = "1.0"))
	float MaxChargeMultiplier = 2.2f;

	// 狙击镜专属极窄开镜 FOV
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Sniper")
	float SniperScopeFOV = 22.0f;

private:
	float CurrentChargeTime = 0.0f;
	float LastFiredChargeMultiplier = 1.0f;
};