#pragma once

#include "CoreMinimal.h"
#include "Weapon/LSWeaponBase.h"
#include "LSRifle.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLSRifleFireModeChanged, ELSFireMode, NewFireMode);

/**
 * 突击步枪 (ALSRifle)
 * 全能中距离战术步枪，支持按 [B] 键在全自动 (FullAuto) 与半自动 (SemiAuto) 模式间自由切换
 */
UCLASS()
class LUMI_SPARK_API ALSRifle : public ALSWeaponBase
{
	GENERATED_BODY()

public:
	ALSRifle();

	// 切换射击模式（全自动 <-> 半自动）
	UFUNCTION(BlueprintCallable, Category = "Weapon|Rifle")
	void ToggleFireMode();

	// 模式切换广播
	UPROPERTY(BlueprintAssignable, Category = "Weapon|Rifle|Events")
	FOnLSRifleFireModeChanged OnFireModeSwitched;

protected:
	virtual void StartFire() override;
};