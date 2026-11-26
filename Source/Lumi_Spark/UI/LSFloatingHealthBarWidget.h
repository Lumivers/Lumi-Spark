#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LSFloatingHealthBarWidget.generated.h"

class UBorder;
class UProgressBar;
class UTextBlock;

/**
 * 敌人头顶浮动血条控件基类 (ULSFloatingHealthBarWidget)
 * 挂载在 Enemy 的 UWidgetComponent 上，支持蓝图 UMG 绑定；
 * 若未绑定蓝图子类控件树，则在 C++ 中自动构建经典绿色血条表现
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
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HealthBar")
	TObjectPtr<UProgressBar> HealthProgressBar;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HealthBar")
	TObjectPtr<UTextBlock> HealthText;

	// 是否保持恒定经典绿条（关闭则随血量降低由绿渐变为红）
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HealthBar|Style")
	bool bKeepConstantGreenColor = true;

	// 经典敌人头顶绿血条填充色
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HealthBar|Style")
	FLinearColor ClassicGreenColor = FLinearColor(0.16f, 0.88f, 0.34f, 1.0f);

	// 血条底槽深色背景
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HealthBar|Style")
	FLinearColor TrackBackgroundColor = FLinearColor(0.06f, 0.06f, 0.08f, 0.85f);

	// 血条外框描边颜色
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HealthBar|Style")
	FLinearColor BorderOutlineColor = FLinearColor(0.02f, 0.02f, 0.03f, 0.95f);

private:
	// 纯 C++ 回退外框（仅当未提供蓝图控件树时构建）
	UPROPERTY(Transient)
	TObjectPtr<UBorder> AutoBorderContainer = nullptr;

	// 缓存最近一次百分比与文本，确保在 RebuildWidget() 尚未执行时先调用 UpdateHealth 也不会丢失初值
	float CachedHealthPercent = 1.0f;
	float CachedCurrentHealth = 0.0f;
	float CachedMaxHealth = 0.0f;
};
