#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Core/LSTypes.h"
#include "LSElementalField.generated.h"

class USphereComponent;

/**
 * 残留元素领域实体 (ALSElementalField)
 * 手雷爆炸后在地面生成并持续存在 3.5s，定时器周期性对进入范围的目标施加元素附着与特色效果
 */
UCLASS()
class LUMI_SPARK_API ALSElementalField : public AActor
{
	GENERATED_BODY()

public:
	ALSElementalField();

	virtual void BeginPlay() override;

	// 初始化领域属性（由手雷在爆炸生成时注入）
	void InitializeField(FGameplayTag InElementTag, AActor* InInstigator, float InDuration = 3.5f, float InRadius = 350.0f);

protected:
	// 定时器周期结算（0.5s 节拍，避免每帧 Tick）
	UFUNCTION()
	void HandlePeriodicTick();

	FColor GetFieldDebugColor() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> FieldSphere;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Field")
	float Duration = 3.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Field")
	float FieldRadius = 350.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Field")
	float TickInterval = 0.5f;

	// 领域所属元素 Tag
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Field")
	FGameplayTag ElementTag;

	// 火海单次 DoT 伤害
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Field|Pyro")
	float FireDoTDamage = 15.0f;

	// 冰霜减速乘率 (0.6 代表降低 40% 移动速度)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Field|Cryo")
	float IceSlowMultiplier = 0.6f;

	// 领域施加者
	UPROPERTY()
	TObjectPtr<AActor> FieldInstigator = nullptr;

private:
	FTimerHandle PeriodicTimerHandle;
};