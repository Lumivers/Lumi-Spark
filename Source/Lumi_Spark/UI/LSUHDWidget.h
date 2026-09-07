#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/LSTypes.h"
#include "LSUHDWidget.generated.h"

class ALSWeaponBase;
class ULSWeaponComponent;

/**
 * 战斗 HUD 界面 C++ 中枢基类
 * 监听武器弹药、切枪事件与全局命中总线，驱动蓝图 UMG 表现（动态准星、HitMarker、弹药计数）
 */
UCLASS(Abstract)
class LUMI_SPARK_API ULSHUDWidget  : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	
	//获取当前武器散布比率（0.0 - 1.0），供蓝图动态准星使用
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HUD|Crossair")
	float GetCurrentSpreadRatio() const;
	
	//获取当前手持武器
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HUD|Weapon")
	ALSWeaponBase* GetCurrentWeapon() const { return CurrentBoundWeapon; }
	
protected:
	//蓝图实现的表现层事件
	
	//弹药刷新通知(驱动文本显示
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Ammo")
	void OnAmmoUpdated(int32 CurrentAmmo, int32 MagazineSize, int32 ReserveAmmo);
	
	//击中反馈通知（驱动HitMarker动画淡入闪烁）
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|HitMarker")
	void OnHitMarkerTriggered(bool bIsHeadshot);
	
	//切枪通知（驱动武器图标刷新）
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Weapon")
	void OnWeaponSwitched(ALSWeaponBase* NewWeapon);
	
	//音效配置
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Audio")
	TObjectPtr<USoundBase> HitNormalSound;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Audio")
	TObjectPtr<USoundBase> HitHeadshotSound;
	
private:
	//缓存指针
	UPROPERTY(Transient)
	TObjectPtr<ULSWeaponComponent> CachedWeaponComp = nullptr;
	
	UPROPERTY(Transient)
	TObjectPtr<ALSWeaponBase> CurrentBoundWeapon = nullptr;
	
	//内部绑定回调
	UFUNCTION()
	void HandleAmmoChanged(int32 CurrentAmmo, int32 MagazineSize, int32 ReserveAmmo);
	
	UFUNCTION()
	void HandleWeaponChanged(ALSWeaponBase* NewWeapon);
	
	UFUNCTION()
	void HandleDamageDealt(const FLSDamageContext& DamageContext);
	
	void BindToWeapon(ALSWeaponBase* Weapon);
};