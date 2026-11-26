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
class UAnimMontage;
class UNiagaraSystem;
class USoundBase;
class UParticleSystem;
class UWidgetComponent;

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

	// 响应生命组件广播更新头顶血条
	UFUNCTION()
	void HandleHealthChanged(float CurrentHealth, float MaxHealth, bool bIsDamage);

	// 尝试播放受击音效（内部执行冷却校验与网络多播）
	void TryPlayHitSound();

	// 多播播放受击音效（确保联机主机与客户端同步听到 3D 受击音效）
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayHitSound();
	
	// ─── 专职战斗组件 ───
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<ULSHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<ULSElementComponent> ElementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<ULSShieldComponent> ShieldComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<ULSThreatComponent> ThreatComponent;

	// ─── 敌人头顶血条组件 ───
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> HealthBarComponent;

	// 头顶血条相对胶囊体顶部的竖直抬升偏移（厘米）
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Enemy|HealthBar", meta = (ClampMin = "0.0"))
	float HealthBarHeadClearance = 28.0f;

	// 头顶血条屏幕空间绘制宽高（像素）
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Enemy|HealthBar")
	FVector2D HealthBarDrawSize = FVector2D(108.0f, 9.0f);

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

	// ─── 死亡与布娃娃表现 ───
	/** 是否优先采用物理布娃娃（Ragdoll）倒地，若为 false 且配置了 DeathMontage 则播放死亡蒙太奇 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Death")
	bool bUseRagdollDeath = true;

	/** 死亡动画蒙太奇（当 bUseRagdollDeath 为 false 或备选时播放） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Death")
	TObjectPtr<UAnimMontage> DeathMontage;

	/** 致命一击赋予布娃娃的物理冲量初速度（cm/s，推荐 350~600，赋予子弹击杀动量） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Death", meta = (ClampMin = "0.0"))
	float DeathImpulseForce = 450.0f;

	/** 尸体在场景中保留的时间（秒，默认 5 秒后清理 Actor） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Death", meta = (ClampMin = "1.0"))
	float CorpseLifeSpan = 5.0f;

	// ─── 音效表现 ───
	/** 受击音效（敌人受到非致死伤害时播放，带冷却防止连射鬼畜） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Audio")
	TObjectPtr<USoundBase> HitSound;

	/** 受击音效冷却时间（秒，默认 0.35 秒；防止高射速武器或霰弹单发多弹丸连续触发） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Audio", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float HitSoundCooldown = 0.35f;

	/** 死亡音效（击杀倒地时播放） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death")
	TObjectPtr<USoundBase> DeathSound;

	/** 能量微粒掉落 Niagara 特效（在怪物体表/击杀位置迸发） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Drops")
	TObjectPtr<UNiagaraSystem> EnergyDropNiagara;

	/** 能量微粒掉落 Cascade 粒子特效（兼容传统粒子发射器） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Drops")
	TObjectPtr<UParticleSystem> EnergyDropEmitter;

	bool bIsDead = false;

	// 记录最后攻击该怪物的施加者（用于击杀判定与侵蚀度结算）
	UPROPERTY()
	TWeakObjectPtr<AActor> LastDamageCauser;

	UPROPERTY()
	TWeakObjectPtr<AController> LastDamageInstigator;

	// 记录最后一次受击的物理特征（用于致命一击布娃娃继承射击动量）
	FVector LastHitLocation = FVector::ZeroVector;
	FVector LastHitDirection = FVector::ZeroVector;
	FName LastHitBoneName = NAME_None;

	// 上次播放受击音效的世界时间（负值确保首次受击必然发声）
	double LastHitSoundTime = -1.0;

	void ApplyCorrosionReductionToKiller(AActor* Killer);
};