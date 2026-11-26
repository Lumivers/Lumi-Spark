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
class UImage;
class UProgressBar;
class UTextBlock;
struct FLSRaidReport;

// 局内限量伤害跳字运行时条目
USTRUCT()
struct FLSHUDDamagePopEntry
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextWidget = nullptr;

	FVector WorldLocation = FVector::ZeroVector;
	FLinearColor BaseColor = FLinearColor::White;
	float RemainingTime = 0.0f;
	float TotalLifeTime = 0.65f;
};

UCLASS(Abstract)
class LUMI_SPARK_API ULSHUDWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HUD|Crosshair")
	float GetCurrentSpreadRatio() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HUD|Crosshair")
	float GetCurrentSpreadGap() const { return CurrentSpreadGap; }
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HUD|Weapon")
	ALSWeaponBase* GetCurrentWeapon() const { return CurrentBoundWeapon; }

	// 核心角色数据重绑入口
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void BindToCharacter(ALSCharacterBase* NewCharacter);

	// ─── 暴露给 UMG 蓝图的表现层事件 ───
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

	// 准星四向扩散更新事件（向外位移量、散布比例、是否开镜）
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Crosshair")
	void OnCrosshairSpreadUpdated(float CurrentGap, float SpreadRatio, bool bIsADS);

protected:
	// 初始化与自动装配四向分体线条
	void SetupCrosshairLines();

	// ─── 四向分体准星组件绑定（线条自身尺寸恒定，仅向外位移；支持蓝图手动绑定或 C++ 自动装配）───
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Crosshair")
	TObjectPtr<UWidget> Crosshair_Top = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Crosshair")
	TObjectPtr<UWidget> Crosshair_Bottom = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Crosshair")
	TObjectPtr<UWidget> Crosshair_Left = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Crosshair")
	TObjectPtr<UWidget> Crosshair_Right = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Crosshair")
	TObjectPtr<UWidget> Crosshair_CenterDot = nullptr;

	// 旧单张十字贴图（若启用四向分体线将自动折叠，保持向下兼容）
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Crosshair")
	TObjectPtr<UImage> Crosshair_Image = nullptr;

	// ─── 四向对角命中标记组件绑定（支持蓝图手动绑定或 C++ 自动装配）───
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|HitMarker")
	TObjectPtr<UWidget> HitMarker_TL = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|HitMarker")
	TObjectPtr<UWidget> HitMarker_TR = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|HitMarker")
	TObjectPtr<UWidget> HitMarker_BL = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|HitMarker")
	TObjectPtr<UWidget> HitMarker_BR = nullptr;

	// 旧单张命中标记贴图（程序化生成时自动折叠隐藏，避免双重叠影）
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|HitMarker")
	TObjectPtr<UImage> HitMarker_Image = nullptr;

	// ─── 4K 实测比例映射：四向分体扩散参数（线条自身尺寸严格恒定，仅向外位移）───
	// 准星线条粗细（像素，恒定不变，默认 2.0）
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|Crosshair")
	float LineThickness = 2.0f;

	// 准星线条长度（像素，加长一倍至 20.0，向内收紧成精锐十字）
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|Crosshair")
	float LineLength = 20.0f;

	// 是否显示中心小点
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|Crosshair")
	bool bShowCenterDot = false;

	// 中心小点尺寸（宽高）
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|Crosshair")
	float CenterDotSize = 2.0f;

	// 准星线条颜色与不透明度
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|Crosshair")
	FLinearColor CrosshairColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.9f);

	// 腰射静止基础间距（中心至线条中心距离，默认 18.0px，内端距中心仅 8px，收紧成紧凑十字）
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|Crosshair")
	float HipfireBaseSpreadGap = 18.0f;

	// 腰射最大散射间距（对应 1.8 倍基准，为 32.4px）
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|Crosshair")
	float HipfireMaxSpreadGap = 32.4f;

	// 开镜肩射静止基础间距（对应 0.8 倍基准，收紧为 14.4px，内端距中心仅 4.4px 极度精准）
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|Crosshair")
	float ADSBaseSpreadGap = 14.4f;

	// 开镜肩射最大散射间距（对应 1.2 倍基准，为 21.6px）
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|Crosshair")
	float ADSMaxSpreadGap = 21.6f;

	// 准星扩散回正平滑插值速度
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|Crosshair")
	float SpreadInterpSpeed = 16.0f;

	// ─── 角色运动导致的准星动态扩散配置 ───
	// 移动扩散最大参考基准速度（厘米/秒，默认 600.0f 对应标准奔跑速度）
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|Crosshair|Movement")
	float MaxMovementSpreadSpeed = 600.0f;

	// 腰射移动最大附加扩散间距（像素，奔跑/冲刺时最多向外增加多少像素，默认 10.0f）
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|Crosshair|Movement")
	float HipfireMaxMovementSpreadGap = 10.0f;

	// 开镜肩射移动扩散乘数（开镜瞄准移动时扩散大幅衰减，默认 0.35f）
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|Crosshair|Movement")
	float ADSMovementSpreadMultiplier = 0.35f;

	// 滞空/跳跃时的额外散布间距惩罚（像素，默认 8.0f）
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|Crosshair|Movement")
	float InAirSpreadPenaltyGap = 8.0f;

	// 蹲伏姿态时的散布收紧倍率（默认 0.8f，下蹲更稳）
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|Crosshair|Movement")
	float CrouchSpreadMultiplier = 0.8f;

	// 是否启用纯 C++ 原生程序化准星绘制（彻底脱离贴图，纯代码像素绘制，换枪全自动响应）
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|Crosshair")
	bool bEnableProceduralCrosshair = true;

	// ─── 霰弹枪战术圆形准星配置 (ELSCrosshairStyle::ShotgunCircle) ───
	// 霰弹准星是否显示中心瞄准小点
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|Crosshair|Shotgun")
	bool bShowShotgunCenterDot = true;

	// 霰弹腰射静止基础圆环半径（像素）
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|Crosshair|Shotgun", meta = (ClampMin = "12.0"))
	float ShotgunHipfireBaseRadius = 40.0f;

	// 霰弹腰射最大散射圆环半径（像素）
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|Crosshair|Shotgun", meta = (ClampMin = "16.0"))
	float ShotgunHipfireMaxRadius = 64.0f;

	// 霰弹开镜肩射静止基础圆环半径（像素，右键收紧弹道）
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|Crosshair|Shotgun", meta = (ClampMin = "10.0"))
	float ShotgunADSBaseRadius = 26.0f;

	// 霰弹开镜肩射最大散射圆环半径（像素）
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|Crosshair|Shotgun", meta = (ClampMin = "12.0"))
	float ShotgunADSMaxRadius = 42.0f;

	// 霰弹圆形准星线条粗细（像素）
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|Crosshair|Shotgun", meta = (ClampMin = "1.0", ClampMax = "6.0"))
	float ShotgunCircleThickness = 2.0f;

	// 45° 对角线留白缺口角度（度），让四段弧线框住扩散区同时给 HitMarker 留出清晰空间
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|Crosshair|Shotgun", meta = (ClampMin = "0.0", ClampMax = "40.0"))
	float ShotgunCircleArcGapDegrees = 18.0f;

	// 每段圆弧的折线分段数（越大越圆滑）
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|Crosshair|Shotgun", meta = (ClampMin = "4", ClampMax = "24"))
	int32 ShotgunCircleSegmentsPerArc = 10;

	// 霰弹圆形准星颜色与不透明度
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|Crosshair|Shotgun")
	FLinearColor ShotgunCircleColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.9f);

	// 命中音效最小触发间隔（秒，防止霰弹 8 弹丸同帧命中叠音炸耳）
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|HitMarker", meta = (ClampMin = "0.0"))
	float HitSoundCooldown = 0.04f;

	// ─── 限量伤害跳字设置 ───
	// 是否开启屏幕限量伤害跳字
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|DamagePop")
	bool bEnableFloatingDamagePop = true;

	// 画面内同时存在的最大伤害跳字数量（超过时按生成顺序淘汰最早一条，FIFO）
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|DamagePop", meta = (ClampMin = "1", ClampMax = "10"))
	int32 MaxDamagePopCount = 3;

	// 单条伤害跳字存活时间（秒）
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|DamagePop", meta = (ClampMin = "0.15", ClampMax = "3.0"))
	float DamagePopLifeTime = 0.65f;

	// 伤害跳字在世界空间向上飘升速度（cm/s）
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|DamagePop", meta = (ClampMin = "0.0"))
	float DamagePopFloatSpeed = 72.0f;

	// 普通命中跳字字号
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|DamagePop", meta = (ClampMin = "8", ClampMax = "64"))
	int32 DamagePopNormalFontSize = 20;

	// 暴击/爆头跳字字号
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|DamagePop", meta = (ClampMin = "8", ClampMax = "72"))
	int32 DamagePopCritFontSize = 26;

	// 普通命中跳字颜色
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|DamagePop")
	FLinearColor DamagePopNormalColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);

	// 暴击/爆头跳字颜色（亮金橙，醒目不遮挡准星）
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|DamagePop")
	FLinearColor DamagePopCritColor = FLinearColor(1.0f, 0.82f, 0.16f, 1.0f);

	// 跳字生成随机散布半径（X: 水平半径 cm, Y: 竖直高度附加随机范围 cm）
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|DamagePop")
	FVector2D DamagePopJitterRadius = FVector2D(22.0f, 18.0f);

	// 命中标记斜线厚度（像素，默认 2.0，与准星线条保持一致）
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|HitMarker")
	float HitMarkerLineThickness = 2.0f;

	// 命中标记斜线长度（像素，默认 12.0）
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|HitMarker")
	float HitMarkerLineLength = 12.0f;

	// 普通命中颜色（默认高亮白）
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|HitMarker")
	FLinearColor HitMarkerNormalColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);

	// 爆头/弱点致命命中颜色（默认金红）
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|HitMarker")
	FLinearColor HitMarkerHeadshotColor = FLinearColor(1.0f, 0.25f, 0.25f, 1.0f);

	// 动态击中标记持续时间（秒）
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|HitMarker")
	float HitMarkerDuration = 0.16f;

	// 当前计算得到的线条偏移量（像素）
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "HUD|Crosshair")
	float CurrentSpreadGap = 18.0f;

	// 剩余击中标记存活时间
	float HitMarkerTimeRemaining = 0.0f;

	// 最近一次击中是否爆头
	bool bLastHitWasHeadshot = false;

public:
	// 主动触发命中标记闪现（供 C++ 与蓝图统一调用）
	UFUNCTION(BlueprintCallable, Category = "HUD|HitMarker")
	void TriggerHitMarker(bool bIsHeadshot);

	// 更新局内撤离倒计时条（供 PlayerController 撤离 RPC 直接驱动，免去屏幕左上角调试文字）
	UFUNCTION(BlueprintCallable, Category = "HUD|Extraction")
	void UpdateExtractionProgress(const FText& PointName, float Progress, float RemainingSeconds);

	// 隐藏局内撤离倒计时条（离开撤离区时调用）
	UFUNCTION(BlueprintCallable, Category = "HUD|Extraction")
	void HideExtractionProgress();

	// 显示撤离成功结算横幅
	UFUNCTION(BlueprintCallable, Category = "HUD|Extraction")
	void ShowExtractionSuccess(const FLSRaidReport& Report);

protected:
	// 初始化撤离提示与进度条控件（若蓝图未手动摆放则自动构建）
	void SetupExtractionHUD();

	// 生成一条先入先出限量伤害跳字
	void SpawnFloatingDamagePop(const FLSDamageContext& DamageContext);

	// 逐帧更新所有存活伤害跳字的世界投影、上浮与淡出
	void UpdateFloatingDamagePops(float DeltaTime);

	// 同步单条跳字的屏幕坐标与透明度（在屏幕视野外时自动隐藏）
	bool UpdateDamagePopScreenTransform(const FLSHUDDamagePopEntry& Entry, float Alpha) const;

	// 从对象池借用或新建一个跳字文本控件
	UTextBlock* AcquireDamagePopTextBlock();

	// 回收过期或被淘汰的跳字控件回池
	void RecycleDamagePopEntry(const FLSHUDDamagePopEntry& Entry);

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

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ExtractionTitleText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> ExtractionProgressBar = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ExtractionResultText = nullptr;

	FText CachedExtractionPointName;
	double LastHitSoundTime = -1.0;

	// 按生成顺序排列的存活伤害跳字队列（索引 0 为最早生成，超出 MaxDamagePopCount 时优先淘汰）
	UPROPERTY(Transient)
	TArray<FLSHUDDamagePopEntry> ActiveDamagePops;

	// 已构建的可复用跳字控件池
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> DamagePopWidgetPool;
};