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
	 * 初始化三人小队
	 * @param PrimaryCharacter 默认出战的主角色（Slot 0）
	 * @param StandbyClasses 待命副角色类型数组（Slot 1, Slot 2），服务端在后台静默生成并使其休眠
	 */
	UFUNCTION(BlueprintCallable, Category = "Team")
	void SetupTeam(ALSCharacterBase* PrimaryCharacter, const TArray<TSubclassOf<ALSCharacterBase>>& StandbyClasses);

	/** 全局基础切人条件校验（冷却中、小队有效性） */
	UFUNCTION(BlueprintPure, Category = "Team")
	bool CanSwitch() const;

	/** 校验能否切至指定槽位角色（检查目标存活、非当前角色） */
	UFUNCTION(BlueprintPure, Category = "Team")
	bool CanSwitchToIndex(int32 TargetIndex) const;

	/**
	 * 顺逆轮换切人（Tab 顺切、滚轮顺逆切）
	 * @param bForward true 向后轮换（0->1->2->0），false 向前轮换（0->2->1->0）
	 */
	UFUNCTION(BlueprintCallable, Category = "Team")
	bool CycleNextCharacter(bool bForward = true);

	/** 显式切换到指定索引角色（1 / 2 / 3 键直切） */
	UFUNCTION(BlueprintCallable, Category = "Team")
	bool SwitchTo(int32 TargetIndex);

	/** 快速切至下一个角色（兼容旧接口） */
	UFUNCTION(BlueprintCallable, Category = "Team")
	bool ToggleCharacter();

	/** 获取当前在场活跃角色 */
	UFUNCTION(BlueprintPure, Category = "Team")
	ALSCharacterBase* GetActiveCharacter() const;

	/** 获取当前待命后台角色（兼容旧双人小队蓝图调用） */
	UFUNCTION(BlueprintPure, Category = "Team")
	ALSCharacterBase* GetInactiveCharacter() const;

	/** 获取指定槽位的角色实例（0 ~ 2） */
	UFUNCTION(BlueprintPure, Category = "Team")
	ALSCharacterBase* GetCharacterAtIndex(int32 Index) const;

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