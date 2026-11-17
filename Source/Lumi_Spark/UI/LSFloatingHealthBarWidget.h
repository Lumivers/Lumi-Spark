#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LSFloatingHealthBarWidget.generated.h"

class UProgressBar;
class UTextBlock;

/**
 * 敌人头顶浮动血条控件基类 (ULSFloatingHealthBarWidget)
 * 挂载在 Enemy 的 UWidgetComponent 上，支持蓝图 UMG 绑定或纯 C++ 调用
 */
UCLASS(Blueprintable)
class LUMI_SPARK_API ULSFloatingHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 刷新血量百分比与数值
	UFUNCTION(BlueprintCallable, Category = "HealthBar")
	void UpdateHealth(float CurrentHealth, float MaxHealth);

	// 蓝图表现层事件：供 UMG 蓝图扩展伤害闪烁、渐变动效
	UFUNCTION(BlueprintImplementableEvent, Category = "HealthBar")
	void OnHealthUpdated(float CurrentHealth, float MaxHealth, float HealthPercent);

protected:
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HealthBar")
	TObjectPtr<UProgressBar> HealthProgressBar;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HealthBar")
	TObjectPtr<UTextBlock> HealthText;
};
