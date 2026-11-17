// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "LSCameraComponent.generated.h"

class USpringArmComponent;

/** 第三人称视角模式枚举 */
UENUM(BlueprintType)
enum class ELSCameraMode : uint8
{
	ThirdPerson     UMETA(DisplayName = "Third Person"),    // 第三人称自由漫游（默认）
	OverShoulder    UMETA(DisplayName = "Over Shoulder")    // 过肩瞄准 (ADS)
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LUMI_SPARK_API ULSCameraComponent : public UCameraComponent
{
	GENERATED_BODY()
	
public:
	ULSCameraComponent();
	
	// ═══ 当前视角模式 ═══
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Mode")
	ELSCameraMode CurrentMode = ELSCameraMode::ThirdPerson;
	
	// ═══ 第三人称自由视角参数配置 ═══
	UPROPERTY(EditDefaultsOnly, Category = "Camera|TPS")
	float TPSArmLength = 300.f; // 第三人称弹簧臂距离
	
	UPROPERTY(EditDefaultsOnly, Category = "Camera|TPS")
	float TPSFov = 80.f;
	
	// ═══ 过肩瞄准参数配置 ═══
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Shoulder")
	FVector ShoulderOffset = FVector(0.f, 60.f, 60.f); // 右肩偏移
	
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Shoulder")
	float ShoulderFov = 65.f; // 开镜/过肩收窄 FOV
	
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Transition")
	float TransitionSpeed = 12.f; // 视角切换平滑插值速率
	
	// ═══ 垂直高程阻尼与死区滤波配置 ═══
	/** 是否开启垂直高程延后平滑（解决翻墙攀爬镜头瞬间冲天脱节） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera|Smoothing")
	bool bEnableVerticalDamping = true;

	/** 垂直滤波死区半高（cm，小于此阈值的奔跑颠簸/碎石微小起伏将被完全过滤，防晕防震） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera|Smoothing", meta = (ClampMin = "0.0", ClampMax = "25.0"))
	float VerticalDeadZone = 7.0f;

	/** 爬墙/上升时的延后平滑速率（推荐 3.5~5.0，速度越慢越延后，营造动作优先的从容呼吸感） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera|Smoothing", meta = (ClampMin = "1.0", ClampMax = "20.0"))
	float UpwardSmoothingSpeed = 4.5f;

	/** 下落/降落时的平滑速率（推荐 7.0~10.0，快速跟进防止镜头悬空过久） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera|Smoothing", meta = (ClampMin = "1.0", ClampMax = "20.0"))
	float DownwardSmoothingSpeed = 8.0f;

	/** 最大允许的高度滞后距离（cm，默认 60cm，防止镜头落后过多或触地穿模） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera|Smoothing", meta = (ClampMin = "20.0", ClampMax = "120.0"))
	float MaxVerticalLagDistance = 60.0f;

	/** 是否开启弹簧臂水平位移弹性滞后（消除机械硬拉扯） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera|Smoothing")
	bool bEnableCameraLag = true;

	/** 水平位移滞后速率（推荐 12~16，既灵敏跟手又具备影视级平滑质感） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera|Smoothing", meta = (ClampMin = "1.0"))
	float CameraLagSpeed = 14.0f;

	// ═══ 控制接口（供 Controller / Character 调用） ═══
	UFUNCTION(BlueprintCallable, Category="Camera")
	void SetCameraMode(ELSCameraMode NewMode);

	// 滚轮缩放第三人称视角臂长
	UFUNCTION(BlueprintCallable, Category="Camera")
	void AdjustCameraZoom(float ZoomDelta);

	UPROPERTY(EditDefaultsOnly, Category = "Camera|TPS")
	float MinArmLength = 120.f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|TPS")
	float MaxArmLength = 550.f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|TPS")
	float ZoomStep = 35.f;

	UFUNCTION(BlueprintCallable, Category="Camera")
	void EnterADS(); // 开镜/进入过肩瞄准
	
	UFUNCTION(BlueprintCallable, Category="Camera")
	void ExitADS(); // 退出开镜
	
	UFUNCTION(BlueprintCallable, Category="Camera")
	FORCEINLINE bool IsADS() const { return bIsInADS; }
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
	TObjectPtr<USpringArmComponent> SpringArm;

protected:
	virtual void BeginPlay() override;
	
public:
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
private:
	ELSCameraMode PreADSMode = ELSCameraMode::ThirdPerson;
	float TargetArmLength = 280.0f;
	float CustomTPSArmLength = 280.0f;
	FVector TargetSocketOffset = FVector(0.0f, 50.0f, 65.0f);
	bool bIsInADS = false;
	float TargetFov = 80.f;
	
	// 逐帧更新摄像机平滑插值与防穿墙检测
	void UpdateCameraInterpolation(float DeltaTime);

	// 维护的世界平滑高程目标
	float SmoothedCameraPivotWorldZ = 0.0f;
	bool bHasInitializedWorldZ = false;

	// 动态垂直高程补偿值（当前平滑结果）
	float CurrentVerticalCompensationZ = 0.0f;

	// 缓存的父级角色弱引用
	UPROPERTY(Transient)
	TWeakObjectPtr<class ACharacter> CachedOwnerCharacter;
};
