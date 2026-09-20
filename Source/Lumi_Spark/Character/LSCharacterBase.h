// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LSMovementComponent.h"
#include "Core/LSTypes.h"
#include "Core/LSInteractableInterface.h"
#include "LSCharacterBase.generated.h"

//前向声明
class ULSCameraComponent;
class ULSMovementComponent;
class USkeletalMeshComponent;
class ULSWeaponComponent;
class ULSElementComponent;
class ULSSkillComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLSHealthChanged, float, CurrentHealth, float, MaxHealth);

UCLASS()
class LUMI_SPARK_API ALSCharacterBase : public ACharacter, public ILSInteractableInterface
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
	
	// ILSInteractableInterface 接口实现
	virtual bool CanInteract(AActor* Interactor) const override;
	virtual FText GetInteractPrompt(AActor* Interactor) const override;
	virtual float GetInteractDuration(AActor* Interactor) const override;
	virtual void OnInteractComplete(AActor* Interactor) override;
	
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

	//获取技能与大招充能组件
	FORCEINLINE ULSSkillComponent* GetSkillComponent() const { return SkillComponent; }

	//生命值委托
	UPROPERTY(BlueprintAssignable, Category = "Health|Events")
	FOnLSHealthChanged OnHealthChanged;

	//进退场与小队状态
	UFUNCTION(BlueprintCallable, Category = "Team")
	virtual void EnterBackgroundMode(); // 进入后台模式（隐身、关碰撞、轻量更新）

	UFUNCTION(BlueprintCallable, Category = "Team")
	virtual void ExitBackgroundMode();  // 退出后台模式（显形、开碰撞

	UFUNCTION(BlueprintCallable, BluePrintPure, Category = "Health")
	bool IsDead() const { return CurrentHealth <= 0.0f; }
	
	// 倒地与救援生命周期
	UFUNCTION(BlueprintCallable, Category = "Health|Downed")
	bool IsDowned() const { return bIsDowned; }
	
	UFUNCTION(BlueprintCallable, Category = "Health|Downed")
	float GetBleedoutRatio() const { return DownedBleedoutMaxTime > 0.0f ? (BleedoutRemainingTimer / DownedBleedoutMaxTime) : 0.0f; }
	
	// 受击与倒地处理
	UFUNCTION(BlueprintCallable, Category = "Health|Downed")
	virtual void EnterDownedState(AActor* Killer);
	
	// 倒地被救援
	UFUNCTION(BlueprintCallable, Category = "Health|Downed")
	virtual void Revive(AActor* Reviver, float RestoredHealthPercent = 0.5f);

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

	// 技能与大招充能组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skill", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULSSkillComponent> SkillComponent;

	UFUNCTION()
	void OnRep_CurrentHealth();
	
	//倒地状态网络属性同步
	UPROPERTY(ReplicatedUsing = OnRep_IsDowned, VisibleInstanceOnly, BlueprintReadOnly, Category = "Health|Downed")
	bool bIsDowned = false;
	
	//倒地最大流血存活时长（秒）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health|Downed")
	float DownedBleedoutMaxTime = 45.0f;
	
	//倒地匍匐移动速度
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health|Downed")
	float DownedWalkSpeed = 150.0f;
	
	//剩余流血时间
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Health|Downed")
	float BleedoutRemainingTimer = 0.0f;
	
	FTimerHandle BleedoutTimerHandle;
	
	UFUNCTION()
	void OnRep_IsDowned();
	
	void HandleBleedoutTick();

	// 死亡处理流程
	virtual void Die(AActor* Killer);
};
