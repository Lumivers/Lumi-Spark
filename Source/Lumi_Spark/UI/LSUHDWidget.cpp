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
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Extraction/LSExtractionTypes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Rendering/DrawElements.h"

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
	SetupExtractionHUD();

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

	// 3. 基础静止间距与连射热度扩散（依据当前武器准星形态切换经典十字或霰弹圆形半径）：
	const bool bIsShotgunCircle = (CurrentBoundWeapon && CurrentBoundWeapon->GetCrosshairStyle() == ELSCrosshairStyle::ShotgunCircle);
	const float ActiveHipBase = bIsShotgunCircle ? ShotgunHipfireBaseRadius : HipfireBaseSpreadGap;
	const float ActiveHipMax = bIsShotgunCircle ? ShotgunHipfireMaxRadius : HipfireMaxSpreadGap;
	const float ActiveADSBase = bIsShotgunCircle ? ShotgunADSBaseRadius : ADSBaseSpreadGap;
	const float ActiveADSMax = bIsShotgunCircle ? ShotgunADSMaxRadius : ADSMaxSpreadGap;

	const float BaseMin = bIsADS ? ActiveADSBase : ActiveHipBase;
	const float BaseMax = bIsADS ? ActiveADSMax : ActiveHipMax;
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
		// 对霰弹等宽环准星将命中标记收紧在中心安全半径内，保证十字/圆环均可清晰反馈命中
		const float HitMarkerRadius = FMath::Min(CurrentSpreadGap, 26.0f);
		const float DiagOffset = HitMarkerRadius * 0.70710678f;

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

	// 9. 更新所有存活的限量伤害跳字位置与渐隐
	UpdateFloatingDamagePops(InDeltaTime);
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

		// 依据新换上的武器准星形态，自适应调整四向线条与中心点可见性（霰弹圆环由 NativePaint 绘制，隐藏四向直条）
		const ELSCrosshairStyle Style = CurrentBoundWeapon->GetCrosshairStyle();
		const bool bShowCrossLines = bEnableProceduralCrosshair && (Style == ELSCrosshairStyle::ClassicCross);
		const bool bShowDot = bEnableProceduralCrosshair && (
			(Style == ELSCrosshairStyle::DotOnly) ||
			(Style == ELSCrosshairStyle::ShotgunCircle && bShowShotgunCenterDot) ||
			(Style == ELSCrosshairStyle::ClassicCross && bShowCenterDot));

		const ESlateVisibility LinesVis = bShowCrossLines ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
		const ESlateVisibility DotVis = bShowDot ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;

		if (Crosshair_Top) Crosshair_Top->SetVisibility(LinesVis);
		if (Crosshair_Bottom) Crosshair_Bottom->SetVisibility(LinesVis);
		if (Crosshair_Left) Crosshair_Left->SetVisibility(LinesVis);
		if (Crosshair_Right) Crosshair_Right->SetVisibility(LinesVis);
		if (Crosshair_CenterDot) Crosshair_CenterDot->SetVisibility(DotVis);

		// 切枪时立即同步准星初始半径，避免从步枪切霰弹时圆环从小点慢悠悠放大
		const bool bIsShotgunCircle = (Style == ELSCrosshairStyle::ShotgunCircle);
		bool bIsADS = false;
		if (BoundCharacter && BoundCharacter->GetCameraComponent())
		{
			bIsADS = BoundCharacter->GetCameraComponent()->IsADS();
		}
		CurrentSpreadGap = bIsShotgunCircle
			? (bIsADS ? ShotgunADSBaseRadius : ShotgunHipfireBaseRadius)
			: (bIsADS ? ADSBaseSpreadGap : HipfireBaseSpreadGap);
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

	if (UWorld* World = GetWorld())
	{
		const double Now = World->GetTimeSeconds();
		if (LastHitSoundTime < 0.0 || (Now - LastHitSoundTime) >= static_cast<double>(HitSoundCooldown))
		{
			LastHitSoundTime = Now;
			USoundBase* SoundToPlay = DamageContext.bIsHeadshot ? HitHeadshotSound : HitNormalSound;
			if (SoundToPlay)
			{
				UGameplayStatics::PlaySound2D(this, SoundToPlay);
			}
		}
	}

	TriggerHitMarker(DamageContext.bIsHeadshot);
	SpawnFloatingDamagePop(DamageContext);
}

int32 ULSHUDWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	int32 MaxLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	if (bEnableProceduralCrosshair && CurrentBoundWeapon && CurrentBoundWeapon->GetCrosshairStyle() == ELSCrosshairStyle::ShotgunCircle)
	{
		const FVector2f Center = FVector2f(AllottedGeometry.GetLocalSize()) * 0.5f;
		const float Radius = FMath::Max(6.0f, CurrentSpreadGap);
		const int32 Segments = FMath::Max(4, ShotgunCircleSegmentsPerArc);
		const float HalfSpanDeg = FMath::Max(5.0f, 45.0f - (ShotgunCircleArcGapDegrees * 0.5f));

		// 在 0° / 90° / 180° / 270° 四个象限绘制四段圆弧，在 45° 对角线处留白以配合 HitMarker 闪烁
		static const float QuadrantCentersDeg[4] = { 0.0f, 90.0f, 180.0f, 270.0f };
		for (int32 ArcIdx = 0; ArcIdx < 4; ++ArcIdx)
		{
			const float StartDeg = QuadrantCentersDeg[ArcIdx] - HalfSpanDeg;
			const float EndDeg = QuadrantCentersDeg[ArcIdx] + HalfSpanDeg;

			TArray<FVector2f> ArcPoints;
			ArcPoints.Reserve(Segments + 1);
			for (int32 Seg = 0; Seg <= Segments; ++Seg)
			{
				const float Alpha = static_cast<float>(Seg) / static_cast<float>(Segments);
				const float AngleRad = FMath::DegreesToRadians(FMath::Lerp(StartDeg, EndDeg, Alpha));
				ArcPoints.Emplace(Center.X + FMath::Cos(AngleRad) * Radius, Center.Y + FMath::Sin(AngleRad) * Radius);
			}

			FSlateDrawElement::MakeLines(
				OutDrawElements,
				MaxLayer + 1,
				AllottedGeometry.ToPaintGeometry(),
				MoveTemp(ArcPoints),
				ESlateDrawEffect::None,
				ShotgunCircleColor,
				true,
				ShotgunCircleThickness
			);
		}
		MaxLayer += 1;
	}

	return MaxLayer;
}

void ULSHUDWidget::SetupExtractionHUD()
{
	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(GetRootWidget());
	if (!RootCanvas)
	{
		return;
	}

	if (!ExtractionTitleText)
	{
		ExtractionTitleText = NewObject<UTextBlock>(this, FName("Auto_Extraction_Title"));
		if (ExtractionTitleText)
		{
			ExtractionTitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 1.0f, 0.92f, 1.0f)));
			ExtractionTitleText->SetJustification(ETextJustify::Center);
			ExtractionTitleText->SetShadowOffset(FVector2D(1.5f, 1.5f));
			ExtractionTitleText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f));
			FSlateFontInfo FontInfo = ExtractionTitleText->GetFont();
			FontInfo.Size = 18;
			ExtractionTitleText->SetFont(FontInfo);
			ExtractionTitleText->SetVisibility(ESlateVisibility::Collapsed);

			if (UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(ExtractionTitleText))
			{
				PanelSlot->SetAnchors(FAnchors(0.5f, 0.0f, 0.5f, 0.0f));
				PanelSlot->SetAlignment(FVector2D(0.5f, 0.0f));
				PanelSlot->SetPosition(FVector2D(0.0f, 52.0f));
				PanelSlot->SetAutoSize(true);
				PanelSlot->SetZOrder(10);
			}
		}
	}

	if (!ExtractionProgressBar)
	{
		ExtractionProgressBar = NewObject<UProgressBar>(this, FName("Auto_Extraction_Bar"));
		if (ExtractionProgressBar)
		{
			ExtractionProgressBar->SetFillColorAndOpacity(FLinearColor(0.18f, 0.92f, 0.68f, 1.0f));
			ExtractionProgressBar->SetPercent(0.0f);
			ExtractionProgressBar->SetVisibility(ESlateVisibility::Collapsed);

			if (UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(ExtractionProgressBar))
			{
				PanelSlot->SetAnchors(FAnchors(0.5f, 0.0f, 0.5f, 0.0f));
				PanelSlot->SetAlignment(FVector2D(0.5f, 0.0f));
				PanelSlot->SetPosition(FVector2D(0.0f, 84.0f));
				PanelSlot->SetSize(FVector2D(340.0f, 10.0f));
				PanelSlot->SetZOrder(10);
			}
		}
	}

	if (!ExtractionResultText)
	{
		ExtractionResultText = NewObject<UTextBlock>(this, FName("Auto_Extraction_Result"));
		if (ExtractionResultText)
		{
			ExtractionResultText->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 1.0f, 0.65f, 1.0f)));
			ExtractionResultText->SetJustification(ETextJustify::Center);
			ExtractionResultText->SetShadowOffset(FVector2D(2.0f, 2.0f));
			ExtractionResultText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f));
			FSlateFontInfo FontInfo = ExtractionResultText->GetFont();
			FontInfo.Size = 28;
			ExtractionResultText->SetFont(FontInfo);
			ExtractionResultText->SetVisibility(ESlateVisibility::Collapsed);

			if (UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(ExtractionResultText))
			{
				PanelSlot->SetAnchors(FAnchors(0.5f, 0.32f, 0.5f, 0.32f));
				PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
				PanelSlot->SetPosition(FVector2D::ZeroVector);
				PanelSlot->SetAutoSize(true);
				PanelSlot->SetZOrder(15);
			}
		}
	}
}

void ULSHUDWidget::UpdateExtractionProgress(const FText& PointName, float Progress, float RemainingSeconds)
{
	if (!ExtractionTitleText || !ExtractionProgressBar)
	{
		SetupExtractionHUD();
	}

	if (!PointName.IsEmpty())
	{
		CachedExtractionPointName = PointName;
	}

	if (ExtractionTitleText)
	{
		const FString Label = CachedExtractionPointName.IsEmpty()
			? FString::Printf(TEXT("正在撤离  %.1fs"), RemainingSeconds)
			: FString::Printf(TEXT("正在撤离 [%s]  %.1fs"), *CachedExtractionPointName.ToString(), RemainingSeconds);
		ExtractionTitleText->SetText(FText::FromString(Label));
		ExtractionTitleText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (ExtractionProgressBar)
	{
		ExtractionProgressBar->SetPercent(FMath::Clamp(Progress, 0.0f, 1.0f));
		ExtractionProgressBar->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void ULSHUDWidget::HideExtractionProgress()
{
	if (ExtractionTitleText)
	{
		ExtractionTitleText->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (ExtractionProgressBar)
	{
		ExtractionProgressBar->SetVisibility(ESlateVisibility::Collapsed);
		ExtractionProgressBar->SetPercent(0.0f);
	}
}

void ULSHUDWidget::ShowExtractionSuccess(const FLSRaidReport& Report)
{
	HideExtractionProgress();
	if (!ExtractionResultText)
	{
		SetupExtractionHUD();
	}

	if (ExtractionResultText)
	{
		const FString Msg = FString::Printf(TEXT("撤离成功  EXTRACTION COMPLETE\n带出物资: %d 件 | 总估值: %.0f"),
			Report.ExtractedItemCount, Report.TotalExtractedValue);
		ExtractionResultText->SetText(FText::FromString(Msg));
		ExtractionResultText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
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

void ULSHUDWidget::SpawnFloatingDamagePop(const FLSDamageContext& DamageContext)
{
	if (!bEnableFloatingDamagePop || DamageContext.FinalDamage <= 0.0f)
	{
		return;
	}

	FVector HitWorldLoc = FVector::ZeroVector;
	if (!DamageContext.HitResult.ImpactPoint.IsNearlyZero())
	{
		HitWorldLoc = DamageContext.HitResult.ImpactPoint;
	}
	else if (!DamageContext.HitResult.Location.IsNearlyZero())
	{
		HitWorldLoc = DamageContext.HitResult.Location;
	}
	else if (DamageContext.TargetActor)
	{
		HitWorldLoc = DamageContext.TargetActor->GetActorLocation() + FVector(0.0f, 0.0f, 80.0f);
	}
	else
	{
		return;
	}

	const float JitterXY = FMath::Max(0.0f, DamagePopJitterRadius.X);
	const float JitterZ = FMath::Max(0.0f, DamagePopJitterRadius.Y);
	HitWorldLoc += FVector(
		FMath::FRandRange(-JitterXY, JitterXY),
		FMath::FRandRange(-JitterXY, JitterXY),
		16.0f + FMath::FRandRange(0.0f, JitterZ)
	);

	// 严格维持同屏数量上限：超出时按生成顺序淘汰最早一条（队列头部 index 0）
	const int32 MaxCount = FMath::Max(1, MaxDamagePopCount);
	while (ActiveDamagePops.Num() >= MaxCount)
	{
		RecycleDamagePopEntry(ActiveDamagePops[0]);
		ActiveDamagePops.RemoveAt(0);
	}

	UTextBlock* TextWidget = AcquireDamagePopTextBlock();
	if (!TextWidget)
	{
		return;
	}

	const bool bIsCrit = DamageContext.bIsHeadshot || DamageContext.bIsCritical;
	const int32 RoundedDamage = FMath::Max(1, FMath::RoundToInt(DamageContext.FinalDamage));
	const FString PopText = bIsCrit
		? FString::Printf(TEXT("%d!"), RoundedDamage)
		: FString::Printf(TEXT("%d"), RoundedDamage);

	FSlateFontInfo FontInfo = TextWidget->GetFont();
	FontInfo.Size = bIsCrit ? DamagePopCritFontSize : DamagePopNormalFontSize;
	TextWidget->SetFont(FontInfo);
	TextWidget->SetText(FText::FromString(PopText));

	const FLinearColor BaseColor = bIsCrit ? DamagePopCritColor : DamagePopNormalColor;
	TextWidget->SetColorAndOpacity(FSlateColor(BaseColor));

	FLSHUDDamagePopEntry NewEntry;
	NewEntry.TextWidget = TextWidget;
	NewEntry.WorldLocation = HitWorldLoc;
	NewEntry.BaseColor = BaseColor;
	NewEntry.TotalLifeTime = FMath::Max(0.15f, DamagePopLifeTime);
	NewEntry.RemainingTime = NewEntry.TotalLifeTime;

	// 生成当帧立即投影定位，防止控件在新一帧前先在左上角 (0, 0) 闪烁一帧
	UpdateDamagePopScreenTransform(NewEntry, 1.0f);
	ActiveDamagePops.Add(MoveTemp(NewEntry));
}

void ULSHUDWidget::UpdateFloatingDamagePops(float DeltaTime)
{
	if (ActiveDamagePops.IsEmpty())
	{
		return;
	}

	for (int32 Index = ActiveDamagePops.Num() - 1; Index >= 0; --Index)
	{
		FLSHUDDamagePopEntry& Entry = ActiveDamagePops[Index];
		Entry.RemainingTime -= DeltaTime;
		if (Entry.RemainingTime <= 0.0f)
		{
			RecycleDamagePopEntry(Entry);
			ActiveDamagePops.RemoveAt(Index);
			continue;
		}

		Entry.WorldLocation.Z += DamagePopFloatSpeed * DeltaTime;
		const float NormalizedRemaining = Entry.RemainingTime / FMath::Max(0.01f, Entry.TotalLifeTime);
		// 前 65% 存活时间保持满不透明便于读数，最后 35% 平滑淡出
		const float Alpha = NormalizedRemaining < 0.35f ? (NormalizedRemaining / 0.35f) : 1.0f;
		UpdateDamagePopScreenTransform(Entry, Alpha);
	}
}

bool ULSHUDWidget::UpdateDamagePopScreenTransform(const FLSHUDDamagePopEntry& Entry, float Alpha) const
{
	if (!Entry.TextWidget)
	{
		return false;
	}

	APlayerController* OwningPC = GetOwningPlayer();
	if (!OwningPC)
	{
		Entry.TextWidget->SetVisibility(ESlateVisibility::Collapsed);
		return false;
	}

	FVector2D ScreenPos = FVector2D::ZeroVector;
	const bool bOnScreen = UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(OwningPC, Entry.WorldLocation, ScreenPos, true);
	if (!bOnScreen)
	{
		Entry.TextWidget->SetVisibility(ESlateVisibility::Collapsed);
		return false;
	}

	if (UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(Entry.TextWidget->Slot))
	{
		PanelSlot->SetPosition(ScreenPos);
	}

	Entry.TextWidget->SetRenderOpacity(FMath::Clamp(Alpha, 0.0f, 1.0f));
	if (Entry.TextWidget->GetVisibility() != ESlateVisibility::HitTestInvisible)
	{
		Entry.TextWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	return true;
}

UTextBlock* ULSHUDWidget::AcquireDamagePopTextBlock()
{
	while (!DamagePopWidgetPool.IsEmpty())
	{
		UTextBlock* Pooled = DamagePopWidgetPool.Pop();
		if (Pooled)
		{
			return Pooled;
		}
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(GetRootWidget());
	if (!RootCanvas)
	{
		return nullptr;
	}

	UTextBlock* NewText = NewObject<UTextBlock>(this);
	if (!NewText)
	{
		return nullptr;
	}

	NewText->SetJustification(ETextJustify::Center);
	NewText->SetShadowOffset(FVector2D(1.5f, 1.5f));
	NewText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f));
	NewText->SetVisibility(ESlateVisibility::Collapsed);

	if (UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(NewText))
	{
		PanelSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		PanelSlot->SetAutoSize(true);
		PanelSlot->SetZOrder(25);
	}

	return NewText;
}

void ULSHUDWidget::RecycleDamagePopEntry(const FLSHUDDamagePopEntry& Entry)
{
	if (Entry.TextWidget)
	{
		Entry.TextWidget->SetVisibility(ESlateVisibility::Collapsed);
		Entry.TextWidget->SetRenderOpacity(0.0f);
		DamagePopWidgetPool.Add(Entry.TextWidget);
	}
}