#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "Core/LSTypes.h"
#include "LSEnemyBase.generated.h"

class ULSHealthComponent;
class ULSElementComponent;
class ULSShieldComponent;
class ULSThreatComponent;
class ULSEnemyDataAsset;

/**
 * 敌人角色基类 (ALSEnemyBase)
 * 挂载四大专职组件，由数据资产驱动属性与抗性，支持霸体与死亡结算
 */
UCLASS()
class LUMI_SPARK_API ALSEnemyBase : public ACharacter
{
	GENERATED_BODY()

public:
	ALSEnemyBase();

	virtual void BeginPlay() override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

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

	bool bIsDead = false;
};