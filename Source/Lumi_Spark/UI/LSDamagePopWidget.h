#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "Core/LSTypes.h"
#include "LSDamagePopWidget.generated.h"

/**
 * 单条飘字渲染数据快照
 */
USTRUCT(BlueprintType)
struct LUMI_SPARK_API FLSDamagePopItem
{
	GENERATED_BODY()

	// 伤害数值
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DamagePop")
	float Damage = 0.0f;

	// 当前 3D 世界空间位置（随时间上浮）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DamagePop")
	FVector WorldLocation = FVector::ZeroVector;

	// 投影后的 2D 屏幕坐标
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DamagePop")
	FVector2D ScreenPosition = FVector2D::ZeroVector;

	// 渲染颜色（由元素类型决定）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DamagePop")
	FLinearColor DisplayColor = FLinearColor::White;

	// 是否暴击/爆头（字号放大，带金色泛光）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DamagePop")
	bool bIsCritical = false;

	// 反应名称（如"蒸发"、"超载"，无反应则为空）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DamagePop")
	FText ReactionText;

	// 动态缩放比例 (Pop-Scale 弹性动画)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DamagePop")
	float CurrentScale = 1.0f;

	// 不透明度 (1.0 -> 0.0 渐隐)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DamagePop")
	float CurrentAlpha = 1.0f;

	// 剩余存活时间
	float RemainingLifeTime = 0.8f;
	float TotalLifeTime = 0.8f;
	FVector Velocity = FVector::ZeroVector;
};

/**
 * 战斗伤害飘字中枢界面
 * 监听全局事件总线 OnDamageDealt 与 OnElementReactionTriggered，
 * 负责世界坐标投影、随机散布防重叠与驱动 UMG 渲染
 */
UCLASS(Abstract)
class LUMI_SPARK_API ULSDamagePopWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// 获取当前场景所有活跃的飘字数据列表（供 UMG 蓝图循环绘制）
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DamagePop")
	const TArray<FLSDamagePopItem>& GetActivePopItems() const { return ActivePopItems; }

protected:
	// 蓝图表现层事件：当新飘字诞生时调用（供蓝图播放音效或特化特效）
	UFUNCTION(BlueprintImplementableEvent, Category = "DamagePop")
	void OnPopItemCreated(const FLSDamagePopItem& NewItem);

	// 飘字存活时间（默认 0.85 秒）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DamagePop|Config")
	float PopLifeTime = 0.85f;

	// 暴击字号缩放倍率
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DamagePop|Config")
	float CritScaleMultiplier = 1.4f;

	// 上浮初速度 (单位：cm/s)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DamagePop|Config")
	float FloatSpeed = 120.0f;

private:
	// 活跃飘字池
	UPROPERTY()
	TArray<FLSDamagePopItem> ActivePopItems;

	// 全局事件总线回调
	UFUNCTION()
	void HandleDamageDealt(const FLSDamageContext& DamageContext);

	UFUNCTION()
	void HandleReactionTriggered(AActor* Target, FGameplayTag ReactionTag, float ReactionDamage, AActor* Instigator);

	// 内部生成新飘字
	void SpawnPopNumber(float Damage, const FVector& HitLocation, const FGameplayTag& ElementTag, bool bIsCritical, const FGameplayTag& ReactionTag);

	// 元素色相查找表
	FLinearColor GetColorForElement(const FGameplayTag& ElementTag) const;

	// 反应名称本地化字典
	FText GetReactionNameText(const FGameplayTag& ReactionTag) const;
};