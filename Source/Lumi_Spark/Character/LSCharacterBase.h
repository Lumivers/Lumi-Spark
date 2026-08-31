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
class ALSWeaponBase;

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
	
	//默认佩戴的武器类
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<ALSWeaponBase> DefaultWeaponClass;
	
	//当前持有的武器实例
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<ALSWeaponBase> CurrentWeapon;
	
	//获取摄像机组件
	FORCEINLINE ULSCameraComponent* GetCameraComponent() const { return CameraComponent; }
	
	//获取第一人称手臂Mesh
	FORCEINLINE USkeletalMeshComponent* GetFPArmsMesh() const { return FPArmsMesh; }
	
	FORCEINLINE ULSMovementComponent* GetLSMovementComponent() const { return LSMovementComponent; }
	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;
	
	void StartFire();
	
private:
	float AccumulatedPitchRecoil = 0.0f; // 累积的俯仰后坐力值
};
