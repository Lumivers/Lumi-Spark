#include "Weapon/LSGrenadeBase.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "CollisionQueryParams.h"
#include "Core/LSEventBus.h"
#include "Core/LSTypes.h"

ALSGrenadeBase::ALSGrenadeBase()
{
    PrimaryActorTick.bCanEverTick = false;// 手雷物理交由 ProjectileMovement 纳管，无须自身 Tick
	
	// 开启网络同步与物理弹道同步
	bReplicates = true;
	SetReplicateMovement(true);

    // 1. 物理碰撞球体（RootComponent）
    CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
    CollisionComp->InitSphereRadius(14.0f);
    CollisionComp->SetCollisionProfileName(TEXT("Projectile"));
    CollisionComp->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.f));
    CollisionComp->CanCharacterStepUpOn = ECB_No;
    CollisionComp->OnComponentHit.AddDynamic(this, &ALSGrenadeBase::OnProjectileHit);
    RootComponent = CollisionComp;

    // 2. 投掷物外观网格体（附加在碰撞体下，无碰撞）
    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
    MeshComp->SetupAttachment(CollisionComp);
    MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // 3.弹道物理组件参数配置
    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->UpdatedComponent = CollisionComp;
    ProjectileMovement->InitialSpeed = 1600.f;
    ProjectileMovement->MaxSpeed = 1600.f;
    ProjectileMovement->bRotationFollowsVelocity = true;
    ProjectileMovement->bShouldBounce = true;
    ProjectileMovement->Bounciness = 0.35f; // 弹性系数，影响弹道反弹高度
    ProjectileMovement->Friction = 0.6f; // 摩擦力，影响弹道滚动距离
    ProjectileMovement->BounceVelocityStopSimulatingThreshold = 40.f; // 弹道速度低于该值时停止模拟
}

void ALSGrenadeBase::BeginPlay()
{
    Super::BeginPlay();

    // 忽略生成者自身（防止刚扔出手雷与自己反弹）
    if (AActor* MyInstigator = GetInstigator())
    {
        CollisionComp->IgnoreActorWhenMoving(MyInstigator, true);
    }

    //启动引信倒计时
    if (FuseTime > 0.f)
    {
        GetWorldTimerManager().SetTimer(FuseTimerHandle, this, &ALSGrenadeBase::Explode, FuseTime, false);
    }
}

void ALSGrenadeBase::OnProjectileHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    // 冲击雷机制：触碰非自生物体瞬间起爆
    if (bExplodeOnImpact)
    {
        Explode();
        return;
    }

    // 普通引信雷：碰撞弹跳播放撞击声
    if (BounceSound && Hit.bBlockingHit && OtherActor != GetInstigator())
    {
        UGameplayStatics::PlaySoundAtLocation(this, BounceSound, Hit.ImpactPoint, 0.6f);
    }
}

void ALSGrenadeBase::Explode()
{
    if (bHasExploded) return;
    bHasExploded = true;

    const FVector ExplosionCenter = GetActorLocation();

    // 1. 播放爆炸特效
    if (ExplosionEmitter)
    {
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEmitter, ExplosionCenter, GetActorRotation());
    }

    // 2. 播放爆炸音效
    if (ExplosionSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, ExplosionCenter);
    }


	// 3. 纯 C++ 调试球体（绿色核心 150cm，橙色外圈 600cm，持续 1.5s）
	DrawDebugSphere(GetWorld(), ExplosionCenter, InnerRadius, 16, FColor::Green, false, 1.5f, 0, 1.0f);
	DrawDebugSphere(GetWorld(), ExplosionCenter, OuterRadius, 24, FColor::Orange, false, 1.5f, 0, 1.0f);
	
    // 4. 执行核心伤害检测与元素注入
	PerformExplosionDamageAndElement(ExplosionCenter);
	
    // 5. 销毁投掷物 Actor
	Destroy();
}

void ALSGrenadeBase::PerformExplosionDamageAndElement(const FVector& ExplosionCenter)
{
    // 1,球形空间重叠收集（检索Pawn玩家/敌人与PhysicsBody）
    TArray<FOverlapResult> OverlapResults;
    FCollisionShape ExplosionSphere = FCollisionShape::MakeSphere(OuterRadius);

    FCollisionObjectQueryParams ObjectQueryParams;
    ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn); // 玩家/敌人
    ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody); // 物理
    ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic); // 动态物体

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this); // 忽略自身

    GetWorld()->OverlapMultiByObjectType(OverlapResults, ExplosionCenter, FQuat::Identity, ObjectQueryParams, ExplosionSphere, QueryParams);

    // 避免对同一个复合碰撞体的Actor重复计算
    TSet<AActor*> DamagedActors;

    ULSEventBus* EventBus = ULSEventBus::Get(this);

    for (const FOverlapResult& Overlap : OverlapResults)
    {
        AActor* HitActor = Overlap.GetActor();
        if (!HitActor || DamagedActors.Contains(HitActor))
        {
            continue;
        }

        DamagedActors.Add(HitActor);

        // 2. 防穿墙射线检测（Line of Sight Raycast）
		// 从爆炸中心向受击 Actor 质心拉一条射线，若被掩体墙壁阻隔则免疫爆炸
        FHitResult SightHit;
        FCollisionQueryParams SightParams;
        SightParams.AddIgnoredActor(this);
        SightParams.AddIgnoredActor(HitActor);

        const FVector TargetLocation = HitActor->GetActorLocation();
        const bool bBlocked = GetWorld()->LineTraceSingleByChannel(SightHit, ExplosionCenter, TargetLocation, ECC_Visibility, SightParams);

        if (bBlocked)
        {
            // 射线被阻挡，免疫爆炸伤害
            DrawDebugLine(Getworld(), ExplosionCenter, SightHit.ImpactPoint, FColor::Red, false, 1.5f, 0, 1.0f);
            continue;
        }

        // 视线通畅，画绿线表示有效命中
		DrawDebugLine(GetWorld(), ExplosionCenter, TargetLocation, FColor::Emerald, false, 1.5f, 0, 1.0f);

        // 3, 计算径向距离衰减伤害
        const float Distance = FVector::Dist(ExplosionCenter, TargetLocation);
        const float FinalExplosionDamage = CalculateRadialDamage(Distance);

        // 4. 计算物理冲量 / 角色击退或向心聚怪
		// 若 bInwardPull 为 true，冲量矢量指向手雷中心（吸附），否则指向目标外侧（炸飞）
		const FVector ImpulseDirection = bInwardPull 
			? (ExplosionCenter - TargetLocation).GetSafeNormal() 
			: (TargetLocation - ExplosionCenter).GetSafeNormal();

		if (ACharacter* Char = Cast<ACharacter>(HitActor))
		{
			const float ImpulseScale = FMath::Clamp(1.0f - (Distance / OuterRadius), 0.2f, 1.0f);
			// 聚怪时附带向上微量抬升（Z+160），让敌人脱离地面摩擦力顺滑被吸向中心
			const FVector UpLift = bInwardPull ? FVector(0.f, 0.f, 160.0f) : FVector(0.f, 0.f, 200.0f);
			Char->LaunchCharacter(ImpulseDirection * (ExplosionImpulse / 100.0f) * ImpulseScale + UpLift, true, true);
		}
		else if (UPrimitiveComponent* Prim = Overlap.GetComponent())
		{
			if (Prim->IsSimulatingPhysics())
			{
				// 物理实体同样根据正负冲量执行径向冲量
				const float FinalImpulse = bInwardPull ? -ExplosionImpulse : ExplosionImpulse;
				Prim->AddRadialImpulse(ExplosionCenter, OuterRadius, FinalImpulse, ERadialImpulseFalloff::RIF_Linear, true);
			}
		}

        // 5, 组装全局伤害上下文
        FLSDamageContext DamageContext;
        DamageContext.DamageCauser = GetInstigator() ? Cast<AActor>(GetInstigator()) : this;
        DamageContext.TargetActor = HitActor;
        DamageContext.BaseDamage = BaseDamage;
        DamageContext.FinalDamage = FinalExplosionDamage;
        DamageContext.ElementTag = ElementTag;
        DamageContext.DamageTypeTag = LSTags::TAG_Damage_Type_Explosion;
        DamageContext.bIsHeadshot = false;
        DamageContext.bIsCritical = false;

        // 6, 广播全局伤害事件与元素附着
        if (EventBus)
        {
            //广播伤害结算 （HUD跳字，受击判定，怪物血条）
            EventBus->OnDamageDealt.Broadcast(DamageContext);

            //广播元素附着
            if (ElementTag.IsValid())
            {
                EventBus->OnElementApplied.Broadcast(HitActor, ElementTag, ElementGauge);
            }
        }
    }
}

float ALSGrenadeBase::CalculateRadialDamage(float Distance) const
{
    if (Distance <= InnerRadius)
    {
        return BaseDamage;
    }
    if (Distance >= OuterRadius)
    {
        return MinDamage;
    }

    const float Alpha = (Distance - InnerRadius) / (OuterRadius - InnerRadius);
    const float FallOffFactor = FMath::Pow(Alpha, DamageFalloffExponent);
    return FMath::Lerp(BaseDamage, MinDamage, FallOffFactor);
}

// ══════════════════════════════════════════════════════════════
// 派生类默认数值构造
// ══════════════════════════════════════════════════════════════

ALSGrenade_Pyro::ALSGrenade_Pyro()
{
	ElementTag = LSTags::TAG_Element_Pyro;
	BaseDamage = 200.0f;
	MinDamage = 45.0f;
	InnerRadius = 180.0f;
	OuterRadius = 650.0f;
	ExplosionImpulse = 55000.0f;
	ElementGauge = ELSElementGauge::Heavy; // 2U 强火
}

ALSGrenade_Hydro::ALSGrenade_Hydro()
{
	ElementTag = LSTags::TAG_Element_Hydro;
	BaseDamage = 130.0f;
	MinDamage = 30.0f;
	InnerRadius = 200.0f;
	OuterRadius = 750.0f; // 潮水范围最大
	ExplosionImpulse = 35000.0f;
	ElementGauge = ELSElementGauge::Heavy; // 2U 强水
}

ALSGrenade_Cryo::ALSGrenade_Cryo()
{
	ElementTag = LSTags::TAG_Element_Cryo;
	BaseDamage = 140.0f;
	MinDamage = 35.0f;
	InnerRadius = 160.0f;
	OuterRadius = 600.0f;
	ExplosionImpulse = 30000.0f;
	ElementGauge = ELSElementGauge::Heavy; // 2U 强冰
}

ALSGrenade_Electro::ALSGrenade_Electro()
{
	ElementTag = LSTags::TAG_Element_Electro;
	BaseDamage = 175.0f;
	MinDamage = 40.0f;
	InnerRadius = 150.0f;
	OuterRadius = 580.0f;
	ExplosionImpulse = 45000.0f;
	ElementGauge = ELSElementGauge::Heavy; // 2U 强雷
}

ALSGrenade_Dendro::ALSGrenade_Dendro()
{
	ElementTag = LSTags::TAG_Element_Dendro;
	BaseDamage = 135.0f;
	MinDamage = 35.0f;
	InnerRadius = 180.0f;
	OuterRadius = 620.0f;
	ExplosionImpulse = 28000.0f;
	bInwardPull = false;
	ElementGauge = ELSElementGauge::Heavy; // 2U 强草，底元素储备充足
}

ALSGrenade_Anemo::ALSGrenade_Anemo()
{
	ElementTag = LSTags::TAG_Element_Anemo;
	BaseDamage = 80.0f;                   // 风雷侧重控场与扩散，纯爆破伤害偏低
	MinDamage = 20.0f;
	InnerRadius = 250.0f;
	OuterRadius = 720.0f;                 // 超大聚怪吸附范围
	ExplosionImpulse = 55000.0f;          // 高吸附牵引力
	bInwardPull = true;                   // 开启向心牵引黑洞机制
	ElementGauge = ELSElementGauge::Light; // 风元素不驻留，默认 1U 触发扩散
}