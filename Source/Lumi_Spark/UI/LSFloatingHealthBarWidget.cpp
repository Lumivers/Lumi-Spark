#include "UI/LSFloatingHealthBarWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void ULSFloatingHealthBarWidget::UpdateHealth(float CurrentHealth, float MaxHealth)
{
	const float Percent = MaxHealth > 0.0f ? FMath::Clamp(CurrentHealth / MaxHealth, 0.0f, 1.0f) : 0.0f;

	if (HealthProgressBar)
	{
		HealthProgressBar->SetPercent(Percent);
		// 动态血量色彩（健康青绿 -> 半血暖黄 -> 濒死暗红）
		const FLinearColor BarColor = FLinearColor::LerpUsingHSV(FLinearColor(1.0f, 0.2f, 0.2f), FLinearColor(0.2f, 1.0f, 0.4f), Percent);
		HealthProgressBar->SetFillColorAndOpacity(BarColor);
	}

	if (HealthText)
	{
		HealthText->SetText(FText::FromString(FString::Printf(TEXT("%.0f / %.0f"), CurrentHealth, MaxHealth)));
	}

	OnHealthUpdated(CurrentHealth, MaxHealth, Percent);
}
