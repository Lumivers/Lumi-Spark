#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "LSThrowableComponent.generated.h"

class ALSGrenadeBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLSGrenadeCountChanged, int32, CurrentCount, int32, MaxCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnLSThrowTrajectoryUpdated, const TArray<FVector>&, PathPoints, const FHitResult&, HitResult, bool, bHitValid);

/**
 * 战术投掷物管理组件 (ULSThrowableComponent)
 * 挂载于角色，管理手雷库存与冷却，长按 [G] 运行抛物线预测，松开调用服务端权威投掷 RPC
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LUMI_SPARK_API ULSThrowableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSThrowableComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ─── 预瞄与投掷控制 ───
	UFUNCTION(BlueprintCallable, Category = "Throwable")
	void StartAimingThrow();

	UFUNCTION(BlueprintCallable, Category = "Throwable")
	void StopAimingThrow(bool bCancel = false);

	UFUNCTION(BlueprintCallable, Category = "Throwable")
	bool CanThrow() const;

	// 战利品补给手雷
	UFUNCTION(BlueprintCallable, Category = "Throwable")
	void AddGrenades(int32 Amount);

	// 属性查询
	UFUNCTION(BlueprintPure, Category = "Throwable")
	int32 GetCurrentGrenadeCount() const { return CurrentGrenadeCount; }

	UFUNCTION(BlueprintPure, Category = "Throwable")
	int32 GetMaxGrenadeCount() const { return MaxGrenadeCount; }

	UFUNCTION(BlueprintPure, Category = "Throwable")
	bool IsAimingThrow() const { return bIsAimingThrow; }

	// ─── 表现广播 ───
	UPROPERTY(BlueprintAssignable, Category = "Throwable|Events")
	FOnLSGrenadeCountChanged OnGrenadeCountChanged;

	UPROPERTY(BlueprintAssignable, Category = "Throwable|Events")
	FOnLSThrowTrajectoryUpdated OnThrowTrajectoryUpdated;

protected:
	// 服务端权威生成投掷物实体并扣除库存
	UFUNCTION(Server, Reliable)
	void Server_ThrowGrenade(FVector LaunchOrigin, FVector LaunchVelocity);

	// 实时计算抛物线路径
	void UpdateTrajectoryPrediction();

	// 获取起点与初速度向量
	bool GetLaunchOriginAndVelocity(FVector& OutOrigin, FVector& OutVelocity) const;

	// 动态解析出战角色对应的元素手雷类
	TSubclassOf<ALSGrenadeBase> ResolveGrenadeClass() const;

	UFUNCTION()
	void OnRep_CurrentGrenadeCount();

protected:
	// 当前手雷数量（权威同步）
	UPROPERTY(ReplicatedUsing = OnRep_CurrentGrenadeCount, VisibleInstanceOnly, BlueprintReadOnly, Category = "Throwable")
	int32 CurrentGrenadeCount = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwable", meta = (ClampMin = "1"))
	int32 MaxGrenadeCount = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwable", meta = (ClampMin = "0.2"))
	float ThrowCooldown = 1.0f;

	// 投掷力道初速度大小 (cm/s)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwable")
	float ThrowForce = 1600.0f;

	// 默认手雷类兜底
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwable")
	TSubclassOf<ALSGrenadeBase> DefaultGrenadeClass;

	// 轨迹模拟时长上限（秒）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwable|Prediction")
	float MaxSimTime = 3.0f;

	// 碰撞检测通道
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwable|Prediction")
	TEnumAsByte<ECollisionChannel> PredictionTraceChannel = ECC_Visibility;

private:
	bool bIsAimingThrow = false;
	float LastThrowTime = -10.0f;
};