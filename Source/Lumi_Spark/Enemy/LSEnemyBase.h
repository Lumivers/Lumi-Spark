#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "Core/LSTypes.h"
#include "Core/LSInteractableInterface.h"
#include "LSEnemyBase.generated.h"

class ULSHealthComponent;
class ULSElementComponent;
class ULSShieldComponent;
class ULSThreatComponent;
class ULSEnemyDataAsset;

//处决广播
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnLSXenoBladeExecuted, AActor*, Victim, AActor*, Executioner, float, DamageDealt);

/**
 * 敌人角色基类 (ALSEnemyBase)
 * 挂载四大专职组件，由数据资产驱动属性与抗性，支持霸体与死亡结算
 */
UCLASS()
class LUMI_SPARK_API ALSEnemyBase : public ACharacter, public ILSInteractableInterface
{
	GENERATED_BODY()

public:
	ALSEnemyBase();

	virtual void BeginPlay() override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	// ILSInteractableInterface 交互接口实现
	virtual bool CanInteract(AActor* Interactor) const override;
	virtual FText GetInteractPrompt(AActor* Interactor) const override;
	virtual float GetInteractDuration(AActor* Interactor) const override { return 0.0f; } // 单击瞬发处决
	virtual void OnInteractComplete(AActor* Interactor) override;
	
	// 异体刃背后处决核心业务
	/**
	 * 执行异体刃背后刺杀
	 * 瞬碎目标全层元素护盾，并扣减 75% 最大生命
	 */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Execution")
	virtual bool ExecuteXenoBlade(AActor* Interactor);
	
	// 组件访问器
	FORCEINLINE ULSHealthComponent* GetHealthComponent() const { return HealthComponent; }
	FORCEINLINE ULSElementComponent* GetElementComponent() const { return ElementComponent; }
	FORCEINLINE ULSShieldComponent* GetShieldComponent() const { return ShieldComponent; }
	FORCEINLINE ULSThreatComponent* GetThreatComponent() const { return ThreatComponent; }
	FORCEINLINE ULSEnemyDataAsset* GetEnemyDataAsset() const { return EnemyDataAsset; }

	// 霸体状态查询
	UFUNCTION(BlueprintPure, Category = "Enemy|State")
	bool HasSuperArmor() const { return ActiveGameplayTags.HasTag(LSTags::TAG_State_SuperArmor); }

	// 移速切换（巡逻慢走 / 交火快跑）
	UFUNCTION(BlueprintCallable, Category = "Enemy|Movement")
	void SetCombatMovementSpeed(bool bInCombat);

	// 死亡回调
	UFUNCTION(BlueprintCallable, Category = "Enemy|LifeCycle")
	virtual void OnDeath(AActor* Killer);
	
	// 处决广播委托
	UPROPERTY(BlueprintAssignable, Category = "Enemy|Events")
	FOnLSXenoBladeExecuted OnXenoBladeExecuted;

protected:
	// 死亡广播掉落元素能量微粒给玩家小队
	virtual void SpawnEnergyParticles(AActor* Killer);

	// 响应生命组件广播的 OnDeath
	UFUNCTION()
	void HandleHealthComponentDeath();
	
	// ─── 专职战斗组件 ───
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<ULSHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<ULSElementComponent> ElementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<ULSShieldComponent> ShieldComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<ULSThreatComponent> ThreatComponent;

	// ─── 配置与状态 ───
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Config")
	TObjectPtr<ULSEnemyDataAsset> EnemyDataAsset;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Enemy|State")
	FGameplayTagContainer ActiveGameplayTags;

	// 缓存的元素抗性表
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Enemy|Stats")
	TMap<FGameplayTag, float> CachedResistances;
	
	// 是否允许被异体刃背后处决（精英怪默认允许，特定巨型首领可置 false）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Execution")
	bool bAllowBackstabExecution = true;
	
	// 处决伤害削减比例（默认 0.75 即 75% 最大生命）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Execution")
	float BackstabDamageRatio = 0.75f;
	
	// 处决最大距离（厘米，默认 230cm 约 2.3 米）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Execution")
	float BackstabMaxDistance = 230.0f;

	bool bIsDead = false;
};