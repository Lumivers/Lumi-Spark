#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Core/LSTypes.h"
#include "LSDendroCore.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;

/**
 * 草原核（草种子）物理实体 Actor
 * 水+草反应生成，具备 6 秒自爆倒计时；
 * 遇火触发烈绽放 (AOE 大范围爆轰)，遇雷触发超绽放 (自动寻敌飞弹)
 */
UCLASS()
class LUMI_SPARK_API ALSDendroCore : public AActor
{
	GENERATED_BODY()

public:
	ALSDendroCore();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 接收伤害与元素触发入口
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	// 初始化制造者面板（供剧变公式计算等级与精通）
	void InitializeCore(AActor* InSpawner, int32 InLevel, float InEM);

	// ─── 核心绽放分支 ───

	// 1. 原绽放超时自然爆炸（2.0x 基础草伤）
	UFUNCTION(BlueprintCallable, Category = "Element|DendroCore")
	void ExplodeBloom();

	// 2. 烈绽放 (火引爆)：大范围火草 AOE 爆轰（3.0x 伤害）
	UFUNCTION(BlueprintCallable, Category = "Element|DendroCore")
	void TriggerBurgeon(AActor* InstigatorActor);

	// 3. 超绽放 (雷引爆)：转化为寻敌弹道射向最近敌人（3.0x 伤害）
	UFUNCTION(BlueprintCallable, Category = "Element|DendroCore")
	void TriggerHyperbloom(AActor* InstigatorActor);

	// 外部直接注入元素触发（解耦支持技能或手雷）
	UFUNCTION(BlueprintCallable, Category = "Element|DendroCore")
	void ApplyReactionTrigger(const FGameplayTag& TriggerElement, AActor* InstigatorActor);

protected:
	// 球形物理碰撞体（刚体模拟、地面弹跳滚动）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> SphereCollision;

	// 草种子静态网格模型
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> CoreMesh;

	// 弹道追踪组件（超绽放雷引爆时激活）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	// 存活寿命（默认 6.0 秒）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DendroCore|Balance")
	float LifeTime = 6.0f;

	// 烈绽放爆轰半径（默认 5 米）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DendroCore|Balance")
	float BurgeonRadius = 500.0f;

	// 原绽放自爆半径（默认 3.5 米）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DendroCore|Balance")
	float BloomRadius = 350.0f;

private:
	FTimerHandle LifeTimerHandle;
	bool bHasExploded = false;
	bool bIsHyperbloomHoming = false;

	TWeakObjectPtr<AActor> SpawnerActor;
	int32 SpawnerLevel = 90;
	float SpawnerEM = 100.0f;

	// 碰撞击中目标回调（用于超绽放飞弹命中敌对 Pawn 时起爆）
	UFUNCTION()
	void OnCoreHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	// 寻找周围最近的敌对 Pawn
	AActor* FindNearestTarget(float SearchRadius);
};