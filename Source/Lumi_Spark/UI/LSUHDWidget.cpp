#include "LSUHDWidget.h"
#include "Weapon/LSWeaponBase.h"
#include "Weapon/LSWeaponComponent.h"
#include "Core/LSEventBus.h"
#include "Kismet/GameplayStatics.h"
#include "Character/LSCharacterBase.h"
#include "Character/LSCameraComponent.h"
#include "Character/LSHealthComponent.h"
#include "Character/LSStaminaComponent.h"
#include "Character/LSEnergyComponent.h"
#include "Character/LSSkillComponent.h"
#include "Components/Image.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "GameFramework/CharacterMovementComponent.h"

void ULSHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	// 彻底解决开局米字准星 Bug：命中标记 HitMarker 开局必须显式折叠隐藏
	if (HitMarker_Image)
	{
		HitMarker_Image->SetVisibility(ESlateVisibility::Collapsed);
		HitMarker_Image->SetRenderOpacity(0.0f);
	}

	// 自动配置或接管四向分体准星线条与对角 45° HitMarker 线条
	SetupCrosshairLines();

	APawn* OwningPawn = GetOwningPlayerPawn();
	if (!OwningPawn) return;
	
	CachedWeaponComp = OwningPawn->FindComponentByClass<ULSWeaponComponent>();
	if (CachedWeaponComp)
	{
		CachedWeaponComp->OnWeaponChanged.AddDynamic(this, &ULSHUDWidget::HandleWeaponChanged);
		BindToWeapon(CachedWeaponComp->GetCurrentWeapon());
	}
	
	if (ULSEventBus* EventBus = ULSEventBus::Get(this))
	{
		EventBus->OnDamageDealt.AddDynamic(this, &ULSHUDWidget::HandleDamageDealt);
		EventBus->OnRaidWiped.AddDynamic(this, &ULSHUDWidget::HandleGlobalRaidWiped);
	}
}

void ULSHUDWidget::NativeDestruct()
{
	if (CachedWeaponComp)
	{
		CachedWeaponComp->OnWeaponChanged.RemoveDynamic(this, &ULSHUDWidget::HandleWeaponChanged);
	}
	if (CurrentBoundWeapon)
	{
		CurrentBoundWeapon->OnAmmoChanged.RemoveDynamic(this, &ULSHUDWidget::HandleAmmoChanged);
	}
	if (ULSEventBus* EventBus = ULSEventBus::Get(this))
	{
		EventBus->OnDamageDealt.RemoveDynamic(this, &ULSHUDWidget::HandleDamageDealt);
		EventBus->OnRaidWiped.RemoveDynamic(this, &ULSHUDWidget::HandleGlobalRaidWiped);
	}
	
	Super::NativeDestruct();
}

void ULSHUDWidget::SetupCrosshairLines()
{
	// 1. 折叠旧的单张缩放十字贴图与旧单图静态 HitMarker，避免双重叠影割裂
	if (Crosshair_Image)
	{
		Crosshair_Image->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (HitMarker_Image)
	{
		HitMarker_Image->SetVisibility(ESlateVisibility::Collapsed);
		HitMarker_Image->SetRenderOpacity(0.0f);
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(GetRootWidget());
	if (!RootCanvas)
	{
		return;
	}

	// 创建单根线条 Image 的通用辅助函数
	auto CreateLineImage = [this, RootCanvas](FName InName, const FVector2D& InSize, int32 InZOrder) -> UImage*
	{
		UImage* LineImg = NewObject<UImage>(this, InName);
		if (!LineImg) return nullptr;

		LineImg->SetColorAndOpacity(CrosshairColor);

		if (UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(LineImg))
		{
			PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
			PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			PanelSlot->SetPosition(FVector2D::ZeroVector);
			PanelSlot->SetSize(InSize);
			PanelSlot->SetZOrder(InZOrder);
		}
		return LineImg;
	};

	// 2. 装配正向准星线条（若蓝图未手动绑定则自动生成，若已绑定则更新尺寸）
	if (!Crosshair_Top && !Crosshair_Bottom && !Crosshair_Left && !Crosshair_Right)
	{
		// 上下垂直线：宽 = LineThickness (2px), 高 = LineLength (20px)
		Crosshair_Top = CreateLineImage(FName("Auto_Crosshair_Top"), FVector2D(LineThickness, LineLength), 0);
		Crosshair_Bottom = CreateLineImage(FName("Auto_Crosshair_Bottom"), FVector2D(LineThickness, LineLength), 0);

		// 左右水平线：宽 = LineLength (20px), 高 = LineThickness (2px)
		Crosshair_Left = CreateLineImage(FName("Auto_Crosshair_Left"), FVector2D(LineLength, LineThickness), 0);
		Crosshair_Right = CreateLineImage(FName("Auto_Crosshair_Right"), FVector2D(LineLength, LineThickness), 0);

		// 中心小点 (ZOrder 1)
		Crosshair_CenterDot = CreateLineImage(FName("Auto_Crosshair_Dot"), FVector2D(CenterDotSize, CenterDotSize), 1);
		if (Crosshair_CenterDot)
		{
			Crosshair_CenterDot->SetVisibility(bShowCenterDot ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
	}
	else
	{
		// 确保已绑定的正向线条几何尺寸同步最新设定
		if (Crosshair_Top) if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Crosshair_Top->Slot)) CanvasSlot->SetSize(FVector2D(LineThickness, LineLength));
		if (Crosshair_Bottom) if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Crosshair_Bottom->Slot)) CanvasSlot->SetSize(FVector2D(LineThickness, LineLength));
		if (Crosshair_Left) if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Crosshair_Left->Slot)) CanvasSlot->SetSize(FVector2D(LineLength, LineThickness));
		if (Crosshair_Right) if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Crosshair_Right->Slot)) CanvasSlot->SetSize(FVector2D(LineLength, LineThickness));
	}

	// 3. 装配对角 45° 命中标记 HitMarker 线条（若蓝图未手动绑定则自动生成，ZOrder 2 位于顶层）
	if (!HitMarker_TL && !HitMarker_TR && !HitMarker_BL && !HitMarker_BR)
	{
		auto CreateHitMarkerLine = [&](FName InName, float AngleDeg) -> UImage*
		{
			UImage* TickImg = CreateLineImage(InName, FVector2D(HitMarkerLineThickness, HitMarkerLineLength), 2);
			if (TickImg)
			{
				TickImg->SetRenderTransformAngle(AngleDeg);
				TickImg->SetVisibility(ESlateVisibility::Collapsed);
			}
			return TickImg;
		};

		HitMarker_TL = CreateHitMarkerLine(FName("Auto_HitMarker_TL"), -45.0f);
		HitMarker_TR = CreateHitMarkerLine(FName("Auto_HitMarker_TR"), 45.0f);
		HitMarker_BL = CreateHitMarkerLine(FName("Auto_HitMarker_BL"), 45.0f);
		HitMarker_BR = CreateHitMarkerLine(FName("Auto_HitMarker_BR"), -45.0f);
	}
	else
	{
		auto EnsureHitMarkerLine = [this](UWidget* TickWidget, float AngleDeg)
		{
			if (!TickWidget) return;
			if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(TickWidget->Slot))
			{
				CanvasSlot->SetSize(FVector2D(HitMarkerLineThickness, HitMarkerLineLength));
			}
			TickWidget->SetRenderTransformAngle(AngleDeg);
			TickWidget->SetVisibility(ESlateVisibility::Collapsed);
		};
		EnsureHitMarkerLine(HitMarker_TL, -45.0f);
		EnsureHitMarkerLine(HitMarker_TR, 45.0f);
		EnsureHitMarkerLine(HitMarker_BL, 45.0f);
		EnsureHitMarkerLine(HitMarker_BR, -45.0f);
	}
}

void ULSHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 1. 获取当前绑定的武器连射散布比例
	float FiringSpreadRatio = 0.0f;
	if (CurrentBoundWeapon)
	{
		FiringSpreadRatio = CurrentBoundWeapon->GetSpreadRatio();
	}

	// 自动容错获取当前控制的角色
	if (!BoundCharacter)
	{
		BoundCharacter = Cast<ALSCharacterBase>(GetOwningPlayerPawn());
	}

	// 2. 检测角色状态（开镜、水平跑动速度、滞空、蹲伏）
	bool bIsADS = false;
	float MoveRatio = 0.0f;
	bool bIsInAir = false;
	bool bIsCrouched = false;

	if (BoundCharacter)
	{
		if (ULSCameraComponent* Cam = BoundCharacter->GetCameraComponent())
		{
			bIsADS = Cam->IsADS();
		}

		// 计算角色水平移动速度比例（仅取 X/Y 轴速度，避免起伏下落干扰速度计）
		const float HorizontalSpeed = BoundCharacter->GetVelocity().Size2D();
		MoveRatio = FMath::Clamp(HorizontalSpeed / FMath::Max(1.0f, MaxMovementSpreadSpeed), 0.0f, 1.0f);

		if (UCharacterMovementComponent* MoveComp = BoundCharacter->GetCharacterMovement())
		{
			bIsInAir = MoveComp->IsFalling();
			bIsCrouched = MoveComp->IsCrouching();
		}
	}

	// 3. 基础静止间距与连射热度扩散：
	// 腰射：基准 18px (内端距中心仅 8px) -> 连射最大散布 32.4px (1.8 倍)
	// 肩射：基准 14.4px (0.8 倍，内端距中心仅 4.4px) -> 连射最大散布 21.6px (1.2 倍)
	const float BaseMin = bIsADS ? ADSBaseSpreadGap : HipfireBaseSpreadGap;
	const float BaseMax = bIsADS ? ADSMaxSpreadGap : HipfireMaxSpreadGap;
	float TargetGap = FMath::Lerp(BaseMin, BaseMax, FiringSpreadRatio);

	// 4. 跑路与跳跃动态外扩惩罚（速度越快扩散越大）：
	float MovementAdd = MoveRatio * HipfireMaxMovementSpreadGap;
	if (bIsInAir)
	{
		MovementAdd += InAirSpreadPenaltyGap;
	}
	if (bIsADS)
	{
		MovementAdd *= ADSMovementSpreadMultiplier;
	}
	if (bIsCrouched)
	{
		MovementAdd *= CrouchSpreadMultiplier;
	}

	TargetGap += MovementAdd;

	// 5. 极速平滑插值，兼顾起步外扩的呼吸感与停步急停的弹性回正
	CurrentSpreadGap = FMath::FInterpTo(CurrentSpreadGap, TargetGap, InDeltaTime, SpreadInterpSpeed);

	// 6. 正向四条准星线条向外平移扩散（线条自身的尺寸与粗细严格恒定不变！）
	if (Crosshair_Top)
	{
		Crosshair_Top->SetRenderTranslation(FVector2D(0.0f, -CurrentSpreadGap));
	}
	if (Crosshair_Bottom)
	{
		Crosshair_Bottom->SetRenderTranslation(FVector2D(0.0f, CurrentSpreadGap));
	}
	if (Crosshair_Left)
	{
		Crosshair_Left->SetRenderTranslation(FVector2D(-CurrentSpreadGap, 0.0f));
	}
	if (Crosshair_Right)
	{
		Crosshair_Right->SetRenderTranslation(FVector2D(CurrentSpreadGap, 0.0f));
	}

	// 7. 命中标记 HitMarker 动态更新（对角 45° 扩散，完全与准星 CurrentSpreadGap 保持同心同频）
	if (HitMarkerTimeRemaining > 0.0f)
	{
		HitMarkerTimeRemaining -= InDeltaTime;
		const float Alpha = FMath::Clamp(HitMarkerTimeRemaining / HitMarkerDuration, 0.0f, 1.0f);
		
		// 爆头金红，普中高亮白
		const FLinearColor HitColor = bLastHitWasHeadshot ? HitMarkerHeadshotColor : HitMarkerNormalColor;
		const FLinearColor FinalColor(HitColor.R, HitColor.G, HitColor.B, HitColor.A * Alpha);

		// 对角线距离：R * cos(45°) = R * 0.70710678f
		// 这样 4 根命中斜线与 4 根准星正向线恰好落在同一个半径为 CurrentSpreadGap 的圆周上！
		const float DiagOffset = CurrentSpreadGap * 0.70710678f;

		auto UpdateHitMarkerTick = [FinalColor](UWidget* TickWidget, const FVector2D& Translation)
		{
			if (!TickWidget) return;
			if (TickWidget->GetVisibility() != ESlateVisibility::HitTestInvisible)
			{
				TickWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
			TickWidget->SetRenderTranslation(Translation);
			if (UImage* Img = Cast<UImage>(TickWidget))
			{
				Img->SetColorAndOpacity(FinalColor);
			}
		};

		UpdateHitMarkerTick(HitMarker_TL, FVector2D(-DiagOffset, -DiagOffset));
		UpdateHitMarkerTick(HitMarker_TR, FVector2D(DiagOffset, -DiagOffset));
		UpdateHitMarkerTick(HitMarker_BL, FVector2D(-DiagOffset, DiagOffset));
		UpdateHitMarkerTick(HitMarker_BR, FVector2D(DiagOffset, DiagOffset));
	}
	else
	{
		// 倒计时结束，折叠 HitMarker，零渲染与布局开销
		if (HitMarker_TL && HitMarker_TL->GetVisibility() != ESlateVisibility::Collapsed) HitMarker_TL->SetVisibility(ESlateVisibility::Collapsed);
		if (HitMarker_TR && HitMarker_TR->GetVisibility() != ESlateVisibility::Collapsed) HitMarker_TR->SetVisibility(ESlateVisibility::Collapsed);
		if (HitMarker_BL && HitMarker_BL->GetVisibility() != ESlateVisibility::Collapsed) HitMarker_BL->SetVisibility(ESlateVisibility::Collapsed);
		if (HitMarker_BR && HitMarker_BR->GetVisibility() != ESlateVisibility::Collapsed) HitMarker_BR->SetVisibility(ESlateVisibility::Collapsed);
	}

	// 彻底屏蔽旧版静态单图 HitMarker_Image，防止蓝图动画强行显示导致的割裂重影
	if (HitMarker_Image && HitMarker_Image->GetVisibility() != ESlateVisibility::Collapsed)
	{
		HitMarker_Image->SetVisibility(ESlateVisibility::Collapsed);
		HitMarker_Image->SetRenderOpacity(0.0f);
	}

	// 8. 广播蓝图事件，允许蓝图层获取精确的像素位移与开镜状态
	OnCrosshairSpreadUpdated(CurrentSpreadGap, FiringSpreadRatio, bIsADS);
}

void ULSHUDWidget::BindToWeapon(ALSWeaponBase* Weapon)
{
	if (CurrentBoundWeapon)
	{
		CurrentBoundWeapon->OnAmmoChanged.RemoveDynamic(this, &ULSHUDWidget::HandleAmmoChanged);
	}
	
	CurrentBoundWeapon = Weapon;
	
	if (CurrentBoundWeapon)
	{
		CurrentBoundWeapon->OnAmmoChanged.AddDynamic(this, &ULSHUDWidget::HandleAmmoChanged);
		OnAmmoUpdated(CurrentBoundWeapon->GetCurrentAmmo(), CurrentBoundWeapon->GetMagazineSize(), CurrentBoundWeapon->GetCurrentReserveAmmo());

		// 依据新换上的武器准星形态，自适应调整四向线条可见性
		const ELSCrosshairStyle Style = CurrentBoundWeapon->GetCrosshairStyle();
		const ESlateVisibility LinesVis = (Style == ELSCrosshairStyle::None || Style == ELSCrosshairStyle::DotOnly) ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible;
		const ESlateVisibility DotVis = (Style == ELSCrosshairStyle::DotOnly || bShowCenterDot) ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;

		if (Crosshair_Top) Crosshair_Top->SetVisibility(LinesVis);
		if (Crosshair_Bottom) Crosshair_Bottom->SetVisibility(LinesVis);
		if (Crosshair_Left) Crosshair_Left->SetVisibility(LinesVis);
		if (Crosshair_Right) Crosshair_Right->SetVisibility(LinesVis);
		if (Crosshair_CenterDot) Crosshair_CenterDot->SetVisibility(DotVis);
	}
	
	OnWeaponSwitched(CurrentBoundWeapon);
}

void ULSHUDWidget::HandleWeaponChanged(ALSWeaponBase* NewWeapon)
{
	BindToWeapon(NewWeapon);
}

void ULSHUDWidget::HandleAmmoChanged(int32 CurrentAmmo, int32 MagazineSize, int32 ReserveAmmo)
{
	OnAmmoUpdated(CurrentAmmo, MagazineSize, ReserveAmmo);
}

void ULSHUDWidget::TriggerHitMarker(bool bIsHeadshot)
{
	HitMarkerTimeRemaining = HitMarkerDuration;
	bLastHitWasHeadshot = bIsHeadshot;
	OnHitMarkerTriggered(bIsHeadshot);
}

void ULSHUDWidget::HandleDamageDealt(const FLSDamageContext& DamageContext)
{
	if (DamageContext.DamageCauser != GetOwningPlayerPawn()) return;
	
	USoundBase* SoundToPlay = DamageContext.bIsHeadshot ? HitHeadshotSound : HitNormalSound;
	if (SoundToPlay)
	{
		UGameplayStatics::PlaySound2D(this, SoundToPlay);
	}
	
	TriggerHitMarker(DamageContext.bIsHeadshot);
}

float ULSHUDWidget::GetCurrentSpreadRatio() const
{
	return CurrentBoundWeapon ? CurrentBoundWeapon->GetSpreadRatio() : 0.0f;
}

void ULSHUDWidget::BindToCharacter(ALSCharacterBase* NewCharacter)
{
    if (NewCharacter == BoundCharacter && BoundCharacter != nullptr) return;

    // 1. 直连解绑旧组件的委托
    if (BoundHealthComp)
    {
        BoundHealthComp->OnHealthChanged.RemoveDynamic(this, &ULSHUDWidget::HandleHealthChanged);
        BoundHealthComp->OnLowHealth.RemoveDynamic(this, &ULSHUDWidget::HandleLowHealth);
    }
    if (BoundStaminaComp)
    {
        BoundStaminaComp->OnStaminaChanged.RemoveDynamic(this, &ULSHUDWidget::HandleStaminaChanged);
    }
    if (BoundEnergyComp)
    {
        BoundEnergyComp->OnEnergyChanged.RemoveDynamic(this, &ULSHUDWidget::HandleEnergyChanged);
    }
    if (BoundSkillComp)
    {
        BoundSkillComp->OnSkillCooldownChanged.RemoveDynamic(this, &ULSHUDWidget::HandleSkillCooldownChanged);
    }

    BoundCharacter = NewCharacter;
    if (!BoundCharacter) return;

    // 重新绑定武器
    if (ULSWeaponComponent* NewWeaponComp = BoundCharacter->GetWeaponComponent())
    {
        BindToWeapon(NewWeaponComp->GetCurrentWeapon());
    }

    // 2. 直连绑定新角色的资源组件，并立即主动推流初始值！
    BoundHealthComp = BoundCharacter->GetHealthComponent();
    if (BoundHealthComp)
    {
        BoundHealthComp->OnHealthChanged.AddDynamic(this, &ULSHUDWidget::HandleHealthChanged);
        BoundHealthComp->OnLowHealth.AddDynamic(this, &ULSHUDWidget::HandleLowHealth);

        OnHealthUpdated(BoundHealthComp->GetCurrentHealth(), BoundHealthComp->GetMaxHealth());
        OnLowHealthWarning(BoundHealthComp->GetHealthPercent() <= 0.2f);
    }

    BoundStaminaComp = BoundCharacter->GetStaminaComponent();
    if (BoundStaminaComp)
    {
        BoundStaminaComp->OnStaminaChanged.AddDynamic(this, &ULSHUDWidget::HandleStaminaChanged);
        OnStaminaUpdated(BoundStaminaComp->GetCurrentStamina(), BoundStaminaComp->GetMaxStamina(), BoundStaminaComp->GetStaminaRatio());
    }

    BoundEnergyComp = BoundCharacter->GetEnergyComponent();
    if (BoundEnergyComp)
    {
        BoundEnergyComp->OnEnergyChanged.AddDynamic(this, &ULSHUDWidget::HandleEnergyChanged);
        OnBurstEnergyUpdated(BoundEnergyComp->GetCurrentEnergy(), BoundEnergyComp->GetMaxEnergy(), BoundEnergyComp->GetEnergyRatio());
    }

    BoundSkillComp = BoundCharacter->GetSkillComponent();
    if (BoundSkillComp)
    {
        BoundSkillComp->OnSkillCooldownChanged.AddDynamic(this, &ULSHUDWidget::HandleSkillCooldownChanged);
        OnSkillCooldownUpdated(BoundSkillComp->GetSkillCooldownRemaining(), 
                               BoundSkillComp->GetSkillCooldownRemaining(), 
                               BoundSkillComp->GetSkillCooldownRatio());
    }
}

void ULSHUDWidget::HandleHealthChanged(float CurrentHealth, float MaxHealth, bool bIsDamage)
{
    OnHealthUpdated(CurrentHealth, MaxHealth);
    if (MaxHealth > 0.0f && (CurrentHealth / MaxHealth) > 0.2f)
    {
        OnLowHealthWarning(false);
    }
}

void ULSHUDWidget::HandleLowHealth(float CurrentHealth, float MaxHealth)
{
    OnLowHealthWarning(true);
}

void ULSHUDWidget::HandleStaminaChanged(float CurrentStamina, float MaxStamina)
{
    const float Ratio = (MaxStamina > 0.0f) ? (CurrentStamina / MaxStamina) : 0.0f;
    OnStaminaUpdated(CurrentStamina, MaxStamina, Ratio);
}

void ULSHUDWidget::HandleSkillCooldownChanged(float CurrentCooldown, float MaxCooldown)
{
    const float Ratio = (MaxCooldown > 0.0f) ? (CurrentCooldown / MaxCooldown) : 0.0f;
    OnSkillCooldownUpdated(CurrentCooldown, MaxCooldown, Ratio);
}

void ULSHUDWidget::HandleEnergyChanged(float CurrentEnergy, float MaxEnergy)
{
    const float Ratio = (MaxEnergy > 0.0f) ? (CurrentEnergy / MaxEnergy) : 0.0f;
    OnBurstEnergyUpdated(CurrentEnergy, MaxEnergy, Ratio);
}

void ULSHUDWidget::HandleGlobalRaidWiped()
{
    OnRaidWipedTriggered();
}