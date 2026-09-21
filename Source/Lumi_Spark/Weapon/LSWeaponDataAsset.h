#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Core/LSTypes.h"
#include "LSWeaponDataAsset.generated.h"

class USkeletalMesh;
class UAnimMontage;
class USoundBase;
class UParticleSystem;
class UMaterialInterface;
class UTexture2D;

/**
 * 武器数据资产 (ULSWeaponDataAsset)
 * 纯数据配置类，供策划在编辑器中配置枪械面板白值、射速、散布模型、后坐力序列及视听表现软引用
 */
UCLASS(BlueprintType)
class LUMI_SPARK_API ULSWeaponDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	ULSWeaponDataAsset();

	// PrimaryDataAsset 资产管理器唯一标识
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	// ═══ 基础身份与元数据 ═══
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Identity")
	FGameplayTag WeaponID;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Identity")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Identity")
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Identity")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Identity", meta = (ClampMin = "1", ClampMax = "5"))
	int32 Rarity = 3;

	// ═══ 战斗与射击参数 ═══

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Combat")
	FGameplayTag WeaponTypeTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Combat")
	FGameplayTag ElementTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Combat")
	ELSElementGauge ElementGauge = ELSElementGauge::Light;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Combat")
	ELSFireMode FireMode = ELSFireMode::FullAuto;

	// 射速（RPM 每分钟射击发数）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Combat", meta = (ClampMin = "1.0"))
	float FireRate = 600.0f;

	// 单发基础直伤
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Combat", meta = (ClampMin = "0.0"))
	float BaseDamage = 32.0f;

	// 爆头弱点伤害倍率
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Combat", meta = (ClampMin = "1.0"))
	float HeadshotMultiplier = 2.0f;

	// 射线最大射程（厘米）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Combat", meta = (ClampMin = "100.0"))
	float MaxRange = 10000.0f;

	// 伤害衰减起始距离（厘米）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Combat")
	float DamageDropoffStart = 2500.0f;

	// 伤害衰减截止距离（厘米）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Combat")
	float DamageDropoffEnd = 6000.0f;

	// 远距离最低伤害保底倍率（0.1 ~ 1.0）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Combat", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float MinDamageMultiplier = 0.5f;

	// ═══ 弹药与换弹参数 ═══

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Ammo", meta = (ClampMin = "1"))
	int32 MagazineSize = 30;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Ammo", meta = (ClampMin = "0"))
	int32 MaxReserveAmmo = 180;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Ammo", meta = (ClampMin = "0.1"))
	float ReloadTime = 1.8f;

	// ═══ 准星散布控制 ═══

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Spread")
	float BaseSpread = 0.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Spread")
	float MaxSpread = 4.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Spread")
	float SpreadIncreasePerShot = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Spread")
	float SpreadRecoveryRate = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Spread")
	float ADSSpreadMultiplier = 0.35f;

	// ═══ 程序化后坐力序列 ═══

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Recoil")
	TArray<FVector2D> RecoilPattern;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Recoil")
	float RecoilMultiplier = 1.0f;

	// ═══ 视听表现软引用资产（按需流式加载） ═══

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Visuals")
	TSoftObjectPtr<USkeletalMesh> WeaponMeshAsset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Visuals")
	TSoftObjectPtr<UAnimMontage> CharacterFireMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Visuals")
	TSoftObjectPtr<UAnimMontage> CharacterReloadMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Visuals")
	TSoftObjectPtr<UAnimMontage> FPArmsFireMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Visuals")
	TSoftObjectPtr<UAnimMontage> FPArmsReloadMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Audio")
	TSoftObjectPtr<USoundBase> FireSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Audio")
	TSoftObjectPtr<USoundBase> ReloadSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Effects")
	TSoftObjectPtr<UParticleSystem> MuzzleFlashFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Effects")
	TSoftObjectPtr<UParticleSystem> ImpactFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Effects")
	TSoftObjectPtr<USoundBase> ImpactSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Effects")
	TSoftObjectPtr<UMaterialInterface> ImpactDecalMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Effects")
	TSoftObjectPtr<UParticleSystem> TracerFX;
};