// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LSCharacterBase.generated.h"

//前向声明
class ULSCameraComponent;
class ULSMovementComponent;
class USkeletalMeshComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

UCLASS()
class LUMI_SPARK_API ALSCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	// 构造函数:传入FObjectInitializer以便替换默认的CharacterMovementComponent为自定义的ULSMovementComponent
	ALSCharacterBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	// 摄像机组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULSCameraComponent> CameraComponent;
	
	// 第一人称手臂Mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> FPArmsMesh;
	
	//自定义移动组件引用
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULSMovementComponent> LSMovementComponent;
	
	// Enhanced Input 配置
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Context")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Action")
	TObjectPtr<UInputAction> MoveAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Action")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Action")
	TObjectPtr<UInputAction> JumpAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Action")
	TObjectPtr<UInputAction> SprintAction;//冲刺输入
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Action")
	TObjectPtr<UInputAction> DashAction;//闪避输入
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Action")
	TObjectPtr<UInputAction> CrouchAction;//下蹲/滑铲输入
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Action")
	TObjectPtr<UInputAction> ToggleCameraModeAction;//切换视角输入
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Action")
	TObjectPtr<UInputAction> ADSAction;//开镜/过肩瞄准输入

	//获取摄像机组件
	FORCEINLINE ULSCameraComponent* GetCameraComponent() const { return CameraComponent; }
	
	//获取第一人称手臂Mesh
	FORCEINLINE USkeletalMeshComponent* GetFPArmsMesh() const { return FPArmsMesh; }
	
	FORCEINLINE ULSMovementComponent* GetLSMovementComponent() const { return LSMovementComponent; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	//输入处理函数
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void HandleToggleCameraMode(const FInputActionValue& Value);
	
	//瞄准输入响应
	void StartADS(const FInputActionValue& Value);
	void StopADS(const FInputActionValue& Value);
	
	//增强机动性输入响应
	void StartSprint(const FInputActionValue& Value);
	void StopSprint(const FInputActionValue& Value);
	void HandleDash(const FInputActionValue& Value);
	void StartCrouchOrSlide(const FInputActionValue& Value);
	void StopCrouchOrSlide(const FInputActionValue& Value);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
