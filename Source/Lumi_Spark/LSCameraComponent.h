// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "LSCameraComponent.generated.h"

/** 视角模式枚举 */

UENUM(BlueprintType)
enum class ELSCameraMode : uint8
{
	FirstPerson     UMETA(DisplayName = "First Person"),    // 第一人称（默认）
	ThirdPerson     UMETA(DisplayName = "Third Person"),    // 第三人称
	OverShoulder    UMETA(DisplayName = "Over Shoulder")    // 过肩瞄准
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LUMI_SPARK_API ULSCameraComponent : public UCameraComponent
{
	
	GENERATED_BODY()
	
public:
	ULSCameraComponent();
	
	// ═══ 当前视角模式 ═══
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Mode")
	ELSCameraMode CurrentMode = ELSCameraMode::FirstPerson;
	
	// ═══ 视角参数配置 ═══
	UPROPERTY(EditDefaultsOnly, Category = "Camera|FPS")
	FVector FPSOffset = FVector(0.f, 0.f, 70.f); // 相对角色眼部高度
	
	UPROPERTY(EditDefaultsOnly, Category = "Camera|FPS")
	float FPSFov = 90.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Camera|TPS")
	float TPSArmLength = 300.f; // 第三人称弹簧臂距离
	
	UPROPERTY(EditDefaultsOnly, Category = "Camera|TPS")
	float TPSFov = 80.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Shoulder")
	FVector ShoulderOffset = FVector(0.f, 60.f, 60.f); // 右肩偏移
	
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Shoulder")
	float ShoulderFov = 65.f; // 开镜/过肩收窄 FOV
	
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Transition")
	float TransitionSpeed = 12.f; // 视角切换平滑插值速率
	
	// ═══ 控制接口（供 Controller / Character 调用） ═══
	UFUNCTION(BlueprintCallable, Category="Camera")
	void SetCameraMode(ELSCameraMode NewMode);
	
	UFUNCTION(BlueprintCallable, Category="Camera")
	void ToggleCameraMode();//循环切换一三人称
	
	UFUNCTION(BlueprintCallable, Category="Camera")
	void EnterADS();//开镜/进入过肩瞄准
	
	UFUNCTION(BlueprintCallable, Category="Camera")
	void ExitADS();//退出开镜
	
protected:
	virtual void BeginPlay() override;
	
public:
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
private:
	FVector TargetOffset;
	float TargetFov;
	bool bIsInADS = false;
	
	// 逐帧更新摄像机平滑插值与防穿墙检测
	void UpdateCameraInterpolation(float DeltaTime);
	
	// 根据当前模式更新角色全身Mesh和第一人称手臂的显隐
	void UpdateMeshVisibility();
};