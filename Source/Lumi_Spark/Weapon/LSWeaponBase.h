#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Core/LSTypes.h"
#include "Engine/NetSerialization.h"
#include "LSWeaponBase.generated.h"

class USkeletalMeshComponent;
class ULSRecoilComponent;
class ULSWeaponDataAsset;
class UNiagaraSystem;
class UCameraShakeBase;

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

	// 获取武器数据资产
	UFUNCTION(BlueprintCallable, Category = "Weapon|Init")
	virtual void InitializeFromDataAsset(const ULSWeaponDataAsset* InDataAsset);

	// 重置插槽附着基础变换缓存（在切换武器或更换插槽时调用，防止跨插槽变换污染）
	UFUNCTION(BlueprintCallable, Category = "Weapon|Attach")
	void ResetAttachTransformCache();
	
	//组件获取
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	FORCEINLINE ULSRecoilComponent* GetRecoilComponent() const { return RecoilComponent; }
	
	// 查询武器上的左手握持插槽世界空间变换（自动遍历自身所有 Mesh 组件）
	UFUNCTION(BlueprintCallable, Category = "Weapon|IK")
	virtual bool GetLeftHandSocketTransform(FTransform& OutTransform, FName SocketName = FName("LeftHandSocket")) const;

	// 查询枪口世界变换（优先匹配插槽，若无插槽则自动基于网格体前沿包围盒计算精准枪口）
	UFUNCTION(BlueprintCallable, Category = "Weapon|Socket")
	virtual bool GetMuzzleTransform(FTransform& OutTransform) const;

	// 查询枪口世界坐标
	UFUNCTION(BlueprintCallable, Category = "Weapon|Socket")
	virtual FVector GetMuzzleLocation() const;

	// 获取适合挂载枪火特效的组件和对应插槽名（若无插槽则返回网格体与 NAME_None）
	virtual USceneComponent* GetMuzzleAttachComponent(FName& OutSocketName) const;

	FORCEINLINE int32 GetCurrentAmmo() const { return CurrentAmmo; }
	FORCEINLINE int32 GetMagazineSize() const { return MagazineSize; }
	FORCEINLINE int32 GetCurrentReserveAmmo() const { return CurrentReserveAmmo; }
	FORCEINLINE FGameplayTag GetElementTag() const { return ElementTag; }
	FORCEINLINE float GetCurrentSpread() const { return CurrentSpread; }
	FORCEINLINE float GetBaseSpread() const { return BaseSpread; }
	FORCEINLINE float GetMaxSpread() const { return MaxSpread; }
	FORCEINLINE float GetBaseDamage() const { return BaseDamage; }
	FORCEINLINE bool IsFiring() const { return bIsFiring; }
	FORCEINLINE bool IsReloading() const { return bIsReloading; }
	FORCEINLINE ELSCrosshairStyle GetCrosshairStyle() const { return CrosshairStyle; }
	FORCEINLINE float GetBulletTracerSpeed() const { return BulletTracerSpeed; }
	FORCEINLINE float GetTracerRibbonLength() const { return TracerRibbonLength; }
	FORCEINLINE bool IsDirectTraceEndTimingEnabled() const { return bUseDirectTraceEndTiming; }
	FORCEINLINE bool IsWeaponDebugDrawEnabled() const { return bEnableWeaponDebugDraw; }
	
	FORCEINLINE float GetSpreadRatio() const 
	{ 
		return (MaxSpread > BaseSpread) ? FMath::Clamp((CurrentSpread - BaseSpread) / (MaxSpread - BaseSpread), 0.0f, 1.0f) : 0.0f; 
	}

	// 获取综合了连射热度、开镜衰减、角色水平移速与滞空等全状态的最终真实弹道散布角（度）
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Weapon|Spread")
	float GetFinalSpreadAngle(bool bInIsADS) const;
	
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
	
	//角色全身换弹动作蒙太奇
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Animation")
	TObjectPtr<UAnimMontage> CharacterReloadMontage;
	
	//音效与开火表现
	
	// 开火镜头微震颤效果类 (Camera Shake)
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Effects")
	TSubclassOf<UCameraShakeBase> FireCameraShakeClass;

	//开火音效
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Effects")
	TObjectPtr<USoundBase> FireSound;
	
	//换弹音效
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Effects")
	TObjectPtr<USoundBase> ReloadSound;
	
	// 枪口火光粒子 (Niagara，优先使用)
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Effects")
	TObjectPtr<UNiagaraSystem> MuzzleFlashNiagara;

	// 枪口火光粒子 (Cascade 传统粒子，备用)
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Effects")
	TObjectPtr<UParticleSystem> MuzzleFlashEmitter;
	
	// 1. 击中表面火花/碎屑粒子 (Niagara，优先使用，环境/掩体受击)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Effects")
	TObjectPtr<UNiagaraSystem> ImpactNiagara;

	// 1. 击中生物肉体受击/飙血粒子 (Niagara，角色/敌人受击)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Effects")
	TObjectPtr<UNiagaraSystem> FleshImpactNiagara;

	// 1. 击中表面火花/碎屑粒子 (Cascade 传统粒子，备用)
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Effects")
	TObjectPtr<UParticleSystem> ImpactEmitter;
	
	// 2. 击中表面物理受击音效（打在墙壁/金属上的撞击声）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Effects")
	TObjectPtr<USoundBase> ImpactSound;

	// 2. 击中生物肉体受击音效（打在人身上的入肉闷响/撕裂声）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Effects")
	TObjectPtr<USoundBase> FleshImpactSound;
	
	// 3. 击中表面弹孔贴花材质（留在墙壁上的焦黑弹坑）
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Effects")
	TObjectPtr<UMaterialInterface> ImpactDecalMaterial;
	
	// 弹孔贴花尺寸（X为沿法线投射深度，Y/Z为贴花长宽半尺寸）
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Effects")
	FVector DecalSize = FVector(30.0f, 12.0f, 12.0f);
	
	// 弹孔贴花留存时间（秒）
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Effects")
	float DecalLifeSpan = 15.0f;
	
	// 最多同时存在的弹孔贴花数量（超出时自动销毁最旧弹孔，避免过多贴花造成渲染卡顿）
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Effects", meta = (ClampMin = "1", ClampMax = "100"))
	int32 MaxDecalCount = 20;

	// 活跃弹孔贴花池（先进先出循环管理）
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<UDecalComponent>> ActiveDecals;
	
	// 4. 子弹曳光烟道粒子 (Niagara，战术级飞行曳光系统)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Effects")
	TObjectPtr<UNiagaraSystem> TracerNiagara;

	// 4. 子弹曳光烟道粒子 (Cascade 传统粒子，备用)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Effects")
	TObjectPtr<UParticleSystem> TracerEmitter;

	// 子弹视觉飞行速度 (米/秒 m/s，默认 800 m/s，步枪 750~850，冲锋枪 650~700，狙击枪 1200+，霰弹 450)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Effects", meta = (ClampMin = "50.0"))
	float BulletTracerSpeed = 800.0f;

	// 曳光尾迹烟道长度 (米，默认 2.0 米，有效控制在 1.5~3 米区间，杜绝贯穿屏幕死线)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Effects", meta = (ClampMin = "0.1", ClampMax = "10.0"))
	float TracerRibbonLength = 2.0f;

	// 是否启用飞行延时受击响应（中远距离依据弹道飞行时间 T = Distance / Speed 延迟触发打击火花与弹孔）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Effects")
	bool bUseDirectTraceEndTiming = true;

	// 弹道近距离即时受击阈值（单位：厘米，默认 1500 cm = 15 米；在此距离内无需延迟，瞬间爆出火花）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Effects", meta = (ClampMin = "0.0"))
	float InstantImpactThresholdDistance = 1500.0f;

	// Cascade 曳光粒子目标点向量参数名（Cascade 粒子默认通常是 "Target"）
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Effects")
	FName TracerTargetParamName = FName("Target");

	// Niagara 曳光粒子目标点向量参数名（Lyra 等通常为 "User.TraceEnd" 或 "TraceEnd"）
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Effects")
	FName TracerNiagaraParamName = FName("TraceEnd");

	// 是否启用武器射线调试绘制（默认关闭；开启后用 DrawDebugLine 绘制瞬时射线供排查）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Debug")
	bool bEnableWeaponDebugDraw = false;
	
	//武器基础身份
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Identity")
	FText WeaponDisplayName = FText::FromString(TEXT("基础步枪"));
	
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Identity")
	FGameplayTag WeaponTypeTag;

	// 该武器绑定的准星呈现形态（换枪时 HUD 依据此属性自动切换准星）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Crosshair")
	ELSCrosshairStyle CrosshairStyle = ELSCrosshairStyle::ClassicCross;
	
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

	// 开启无限弹药（开启后射击不消耗子弹、备弹无限，免除调试频繁打空哑火）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Ammo")
	bool bInfiniteAmmo = false;
	
	// --- 程序化武器物理后坐力回冲（开火枪身自身机械后挫与枪口上跳） ---
	
	// 每次开火枪身后坐后退距离（厘米，沿枪管轴向后退，战术紧凑微冲量，避免枪身与手臂脱节）
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|RecoilKick", meta = (ClampMin = "0.0"))
	float RecoilKickDistance = 0.8f;

	// 每次开火枪口瞬间上扬仰角（度，Pitch 旋转，配合脊椎仰角形成细腻枪口跳动）
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|RecoilKick", meta = (ClampMin = "0.0"))
	float RecoilKickPitch = 0.7f;

	// 每次开火枪身随机微量横偏角（度，Yaw/Roll 摆动）
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|RecoilKick", meta = (ClampMin = "0.0"))
	float RecoilKickYaw = 0.25f;

	// 枪身后坐复位回弹速度（与角色脊椎回正速率对齐，保证人枪回弹相位同步）
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|RecoilKick", meta = (ClampMin = "1.0"))
	float RecoilRecoverySpeed = 20.0f;

	// 最大累积后退距离限制（厘米，防止连发时枪模持续过度后移）
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|RecoilKick")
	float MaxRecoilKickDistance = 1.8f;

	// 开镜瞄准时枪身程序化抖动的阻尼倍率（保持与角色躯干衰减比例完全一致）
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|RecoilKick", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ADSRecoilKickMultiplier = 0.5f;

	// 触发一次程序化物理枪身抖动与后坐力回冲（支持开镜状态阻尼衰减）
	virtual void ApplyProceduralRecoilKick(bool bIsADS = false);

	// 每帧物理回正程序化枪身偏移
	virtual void UpdateProceduralRecoilKick(float DeltaTime);
	
	//准星散布控制
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Spread")
	float BaseSpread = 0.8f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Spread")
	float MaxSpread = 4.0f;
	
	// 连射单发散布增量（默认约 10 发连射达到最大散布天花板）
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Spread")
	float SpreadIncreasePerShot = 0.32f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Spread")
	float SpreadRecoveryRate = 5.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Spread")
	float ADSSpreadMultiplier = 0.35f;

	// 角色水平移动对武器真实弹道散布的惩罚系数（度/最大移速，奔跑时最多附加多少度弹道散布，默认 1.5°）
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Spread")
	float MovementSpreadPenalty = 1.5f;

	// 角色滞空（跳跃/翻爬/坠落）时的弹道散布惩罚（度，默认 2.5°）
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Spread")
	float InAirSpreadPenalty = 2.5f;
	
	//枪口骨骼插槽名称
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Mesh")
	FName MuzzleSocketName = FName("Muzzle");
	
	// 是否优先严格以枪口插槽物理朝向射击（true: 纯枪管弹道；false: 现代TPS标准——从枪口精确打向屏幕准星落点）
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Fire")
	bool bFireAlongMuzzleForward = false;

	// 是否在每次开火时强制把角色朝向瞬移对齐到准星（与 ALS 平滑转身并行开启会在甩镜头时产生朝向拉扯，默认关闭交由 ALS Aiming 自然跟随）
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon|Fire")
	bool bSnapOwnerRotationOnFire = false;
	
	// 是否开启枪身随镜头仰俯动态对齐
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Aim")
	bool bEnableProceduralAimPitch = false;

	// 向上抬枪倍率（负数纠正手部骨骼轴向，控制抬头仰角幅度，默认 -0.8）
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Aim")
	float ProceduralAimPitchUpMultiplier = -0.8f;

	// 向下压枪倍率（独立非对称调小，防止低头时枪口猛戳地面，推荐 -0.3 ~ -0.4）
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Aim")
	float ProceduralAimPitchDownMultiplier = -0.35f;

	// 向下最大俯视压枪极限角度（负数，防戳地物理安全锁，默认 -20°）
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Aim")
	float MinAimPitchLimit = -20.0f;

	// 向上最大仰视抬枪极限角度（正数，默认 +45°）
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Aim")
	float MaxAimPitchLimit = 45.0f;

	// 基础持枪位置位移微调偏移（X=前后, Y=左右, Z=上下，单位厘米，可在蓝图中微调枪身在手里的初始位移）
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon|Aim")
	FVector GripLocationOffset = FVector::ZeroVector;

	// 基础持枪旋转微调偏移（Pitch/Yaw/Roll，可在蓝图中微调枪身在手里的初始姿态）
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon|Aim")
	FRotator GripRotationOffset = FRotator(0.0f, 0.0f, 0.0f);

	// 当前平滑插值后的仰俯角
	float CurrentProceduralAimPitch = 0.0f;
	
	//内部状态与方法
	bool bIsFiring = false;
	bool bIsReloading = false;
	bool bWantsToFire = false;
	float CurrentSpread = 0.8f;

	// 上一次实际开火的世界时间（用于统一校验半自动与全自动的 FireRate 冷却）
	double LastFireTime = -100.0;
	
	// 程序化枪身位移与旋转偏移
	FVector CurrentRecoilKickLoc = FVector::ZeroVector;
	FRotator CurrentRecoilKickRot = FRotator::ZeroRotator;
	FTransform BaseAttachRelativeTransform;
	bool bHasCachedBaseTransform = false;
	
	FTimerHandle FireTimerHandle;
	FTimerHandle ReloadTimerHandle;
	
	// 查询距下一发允许开火的剩余冷却秒数（<= 0 表示可立即开火）
	float GetRemainingFireCooldown() const;

	// 判定命中目标是否为角色/生物实体（用于分流肉体与环境受击反馈）
	static bool IsCharacterImpactTarget(const AActor* HitActor);

	//执行单发HitScan射线射击
	virtual void FireOnce();
	
	// 本地播放开火视听表现（枪声、枪口粒子、蒙太奇、后坐力抖动）
	void PlayLocalFireEffects(bool bIsADS);
	
	// 获取当前武器的换弹速度加成倍率（读取驱动核心，保底为 1.0）
	float GetReloadSpeedMultiplier() const;
	
	// 网络RPC接口
	
	//1，服务端权威开火RPC（扣弹药、服务端射线检测、伤害计算）
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Fire(const FVector_NetQuantize& MuzzleLoc, const FVector_NetQuantize& TraceEnd, bool bIsADS);
	
	// 2，远端客户端广播开火视听表现
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_FireEffects(const FVector_NetQuantize& MuzzleLoc, const FVector_NetQuantize& TraceEnd);
	
	//多播广播击中表面物理表现
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_ImpactEffects(const FHitResult& Hit, bool bPlaySound);
	
	// 3，服务端向开火客户端回传打击确认（触发本地 HUD 准星 HitMarker 闪红、音效与限量跳字）
	UFUNCTION(Client, Reliable)
	void Client_HitConfirm(bool bIsHeadshot, float FinalDamage, const FVector_NetQuantize& HitLocation);
	
	//4，服务端权威换弹RPC（扣除储备弹药、广播换弹动画与音效）
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Reload();
	
	//完成换弹逻辑
	virtual void FinishReload();
	
	//处理命中目标（伤害计算、弱点爆头、元素附着与总线广播）
	virtual void ProcessHit(const FHitResult& Hit);
	
	//计算基于距离的伤害衰减系数
	virtual float CalculateDamageDropoff(float Distance) const;
	
	//获取双段视察矫正后的设计起点与终点
	bool CalculateTraceEndpoints(FVector& OutMuzzleLoc, FVector& OutTraceEnd, bool bIsADS) const;
	
	UFUNCTION()
	void OnRep_CurrentAmmo();
	
	UFUNCTION()
	void OnRep_CurrentReserveAmmo();
	
	// 本地播放击中表面物理表现（火花、音效、贴花，根据距离与飞行速度自动调度延时）
	void PlayImpactEffects(const FHitResult& Hit, bool bPlaySound = true);

	// 真正执行击中表面物理生成（火花、音效、贴花）
	virtual void ExecuteImpactEffects(const FHitResult& Hit, bool bPlaySound = true);
	
	// 本地/远端播放子弹曳光弹道烟道
	void PlayTracerEffect(const FVector& StartLoc, const FVector& EndLoc);
};