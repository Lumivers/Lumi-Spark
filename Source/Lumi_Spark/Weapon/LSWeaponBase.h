#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Core/LSTypes.h"
#include "Engine/NetSerialization.h"
#include "LSWeaponBase.generated.h"

class USkeletalMeshComponent;
class ULSRecoilComponent;

//射击模式枚举
UENUM(Blueprintable)
enum class ELSFireMode : uint8
{
	SemiAuto UMETA(DisplayName = "半自动（单发）"),
	FullAuto UMETA(DisplayName = "全自动（连发）"),
	Burst UMETA(DisplayName = "三连发（点射）")
};

//武器通用委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnLSAmmoChanged, int32, CurrentAmmo, int32, MagazineSize, int32, ReserveAmmo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLSReloadStart);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLSReloadEnd);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLSWeaponFired);

/**
 * 武器 Actor 基类
 * 挂载后坐力组件与武器网格体，处理即时射线射击、散布恢复与弹药逻辑
 */
UCLASS(Abstract)
class LUMI_SPARK_API ALSWeaponBase : public AActor
{
	GENERATED_BODY()
	
public:
	ALSWeaponBase();
	
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	//核心交互接口
	
	//开始射击
	UFUNCTION(BlueprintCallable, Category = "Weapon|Combat")
	virtual void StartFire();
	
	//停止射击
	UFUNCTION(BlueprintCallable, Category = "Weapon|Combat")
	virtual void StopFire();
	
	//触发换弹
	UFUNCTION(Blueprintable, Category = "Weapon|Ammo")
	virtual void Reload();
	
	//能否射击判定
	UFUNCTION(Blueprintable, Category = "Weapon|Combat")
	virtual bool CanFire() const;
	
	//能否换弹判定
	UFUNCTION(Blueprintable, Category = "Weapon|Ammo")
	virtual bool CanReload() const;
	
	//组件获取
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	FORCEINLINE ULSRecoilComponent* GetRecoilComponent() const { return RecoilComponent; }
	FORCEINLINE int32 GetCurrentAmmo() const { return CurrentAmmo; }
	FORCEINLINE int32 GetMagazineSize() const { return MagazineSize; }
	FORCEINLINE int32 GetCurrentReserveAmmo() const { return CurrentReserveAmmo; }
	FORCEINLINE FGameplayTag GetElementTag() const { return ElementTag; }
	FORCEINLINE float GetCurrentSpread() const { return CurrentSpread; }
	FORCEINLINE float GetBaseSpread() const { return BaseSpread; }
	FORCEINLINE float GetMaxSpread() const { return MaxSpread; }
	
	FORCEINLINE float GetSpreadRatio() const 
	{ 
		return (MaxSpread > BaseSpread) ? FMath::Clamp((CurrentSpread - BaseSpread) / (MaxSpread - BaseSpread), 0.0f, 1.0f) : 0.0f; 
	}
	
	//委托广播
	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FOnLSAmmoChanged OnAmmoChanged;
	
	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FOnLSReloadStart OnReloadStart;
	
	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FOnLSReloadEnd OnReloadEnd;
	
	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FOnLSWeaponFired OnWeaponFired;
	
protected:
	virtual void BeginPlay() override;
	
	//组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Components")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Components")
	TObjectPtr<ULSRecoilComponent> RecoilComponent;
	
	//动画
	
	//角色全身开火动作蒙太奇
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Animation")
	TObjectPtr<UAnimMontage> CharacterFireMontage;
	
	//第一人称手臂开火动作蒙太奇
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Animation")
	TObjectPtr<UAnimMontage> FPArmsFireMontage;
	
	//角色全身换弹动作蒙太奇
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Animation")
	TObjectPtr<UAnimMontage> CharacterReloadMontage;
	
	//第一人称手臂换弹动作蒙太奇
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Animation")
	TObjectPtr<UAnimMontage> FPArmsReloadMontage;
	
	//音效与开火表现
	
	//开火音效
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Effects")
	TObjectPtr<USoundBase> FireSound;
	
	//换弹音效
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Effects")
	TObjectPtr<USoundBase> ReloadSound;
	
	//枪口火光粒子
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Effects")
	TObjectPtr<UParticleSystem> MuzzleFlashEmitter;
	
	//武器基础身份
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Identity")
	FText WeaponDisplayName = FText::FromString(TEXT("基础步枪"));
	
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Identity")
	FGameplayTag WeaponTypeTag;
	
	//武器自带元素属性
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Identity")
	FGameplayTag ElementTag;
	
	//每次射击附着的元素量级（默认1U）
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Identity")
	ELSElementGauge ElementGauge = ELSElementGauge::Light;
	
	//射击模式与手感
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Fire")
	ELSFireMode FireMode = ELSFireMode::FullAuto;
	
	//射速：每分钟射击次数（RPM）
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Fire", meta = (ClampMin = "1.0"))
	float FireRate = 600.0f;
	
	//基础单发伤害
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Damage", meta = (ClampMin = "1.0"))
	float BaseDamage = 32.0f;
	
	//爆头伤害倍率
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Damage", meta = (ClampMin = "1.0"))
	float HeadshotMultiplier = 2.0f;
	
	//有效射程（单位：厘米，默认100m）
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Damage", meta = (ClampMin = "100.0"))
	float MaxRange = 10000.0f;
	
	//伤害衰减起始距离
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Damage")
	float DamageDropoffStart = 2500.0f;
	
	//伤害衰减结束距离
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Damage")
	float DamageDropoffEnd = 6000.0f;
	
	//远距离最低伤害倍率(默认50）
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Damage", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float MinDamageMultiplier = 0.5f;
	
	//弹药与换弹
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Ammo", meta = (ClampMin = "1"))
	int32 MagazineSize = 30;
	
	UPROPERTY(ReplicatedUsing = OnRep_CurrentAmmo, VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon|Ammo")
	int32 CurrentAmmo = 90;
	
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Ammo", meta = (ClampMin = "0"))
	int32 MaxReserveAmmo = 180;
	
	UPROPERTY(ReplicatedUsing = OnRep_CurrentReserveAmmo, VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon|Ammo")
	int32 CurrentReserveAmmo = 180;
	
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Ammo", meta = (ClampMin = "0.1"))
	float ReloadTime = 1.8f;
	
	//准星散布控制
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Spread")
	float BaseSpread = 0.8f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Spread")
	float MaxSpread = 4.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Spread")
	float SpreadIncreasePerShot = 0.25f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Spread")
	float SpreadRecoveryRate = 5.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Spread")
	float ADSSpreadMultiplier = 0.35f;
	
	//枪口骨骼插槽名称
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Mesh")
	FName MuzzleSocketName = FName("Muzzle");
	
	//内部状态与方法
	bool bIsFiring = false;
	bool bIsReloading = false;
	float CurrentSpread = 0.8f;
	
	FTimerHandle FireTimerHandle;
	FTimerHandle ReloadTimerHandle;
	
	//执行单发HitScan射线射击
	virtual void FireOnce();
	
	// 本地播放开火视听表现（枪声、枪口粒子、蒙太奇、后坐力抖动）
	void PlayLocalFireEffects(bool bIsADS);
	
	// 网络RPC接口
	
	//1，服务端权威开火RPC（扣弹药、服务端射线检测、伤害计算）
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Fire(const FVector_NetQuantize& MuzzleLoc, const FVector_NetQuantize& TraceEnd, bool bIsADS);
	
	// 2，远端客户端广播
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_FireEffects();
	
	//3，服务端向开火客户端回传打击确认（触发本地 HUD 准星 HitMarker 闪红与音效）
	UFUNCTION(Client, Reliable)
	void Client_HitConfirm(bool bIsHeadshot, float FinalDamage);
	
	//4，服务端权威换弹RPC（扣除储备弹药、广播换弹动画与音效）
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Reload();
	
	//完成换弹逻辑
	virtual void FinishReload();
	
	//处理命中目标（伤害计算、弱点爆头、元素附着与总线广播）
	virtual void ProcessHit(const FHitResult& Hit);
	
	//计算基于距离的伤害衰减系数
	float CalculateDamageDropoff(float Distance) const;
	
	//获取双段视察矫正后的设计起点与终点
	bool CalculateTraceEndpoints(FVector& OutMuzzleLoc, FVector& OutTraceEnd, bool bIsADS) const;
	
	UFUNCTION()
	void OnRep_CurrentAmmo();
	
	UFUNCTION()
	void OnRep_CurrentReserveAmmo();
};