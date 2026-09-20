#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "LSGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLSPlayerCountChanged, int32, NewPlayerCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLSRaidWipeStateChanged, bool, bIsWiped);

/**
 * Lumi-Spark 全局战斗状态中枢 (GameState)
 * 负责多人房间权威数据同步、根据 1~4 人动态计算怪物血量/护盾缩放系数
 */
UCLASS()
class LUMI_SPARK_API ALSGameState : public AGameStateBase
{
	GENERATED_BODY()
	
public:
	ALSGameState();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	// 动态数值缩放核心接口
	// 获取当前怪物的血量系数（1人 100%, 2人 160%, 3人 230%, 4人 300%）
	UFUNCTION(BlueprintCallable, Blueprintable, Category = "GameSession|Scaling")
	float GetDynamicHealthMultiplier() const;
	
	// 获取当前怪物的护盾系数
	UFUNCTION(BlueprintCallable, Blueprintable, Category = "GameSession|Scaling")
	float GetDynamicShieldMultiplier() const;
	
	//任何地方单行获取当前缩放比例
	UFUNCTION(BlueprintCallable, Category = "GameSession|Scaling", meta = (WorldContext = "WorldContextObject"))
	static float GetCurrentHealthMultiplier(const UObject* WorldContextObject);
	
	// ─── 服务端权威操作（由 GameMode 调用） ───
	void SetConnectedPlayerCount(int32 NewCount);
	void SetRaidWiped(bool bWiped);
	
	UFUNCTION(BlueprintPure, Category = "GameSession")
	int32 GetConnectedPlayerCount() const { return ConnectedPlayerCount; }
	
	UFUNCTION(BlueprintPure, Category = "GameSession")
	bool IsRaidWiped() const { return bIsRaidWiped; }
	
public:
	// ─── 委托事件 ───
	UPROPERTY(BlueprintAssignable, Category = "GameSession|Events")
	FOnLSPlayerCountChanged OnPlayerCountChanged;
	
	UPROPERTY(BlueprintAssignable, Category = "GameSession|Events")
	FOnLSRaidWipeStateChanged OnRaidWipeStateChanged;
	
protected:
	//当前连接并活跃的玩家总人数（权威属性复制）
	UPROPERTY(ReplicatedUsing = OnRep_ConnectedPlayerCount, VisibleInstanceOnly, BlueprintReadOnly, Category = "GameSession")
	int32 ConnectedPlayerCount = 1;
	
	//是否已触发全队覆灭（权威属性复制）
	UPROPERTY(ReplicatedUsing = OnRep_IsRaidWiped, VisibleInstanceOnly, BlueprintReadOnly, Category = "GameSession")
	bool bIsRaidWiped = false;
	
	/**
	 * 人数缩放梯度配置（索引 0 对应 1人，索引 1 对应 2人，以此类推）
	 * 默认：1人 1.0x, 2人 1.6x, 3人 2.3x, 4人 3.0x
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameSession|Config")
	TArray<float> HealthScaleTable = { 1.0f, 1.6f, 2.3f, 3.0f };
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameSession|Config")
	TArray<float> ShieldScaleTable = { 1.0f, 1.5f, 2.0f, 2.5f };
	
	UFUNCTION()
	void OnRep_ConnectedPlayerCount();
	
	UFUNCTION()
	void OnRep_IsRaidWiped();
};