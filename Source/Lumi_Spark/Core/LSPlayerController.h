#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Core/LSInteractableInterface.h"
#include "LSPlayerController.generated.h"

//前向声明
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class ALSCharacterBase;
class ULSTeamSwitchComponent;
class ULSDriveCoreComponent;
class ULSBackpackComponent;
class ULSCorrosionComponent;

UCLASS()
class LUMI_SPARK_API ALSPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	ALSPlayerController();
	virtual void Tick(float DeltaSeconds) override;
	
	//1，Input Mapping Context（输入映射上下文）配置
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Context")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Context")
	TObjectPtr<UInputMappingContext> UIModeMappingContext;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> HUDWidgetClass;
	
	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> HUDWidgetInstance = nullptr;
	
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
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Team")
	TObjectPtr<UInputAction> IA_SwitchToSlot1; // 1 键直切角色 1
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Team")
	TObjectPtr<UInputAction> IA_SwitchToSlot2; // 2 键直切角色 2
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Team")
	TObjectPtr<UInputAction> IA_SwitchToSlot3; // 3 键直切角色 3
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Team")
	TObjectPtr<UInputAction> IA_CycleCharacter; // 滚轮顺逆切 (Axis1D)
	
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Action")
	TSubclassOf<class ALSGrenadeBase> DefaultGrenadeClass;
	
	//5.灵敏度参数
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Sensitivity")
	float BaseLookSensitivity = 1.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Sensitivity")
	float ADSSensitivityMultiplier = 0.6f;//开镜灵敏度衰减倍率

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Team", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULSTeamSwitchComponent> TeamSwitchComponent;

	// 待命副角色类型配置（Slot 1, Slot 2），在 OnPossess 时由服务端权威静默拉起
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Team|Classes")
	TArray<TSubclassOf<class ALSCharacterBase>> StandbyCharacterClasses;

	FORCEINLINE ULSTeamSwitchComponent* GetTeamSwitchComponent() const { return TeamSwitchComponent; }
	
	//6，模式切换接口
	UFUNCTION(BlueprintCallable, Category = "Input")
	void SwitchToGameInputMode();
	
	UFUNCTION(BlueprintCallable, Category = "Input")
	void SwitchToUIInputMode();
	
	//服务端权威交互完成RPC
	UFUNCTION(Server, Reliable)
	void Server_CompleteInteract(AActor* TargetActor);
	
	UFUNCTION(BlueprintPure, Category = "Equipment")
	ULSDriveCoreComponent* GetDriveCoreComponent() const { return DriveCoreComponent; }
	
	// ═══ 控制台测试与打靶指令 ═══
	
	// 控制台一键装备 4+2 预设（按 ~ 键输入: LS.EquipSet Resonance 或 LS.EquipSet Marksman） 
	UFUNCTION(Exec, Category = "DriveCore|Debug")
	void LSEquipSet(const FString& SetName);
	
	// 控制台在屏幕上打印当前小队全量战力属性（输入: LS.PrintStats） 
	UFUNCTION(Exec, Category = "DriveCore|Debug")
	void LSPrintStats();
	
	UFUNCTION(BlueprintPure, Category = "Extraction")
	ULSBackpackComponent* GetBackpackComponent() const { return BackpackComponent; }
	
	// ═══ 搜打撤调试指令 ═══
	// 拾取物资 (LS.AddLoot Bearing / LS.AddLoot Tape / LS.AddLoot Record / LS.AddLoot Disc / LS.AddLoot Blueprint)
	UFUNCTION(Exec, Category = "Extraction|Debug")
	void LSAddLoot(const FString& LootType);
	
	// 升级背包阶位
	UFUNCTION(Exec, Category = "Extraction|Debug")
	void LSUpgradeBackpack();
	
	// 在屏幕与日志打印当前全队背包与安全箱明细 (LS.PrintBackpack)
	UFUNCTION(Exec, Category = "Extraction|Debug")
	void LSPrintBackpack();
	
	// 模拟阵亡掉落 (LS.SimulateDeath)
	UFUNCTION(Exec, Category = "Extraction|Debug")
	void LSSimulateDeath();

	UFUNCTION(BlueprintPure, Category = "Extraction")
	ULSCorrosionComponent* GetCorrosionComponent() const { return CorrosionComponent; }
	
	// ═══ 阶段 9.1 侵蚀与处决调试指令 ═══
	// 手动增加侵蚀值（如 LS.AddCorrosion 50 或 LS.AddCorrosion 100 触发过载）
	UFUNCTION(Exec, Category = "Extraction|Debug")
	void LSAddCorrosion(float Amount);
	
	// 立即使用净化针 (LS.UseInjector)
	UFUNCTION(Exec, Category = "Extraction|Debug")
	void LSUseInjector();
	
	// 立即更换滤芯 (LS.InstallFilter 180)
	UFUNCTION(Exec, Category = "Extraction|Debug")
	void LSInstallFilter(float Durability);
	
	// 打印侵蚀与滤芯状态 (LS.PrintCorrosion)
	UFUNCTION(Exec, Category = "Extraction|Debug")
	void LSPrintCorrosion();
	
protected:
	//当前交互目标（由 ILSInteractableInterface 接口提供）
	TWeakObjectPtr<AActor> CurrentInteractTarget;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULSDriveCoreComponent> DriveCoreComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Extraction", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULSBackpackComponent> BackpackComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Extraction", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULSCorrosionComponent> CorrosionComponent;
	
	//当前长按进度计时器与总时长
	float InteractTimer = 0.0f;
	float CurrentInteractDuration = 0.0f;
	
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
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
	void HandleInteractStarted();
	void HandleInteractCompleted();
	
	void HandleSwitchToSlot1();
	void HandleSwitchToSlot2();
	void HandleSwitchToSlot3();
	void HandleCycleCharacter(const struct FInputActionValue& Value);

private:
	bool bIsAiming = false;
};