#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LSTeamSwitchComponent.generated.h"

class ALSCharacterBase;

// ─── 委托声明 ───
// 角色切换完成广播：(退场角色, 入场角色, 新激活的角色索引)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnLSTeamSwitch, ALSCharacterBase*, OutCharacter, ALSCharacterBase*, InCharacter, int32, NewActiveIndex);

// 换人冷却倒计时变更广播：(当前剩余冷却, 最大冷却)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLSSwitchCooldownChanged, float, CurrentCooldown, float, MaxCooldown);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LUMI_SPARK_API ULSTeamSwitchComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSTeamSwitchComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/**
	 * 初始化双人小队
	 * @param PrimaryCharacter 默认登场的主角色（Slot 0）
	 * @param SecondaryClass 待命副角色类型（Slot 1），服务端将在后台静默生成该实例
	 */
	UFUNCTION(BlueprintCallable, Category = "Team")
	void SetupTeam(ALSCharacterBase* PrimaryCharacter, TSubclassOf<ALSCharacterBase> SecondaryClass);

	/** 当前是否满足切换条件（CD结束、目标角色存活等） */
	UFUNCTION(BlueprintPure, Category = "Team")
	bool CanSwitch() const;

	/** 切换到另一名角色（0 <-> 1 对调） */
	UFUNCTION(BlueprintCallable, Category = "Team")
	bool ToggleCharacter();

	/** 显式切换到指定索引角色 */
	UFUNCTION(BlueprintCallable, Category = "Team")
	bool SwitchTo(int32 TargetIndex);

	/** 获取当前在场活跃角色 */
	UFUNCTION(BlueprintPure, Category = "Team")
	ALSCharacterBase* GetActiveCharacter() const;

	/** 获取当前待命后台角色 */
	UFUNCTION(BlueprintPure, Category = "Team")
	ALSCharacterBase* GetInactiveCharacter() const;

	/** 获取当前切换冷却剩余时间 */
	UFUNCTION(BlueprintPure, Category = "Team")
	float GetCooldownTimer() const { return CooldownTimer; }

	/** 获取最大切换冷却时间 */
	UFUNCTION(BlueprintPure, Category = "Team")
	float GetSwitchCooldown() const { return SwitchCooldown; }

	/** 获取当前活跃角色的槽位索引 */
	UFUNCTION(BlueprintPure, Category = "Team")
	int32 GetActiveIndex() const { return ActiveIndex; }

public:
	// ─── 委托事件 ───
	UPROPERTY(BlueprintAssignable, Category = "Team|Events")
	FOnLSTeamSwitch OnTeamSwitch;

	UPROPERTY(BlueprintAssignable, Category = "Team|Events")
	FOnLSSwitchCooldownChanged OnSwitchCooldownChanged;

protected:
	/**
	 * 执行角色的无缝交接逻辑（坐标交接、视线/动量继承、Controller Possession 转移）
	 */
	void PerformSwitch(ALSCharacterBase* OutChar, ALSCharacterBase* InChar, int32 NewIndex);

protected:
	// ─── 配置与运行时状态 ───

	/** 小队成员数组（固定容量 2：[0] 主角色，[1] 副角色） */
	UPROPERTY(Transient)
	TArray<TObjectPtr<ALSCharacterBase>> TeamMembers;

	/** 当前在场角色的槽位索引（0 或 1） */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Team")
	int32 ActiveIndex = 0;

	/** 切换角色的基础冷却时间（秒） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Team|Config")
	float SwitchCooldown = 1.0f;

	/** 换人冷却剩余计时器 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Team")
	float CooldownTimer = 0.0f;

	/** 切换时是否继承角色移动动量（速度向量），保障奔跑/滑铲时的丝滑无缝感 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Team|Config")
	bool bInheritVelocity = true;

	/** 切换时新角色是否继承当前的准星朝向 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Team|Config")
	bool bInheritAimDirection = true;
};