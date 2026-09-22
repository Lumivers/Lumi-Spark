#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/LSTypes.h"
#include "LSUHDWidget.generated.h"

class ALSWeaponBase;
class ULSWeaponComponent;
class ALSCharacterBase;
class ULSSkillComponent;
class ULSHealthComponent;
class ULSStaminaComponent;
class ULSEnergyComponent;

UCLASS(Abstract)
class LUMI_SPARK_API ULSHUDWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HUD|Crossair")
	float GetCurrentSpreadRatio() const;
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HUD|Weapon")
	ALSWeaponBase* GetCurrentWeapon() const { return CurrentBoundWeapon; }

	// 切人时核心重绑入口
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void BindToCharacter(ALSCharacterBase* NewCharacter);

	// ─── 暴露给 UMG 蓝图的表现层事件 ───
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Team")
	void OnActiveCharacterSwitched(ALSCharacterBase* NewCharacter, int32 SlotIndex);
	
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Interaction")
	void OnInteractPromptUpdated(bool bIsVisible, const FText& PromptText);
	
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Interaction")
	void OnInteractProgressUpdated(float ProgressRatio);
	
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Coop")
	void OnLocalPlayerDownedChanged(bool bIsDowned, float BleedoutRatio);
	
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Coop")
	void OnRaidWipedTriggered();

	// 生命变动
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Health")
	void OnHealthUpdated(float CurrentHealth, float MaxHealth);

	// 低血量告警 (<20%)
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Health")
	void OnLowHealthWarning(bool bIsLowHealth);

	// 体力变动 (当前, 最大, 比例)
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Stamina")
	void OnStaminaUpdated(float CurrentStamina, float MaxStamina, float Ratio);

	// 技能 CD
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Skill")
	void OnSkillCooldownUpdated(float CurrentCooldown, float MaxCooldown, float Ratio);

	// Q 大招能量
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Skill")
	void OnBurstEnergyUpdated(float CurrentEnergy, float MaxEnergy, float Ratio);

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Ammo")
	void OnAmmoUpdated(int32 CurrentAmmo, int32 MagazineSize, int32 ReserveAmmo);
	
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|HitMarker")
	void OnHitMarkerTriggered(bool bIsHeadshot);
	
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Weapon")
	void OnWeaponSwitched(ALSWeaponBase* NewWeapon);
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Audio")
	TObjectPtr<USoundBase> HitNormalSound;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Audio")
	TObjectPtr<USoundBase> HitHeadshotSound;

	// 组件直连回调
	UFUNCTION()
	void HandleHealthChanged(float CurrentHealth, float MaxHealth, bool bIsDamage);

	UFUNCTION()
	void HandleLowHealth(float CurrentHealth, float MaxHealth);

	UFUNCTION()
	void HandleStaminaChanged(float CurrentStamina, float MaxStamina);

	UFUNCTION()
	void HandleSkillCooldownChanged(float CurrentCooldown, float MaxCooldown);

	UFUNCTION()
	void HandleEnergyChanged(float CurrentEnergy, float MaxEnergy);

	UFUNCTION()
	void HandleGlobalRaidWiped();

private:
	void BindToWeapon(ALSWeaponBase* Weapon);
	
	UFUNCTION()
	void HandleWeaponChanged(ALSWeaponBase* NewWeapon);
	
	UFUNCTION()
	void HandleAmmoChanged(int32 CurrentAmmo, int32 MagazineSize, int32 ReserveAmmo);
	
	UFUNCTION()
	void HandleDamageDealt(const FLSDamageContext& DamageContext);

private:
	UPROPERTY(Transient)
	TObjectPtr<ALSCharacterBase> BoundCharacter = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ULSHealthComponent> BoundHealthComp = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ULSStaminaComponent> BoundStaminaComp = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ULSEnergyComponent> BoundEnergyComp = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ULSSkillComponent> BoundSkillComp = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ULSWeaponComponent> CachedWeaponComp = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ALSWeaponBase> CurrentBoundWeapon = nullptr;
};