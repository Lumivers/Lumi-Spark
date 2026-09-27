#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Extraction/LSExtractionTypes.h"
#include "ALSExtractionPoint.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class ALSPlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLSExtractionPointStateChanged, bool, bIsActive, const FText&, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnLSExtractionProgressUpdated, ALSPlayerController*, PlayerController, float, Progress, float, RemainingTime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLSExtractionPointCompleted, ALSPlayerController*, PlayerController, const FLSRaidReport&, Report);

/**
 * 局内撤离点实体 (ALSExtractionPoint)
 * 玩家进入触发区域后启动倒计时读条（默认 5 秒）
 * 离开或阵亡立即中断；倒计时结束完成权威结算并带出物资
 */
UCLASS()
class LUMI_SPARK_API ALSExtractionPoint : public AActor
{
	GENERATED_BODY()

public:
	ALSExtractionPoint();

	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ─── 撤离点基础配置 ───

	/** 撤离点名称（如：地下排污口、直升机停机坪、应急折跃舱） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction")
	FText ExtractionPointName;

	/** 当前撤离点所属战区危险度（出击成功后扣除面罩耐久点数：简单-1 / 中等-1 / 困难-2 / 高危-3） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction")
	ELSHazardDifficulty ZoneHazard = ELSHazardDifficulty::Medium;

	/** 成功撤离所需留在区域内的读条时长（秒，默认 5 秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float ExtractionDuration = 5.0f;

	/** 是否处于激活可用状态（支持关卡机关、拉闸通电开启） */
	UPROPERTY(ReplicatedUsing = OnRep_IsActive, EditAnywhere, BlueprintReadWrite, Category = "Extraction")
	bool bIsActive = true;

	/** 是否为一次性撤离点（首个玩家/小队撤离后自动关闭通道） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction")
	bool bSingleUse = false;

	// ─── 外部机关控制与查询接口 ───

	/** 开启或关闭撤离点（供外部通电闸门或关卡蓝图调用） */
	UFUNCTION(BlueprintCallable, Category = "Extraction")
	void SetExtractionActive(bool bNewActive, const FText& Reason = FText::GetEmpty());

	/** 查询指定玩家当前是否正在该点读条撤离 */
	UFUNCTION(BlueprintPure, Category = "Extraction")
	bool IsPlayerExtracting(ALSPlayerController* PC) const;

	/** 获取某玩家当前的撤离进度 (0.0 ~ 1.0) */
	UFUNCTION(BlueprintPure, Category = "Extraction")
	float GetPlayerExtractionProgress(ALSPlayerController* PC) const;

	// ─── 委托事件 ───
	UPROPERTY(BlueprintAssignable, Category = "Extraction|Events")
	FOnLSExtractionPointStateChanged OnExtractionStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Extraction|Events")
	FOnLSExtractionProgressUpdated OnExtractionProgressUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Extraction|Events")
	FOnLSExtractionPointCompleted OnExtractionCompleted;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Extraction|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Extraction|Components")
	TObjectPtr<UBoxComponent> TriggerBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Extraction|Components")
	TObjectPtr<UStaticMeshComponent> BasePlatformMesh;

	// 触发器重叠事件
	UFUNCTION()
	void HandleTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleTriggerEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION()
	void OnRep_IsActive();

	/** 执行指定玩家的权威结算与带出 */
	void CompleteExtractionForPlayer(ALSPlayerController* PC);

private:
	// 正在该撤离点读条的玩家及其已累计停留时长（仅在服务器维护）
	TMap<TWeakObjectPtr<ALSPlayerController>, float> ExtractingPlayers;

	/** 辅助获取 Pawn 对应的 PlayerController */
	static ALSPlayerController* GetPlayerControllerFromActor(AActor* Actor);
};