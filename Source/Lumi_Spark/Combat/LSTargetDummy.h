#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LSTargetDummy.generated.h"

class UCapsuleComponent;
class USphereComponent;
class UStaticMeshComponent;
class ULSElementComponent;
class ULSShieldComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLSTargetDummyHealthChanged, float, CurrentHealth, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLSTargetDummyReset);

/**
 * 训练场打靶木桩实体 (Combat Target Dummy)
 * 挂载 ULSElementComponent，身体/头部弱点分层判定
 * 天然支持 16 种元素反应结算、伤害计算器全乘区拟合与 3D 伤害飘字跳字验证
 */
UCLASS()
class LUMI_SPARK_API ALSTargetDummy : public AActor
{
	GENERATED_BODY()

public:
	ALSTargetDummy();

	virtual void BeginPlay() override;

	// 引擎标准伤害重写入口
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	// 访问器
	FORCEINLINE UCapsuleComponent* GetCapsuleComponent() const { return CapsuleComp; }
	FORCEINLINE USphereComponent* GetHeadWeakspotComponent() const { return HeadWeakspotComp; }
	FORCEINLINE UStaticMeshComponent* GetMeshComponent() const { return MeshComp; }
	FORCEINLINE ULSElementComponent* GetElementComponent() const { return ElementComp; }
	FORCEINLINE ULSShieldComponent* GetShieldComponent() const { return ShieldComp; }

	// 手动/自动重置木桩状态
	UFUNCTION(BlueprintCallable, Category = "TargetDummy")
	void ResetDummy();

	UPROPERTY(BlueprintAssignable, Category = "TargetDummy|Events")
	FOnLSTargetDummyHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "TargetDummy|Events")
	FOnLSTargetDummyReset OnDummyReset;

protected:
	// 身体胶囊体碰撞（主受击盒）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TargetDummy|Components")
	TObjectPtr<UCapsuleComponent> CapsuleComp;

	// 头部弱点碰撞球（爆头弱点盒，带 "head" 标签）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TargetDummy|Components")
	TObjectPtr<USphereComponent> HeadWeakspotComp;

	// 显示网格体（可配置靶子贴图或木桩模型）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TargetDummy|Components")
	TObjectPtr<UStaticMeshComponent> MeshComp;

	// 元素反应核心组件（支持附着与 16 种元素反应）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TargetDummy|Components")
	TObjectPtr<ULSElementComponent> ElementComp;

	// 最大生命值（默认 50000 高血量供打靶测试）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TargetDummy|Stats")
	float MaxHealth = 50000.0f;

	// 当前生命值
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "TargetDummy|Stats")
	float CurrentHealth = 50000.0f;

	// 无限生命模式（开启后血量锁满，专供持续测打桩 DPS）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TargetDummy|Stats")
	bool bInfiniteHealth = false;

	// 被打空血量后自动回血重置的延迟时间（秒）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TargetDummy|Stats")
	float AutoResetDelay = 2.0f;
	
	// 护盾组件（可选挂载，支持多层护盾与元素克制）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TargetDummy|Components")
	TObjectPtr<ULSShieldComponent> ShieldComp;

private:
	FTimerHandle ResetTimerHandle;
};