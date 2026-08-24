// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LSCharacterBase.generated.h"

//前向声明
class ULSCameraComponent;
class USkeletalMeshComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

UCLASS()
class LUMI_SPARK_API ALSCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	// 构造函数
	ALSCharacterBase();
	
	// 摄像机组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULSCameraComponent> CameraComponent;
	
	// 第一人称手臂Mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> FPArmsMesh;
	
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
	TObjectPtr<UInputAction> ToggleCameraModeAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Action")
	TObjectPtr<UInputAction> ADSAction;

	//获取摄像机组件
	FORCEINLINE ULSCameraComponent* GetCameraComponent() const { return CameraComponent; }
	
	//获取第一人称手臂Mesh
	FORCEINLINE USkeletalMeshComponent* GetFPArmsMesh() const { return FPArmsMesh; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	//输入处理函数
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void HandleToggleCameraMode(const FInputActionValue& Value);
	void StartADS(const FInputActionValue& Value);
	void StopADS(const FInputActionValue& Value);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
