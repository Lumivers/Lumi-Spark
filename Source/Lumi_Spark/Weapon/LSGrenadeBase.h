#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Core/LSTypes.h"
#include "LSGrenadeBase.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class USoundBase;
class UParticleSystem;

/**
 * 元素物理投掷物基类 (ALSGrenadeBase)
 * 装配物理碰撞球体、弹道运动组件与倒计时引信
 * 落地/引信到期后触发防穿墙径向爆炸，广播伤害并附着对应量级的元素
 */
UCLASS(Abstract)
class LUMI_SPARK_API ALSGrenadeBase : public AActor
{
    GENERATED_BODY()

public:
    ALSGrenadeBase();

    // 组件获取接口
    FORCEINLINE USphereComponent* GetCollisionComp() const { return CollisionComp; }
    FORCEINLINE UStaticMeshComponent* GetMeshComp() const { return MeshComp; }
	FORCEINLINE UProjectileMovementComponent* GetProjectileMovement() const { return ProjectileMovement; }
	FORCEINLINE FGameplayTag GetElementTag() const { return ElementTag; }

    // 核心逻辑接口：触发爆炸
    UFUNCTION(BluePrintCallable, CateGory = "Grenade|Combat")
    virtual void Explode();

protected:
    virtual void BeginPlay() override;

    // ─── 组件层级 ───
    // 物理碰撞核心（作为 RootComponent 纳管物理运动与弹跳）
    UPROPERTY(VisibleAnyWhere, BlueprintReadOnly, CateGory = "Grenade|Components")
    TObjectPtr<USphereComponent> CollisionComp;

    //投掷物外观网格体（附加在碰撞体下，无碰撞）
    UPROPERTY(VisibleAnyWhere, BlueprintReadOnly, CateGory = "Grenade|Components")
    TObjectPtr<UStaticMeshComponent> MeshComp;

    //抛物线与物理弹道运动纳管组件
    UPROPERTY(VisibleAnyWhere, BlueprintReadOnly, CateGory = "Grenade|Components")
    TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

    // ─── 引信与爆炸规则 ───
	// 引信倒计时（秒）
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, CateGory = "Grenade|Fuse", meta = (ClampMin = 0.1f))
    float FuseTime = 2.5f;

    // 是否触碰任意障碍物/敌人即瞬间引爆（冲击雷属性）
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, CateGory = "Grenade|Fuse")
    bool bExplodeOnImpact = false;

    // ─── 伤害与爆炸波及半径 ───
	// 爆炸核心基础伤害
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, CateGory = "Grenade|Damage", meta = (ClampMin = 0.0))
    float BaseDamage = 150.f;

    // 爆炸外边缘最低伤害
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, CateGory = "Grenade|Damage", meta = (ClampMin = 0.0))
    float MinDamage = 30.f;

    // 是否为向心吸附力（风雷开启此项：将敌人向爆炸中心拉扯聚怪，而非向外炸飞）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade|Physics")
	bool bInwardPull = false;

    // 核心全额伤害半径（厘米）
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, CateGory = "Grenade|Damage", meta = (ClampMin = 10.0))
    float InnerRadius = 150.0f;

    // 最大波及衰减半径（厘米）
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, CateGory = "Grenade|Damage", meta = (ClampMin = 50.0))
    float OuterRadius = 600.f;

    // 伤害径向衰减指数（1.0 为线性递减，>1.0 边缘衰减更快）
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, CateGory = "Grenade|Damage", meta = (ClampMin = 0.1))
    float DamageFalloffExponent = 1.0f;

    // 爆炸中心产生的物理击退冲量大小
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, CateGory = "Grenade|Physics")
    float ExplosionImpulse = 40000.0f;

    // ─── 元素属性注入 ───
	// 手雷自带的元素类型 Tag（火/水/冰/雷等）
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, CateGory = "Grenade|Element")
    FGameplayTag ElementTag;

    // 手雷自带的元素量级（手雷默认赋予 Heavy 2U 强元素，持续 12s 衰减，形成良好反应源）
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, CateGory = "Grenade|Element")
    ELSElementGauge ElementGauge = ELSElementGauge::Heavy;

    // ─── 爆炸特效与音效 ───
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, CateGory = "Grenade|Effects")
    TObjectPtr<UParticleSystem> ExplosionEmitter;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, CateGory = "Grenade|Effects")
    TObjectPtr<USoundBase> ExplosionSound;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, CateGory = "Grenade|Effects")
    TObjectPtr<USoundBase> BounceSound;

    // ─── 物理碰撞事件绑定 ───
    UFUNCTION()
    virtual void OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

    // 计算径向爆炸波及检测（射线防穿墙、计算伤害与广播元素总线）
    virtual void PerformExplosionDamageAndElement(const FVector& ExplosionCenter);

    // 距离伤害衰减换算公式
    float CalculateRadialDamage(float Distance) const;

private:
    // 引信倒计时器句柄
    FTimerHandle FuseTimerHandle;

    // 标记是否已经引爆，防止二次入栈
    bool bHasExploded = false;
};

// ══════════════════════════════════════════════════════════════
// 4 种基础元素雷 C++ 预置派生类
// ══════════════════════════════════════════════════════════════

/** 烈火高爆手雷：超强爆炸破坏力、较大击退、施加 2U 强火附着 */
UCLASS()
class LUMI_SPARK_API ALSGrenade_Pyro : public ALSGrenadeBase
{
    GENERATED_BODY()
public:
    ALSGrenade_Pyro();
};

/** 潮汐激流手雷：范围波及广、全场大范围潮湿浸染、施加 2U 强水附着 */
UCLASS()
class LUMI_SPARK_API ALSGrenade_Hydro : public ALSGrenadeBase
{
	GENERATED_BODY()
public:
	ALSGrenade_Hydro();
};

/** 寒霜极冰手雷：极寒冷气凝结、施加 2U 强冰附着，控场冻结关键先手 */
UCLASS()
class LUMI_SPARK_API ALSGrenade_Cryo : public ALSGrenadeBase
{
	GENERATED_BODY()
public:
	ALSGrenade_Cryo();
};


/** 狂雷过载手雷：集中高脉冲爆发、施加 2U 强雷附着，剧变感电/超导点火器 */
UCLASS()
class LUMI_SPARK_API ALSGrenade_Electro : public ALSGrenadeBase
{
	GENERATED_BODY()
public:
	ALSGrenade_Electro();
};

/** 丰饶剧变草雷：施加 2U 强草附着，大范围播撒草系反应底，催生草原核核心 */
UCLASS()
class LUMI_SPARK_API ALSGrenade_Dendro : public ALSGrenadeBase
{
	GENERATED_BODY()
public:
	ALSGrenade_Dendro();
};

/** 涡流引力风雷：超大范围向心聚怪黑洞，施加风元素触发大范围元素扩散 */
UCLASS()
class LUMI_SPARK_API ALSGrenade_Anemo : public ALSGrenadeBase
{
	GENERATED_BODY()
public:
	ALSGrenade_Anemo();
};