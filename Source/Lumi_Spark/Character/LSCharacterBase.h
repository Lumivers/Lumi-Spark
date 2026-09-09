// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LSMovementComponent.h"
#include "Core/LSTypes.h"
#include "LSCharacterBase.generated.h"

//前向声明
class ULSCameraComponent;
class ULSMovementComponent;
class USkeletalMeshComponent;
class ULSWeaponComponent;
class ULSElementComponent;
class UInputMappingContext;
class UInputAction;
class ULSWeaponComponent;
struct FInputActionValue;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLSHealthChanged, float, CurrentHealth, float, MaxHealth);

UCLASS()
class LUMI_SPARK_API ALSCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	// 构造函数:传入FObjectInitializer以便替换默认的CharacterMovementComponent为自定义的ULSMovementComponent
	ALSCharacterBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 引擎伤害重写入口
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
	
	//获取摄像机组件
	FORCEINLINE ULSCameraComponent* GetCameraComponent() const { return CameraComponent; }
	
	//获取第一人称手臂Mesh
	FORCEINLINE USkeletalMeshComponent* GetFPArmsMesh() const { return FPArmsMesh; }
	
	// ✅ 正确写法：
	FORCEINLINE ULSMovementComponent* GetLSMovementComponent() const { return Cast<ULSMovementComponent>(GetCharacterMovement()); }
	
	//获取武器组件
	FORCEINLINE ULSWeaponComponent* GetWeaponComponent() const { return WeaponComponent; }

	// 元素属性
	FORCEINLINE ULSElementComponent* GetElementComponent() const { return ElementComponent; }

	//生命值委托
	UPROPERTY(BlueprintAssignable, Category = "Health|Events")
	FOnLSHealthChanged OnHealthChanged;

protected:
    // 摄像机组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULSCameraComponent> CameraComponent;
	
	// 第一人称手臂Mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> FPArmsMesh;
	
	//自定义移动组件引用
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULSMovementComponent> LSMovementComponent;
	
	//武器组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULSWeaponComponent> WeaponComponent;

	//元素组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Element", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULSElementComponent> ElementComponent;

	// 生命值属性
	UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth, VisibleInstanceOnly, BlueprintReadOnly, Category = "Health")
	float CurrentHealth = 1000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health")
	float MaxHealth = 1000.0f;

	UFUNCTION()
	void OnRep_CurrentHealth();

	// 死亡处理流程
	virtual void Die(AActor* Killer);
};
