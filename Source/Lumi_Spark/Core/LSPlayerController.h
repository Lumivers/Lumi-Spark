#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LSPlayerController.generated.h"

//前向声明
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class ALSCharacterBase;

UCLASS()
class LUMI_SPARK_API ALSPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	ALSPlayerController();
	
	//1，Input Mapping Context（输入映射上下文）配置
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Context")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Context")
	TObjectPtr<UInputMappingContext> UIModeMappingContext;
	
	//2，角色核心3c移动与视角
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Action")
	TObjectPtr<UInputAction> IA_Move;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Action")
	TObjectPtr<UInputAction> IA_Look;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Action")
	TObjectPtr<UInputAction> IA_Jump;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Action")
	TObjectPtr<UInputAction> IA_Sprint;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Action")
	TObjectPtr<UInputAction> IA_Dash;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Action")
	TObjectPtr<UInputAction> IA_Crouch;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Action")
	TObjectPtr<UInputAction> IA_ToggleView; //v切换FPS/TPS
	
	//3，武器与射击战斗Input Actions
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Action")
	TObjectPtr<UInputAction> IA_Fire;   // 鼠标左键射击
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Action")
	TObjectPtr<UInputAction> IA_ADS;    // 鼠标右键开镜
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Action")
	TObjectPtr<UInputAction> IA_Reload; // R 换弹
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Action")
	TObjectPtr<UInputAction> IA_SwitchWeapon1;     // 1 键主武器
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Action")
	TObjectPtr<UInputAction> IA_SwitchWeapon2;     // 2 键副武器
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Action")
	TObjectPtr<UInputAction> IA_QuickSwitchWeapon; // 滚轮快速切枪
	
	//4，角色机制
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Action")
	TObjectPtr<UInputAction> IA_ThrowGrenade; // 3 键：水火冰雷元素手雷
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Action")
	TObjectPtr<UInputAction> IA_Skill;        // E 键：元素战技
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Action")
	TObjectPtr<UInputAction> IA_Burst;        // Q 键：元素爆发（大招）
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Action")
	TObjectPtr<UInputAction> IA_SwitchCharacter; // Tab 键：双人小队即时切换
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Action")
	TObjectPtr<UInputAction> IA_Interact;     // F 键：场景交互
	
	//5.灵敏度参数
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Sensitivity")
	float BaseLookSensitivity = 1.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Sensitivity")
	float ADSSensitivityMultiplier = 0.6f;//开镜灵敏度衰减倍率
	
	//6，模式切换接口
	UFUNCTION(BlueprintCallable, Category = "Input")
	void SwitchToGameInputMode();
	
	UFUNCTION(BlueprintCallable, Category = "Input")
	void SwitchToUIInputMode();
	
protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	
	//输入回调函数
	void HandleMove(const FInputActionValue& Value);
	void HandleLook(const FInputActionValue& Value);
	void HandleJumpStarted();
	void HandleJumpCompleted();
	void HandleSprintStarted();
	void HandleSprintCompleted();
	void HandleDash();
	void HandleCrouchStarted();
	void HandleCrouchCompleted();
	void HandleToggleView();
	
	void HandleFireStarted();
	void HandleFireCompleted();
	void HandleADSStarted();
	void HandleADSCompleted();
	void HandleReload();
	
	void HandleSkill();
	void HandleBurst();
	void HandleThrowGrenadeStarted();
	void HandleThrowGrenadeCompleted();
	void HandleSwitchCharacter();
	void HandleInteract();
	
	void HandleSwitchWeapon1();
	void HandleSwitchWeapon2();
	void HandleQuickSwitchWeapon();

private:
	bool bIsAiming = false;
};