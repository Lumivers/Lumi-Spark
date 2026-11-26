#include "UI/LSFloatingHealthBarWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateColorBrush.h"
#include "Components/Border.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Styling/SlateTypes.h"

TSharedRef<SWidget> ULSFloatingHealthBarWidget::RebuildWidget()
{
	// 若挂载的蓝图或纯 C++ 类没有现成 UMG 控件树，则在 C++ 内自动拼装经典绿血条
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		AutoBorderContainer = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName("Auto_HealthBarBorder"));
		if (AutoBorderContainer)
		{
			AutoBorderContainer->SetPadding(FMargin(1.0f));
			AutoBorderContainer->SetBrushColor(BorderOutlineColor);

			if (!HealthProgressBar)
			{
				HealthProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), FName("Auto_HealthProgressBar"));
			}

			if (HealthProgressBar)
			{
				// 使用纯白 FillImage 搭配 SetFillColorAndOpacity，避免受引擎默认灰蓝样式偏色影响
				FProgressBarStyle BarStyle = FProgressBarStyle::GetDefault();
				BarStyle.SetBackgroundImage(FSlateColorBrush(TrackBackgroundColor));
				BarStyle.SetFillImage(FSlateColorBrush(FLinearColor::White));
				BarStyle.EnableFillAnimation = false;

				HealthProgressBar->SetWidgetStyle(BarStyle);
				HealthProgressBar->SetPercent(CachedHealthPercent);
				const FLinearColor InitColor = bKeepConstantGreenColor
					? ClassicGreenColor
					: FLinearColor::LerpUsingHSV(FLinearColor(1.0f, 0.2f, 0.2f), ClassicGreenColor, CachedHealthPercent);
				HealthProgressBar->SetFillColorAndOpacity(InitColor);

				AutoBorderContainer->SetContent(HealthProgressBar);
			}

			WidgetTree->RootWidget = AutoBorderContainer;
		}
	}

	return Super::RebuildWidget();
}

void ULSFloatingHealthBarWidget::UpdateHealth(float CurrentHealth, float MaxHealth)
{
	const float Percent = MaxHealth > 0.0f ? FMath::Clamp(CurrentHealth / MaxHealth, 0.0f, 1.0f) : 0.0f;
	CachedHealthPercent = Percent;
	CachedCurrentHealth = CurrentHealth;
	CachedMaxHealth = MaxHealth;

	if (HealthProgressBar)
	{
		HealthProgressBar->SetPercent(Percent);
		// 默认保持经典绿色；若关闭常绿选项，则随血量降低由绿过渡至红
		const FLinearColor BarColor = bKeepConstantGreenColor
			? ClassicGreenColor
			: FLinearColor::LerpUsingHSV(FLinearColor(1.0f, 0.2f, 0.2f), ClassicGreenColor, Percent);
		HealthProgressBar->SetFillColorAndOpacity(BarColor);
	}

	if (HealthText)
	{
		HealthText->SetText(FText::FromString(FString::Printf(TEXT("%.0f / %.0f"), CurrentHealth, MaxHealth)));
	}

	OnHealthUpdated(CurrentHealth, MaxHealth, Percent);
}
