#include "Element/LSDendroCore.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Combat/LSDamageCalculator.h"
#include "Core/LSEventBus.h"
#include "Weapon/LSWeaponBase.h"
#include "Weapon/LSGrenadeBase.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/OverlapResult.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/Character.h"

ALSDendroCore::ALSDendroCore()
{
	PrimaryActorTick.bCanEverTick = false; // 平时不 Tick，靠 Timer 和物理模拟驱动
	bReplicates = true;
	SetReplicateMovement(true);

	// 1. 初始化物理碰撞球体（弹性阻尼与真实碰撞）
	SphereCollision = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollision"));
	SphereCollision->InitSphereRadius(25.0f);
	SphereCollision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	SphereCollision->SetSimulatePhysics(true);
	SphereCollision->SetLinearDamping(0.8f);
	SphereCollision->SetAngularDamping(0.8f);
	RootComponent = SphereCollision;

	// 2. 网格模型挂载
	CoreMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoreMesh"));
	CoreMesh->SetupAttachment(SphereCollision);
	CoreMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 3. 追踪飞弹组件（初始挂起，仅在触发超绽放时激活）
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->bAutoActivate = false;
	ProjectileMovement->InitialSpeed = 1600.0f;
	ProjectileMovement->MaxSpeed = 2400.0f;
	ProjectileMovement->bIsHomingProjectile = true;
	ProjectileMovement->HomingAccelerationMagnitude = 20000.0f; // 极速转向锁定
	ProjectileMovement->ProjectileGravityScale = 0.0f;          // 飞弹忽略重力
}

void ALSDendroCore::BeginPlay()
{
	Super::BeginPlay();

	SphereCollision->OnComponentHit.AddDynamic(this, &ALSDendroCore::OnCoreHit);

	// 服务端权威开启 6 秒超时自爆计时器
	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(LifeTimerHandle, this, &ALSDendroCore::ExplodeBloom, LifeTime, false);
	}
}

void ALSDendroCore::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(LifeTimerHandle);
	Super::EndPlay(EndPlayReason);
}

void ALSDendroCore::InitializeCore(AActor* InSpawner, int32 InLevel, float InEM)
{
	SpawnerActor = InSpawner;
	SpawnerLevel = InLevel;
	SpawnerEM = InEM;
}

float ALSDendroCore::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	if (!HasAuthority() || bHasExploded) return 0.0f;

	// 提取攻击源携带的元素属性
	FGameplayTag IncomingElement;
	if (ALSWeaponBase* Weapon = Cast<ALSWeaponBase>(DamageCauser))
	{
		IncomingElement = Weapon->GetElementTag();
	}
	else if (ALSGrenadeBase* Grenade = Cast<ALSGrenadeBase>(DamageCauser))
	{
		IncomingElement = Grenade->GetElementTag();
	}

	if (IncomingElement.IsValid())
	{
		AActor* InstigatorPawn = EventInstigator ? EventInstigator->GetPawn() : DamageCauser;
		ApplyReactionTrigger(IncomingElement, InstigatorPawn);
	}

	return DamageAmount;
}

void ALSDendroCore::ApplyReactionTrigger(const FGameplayTag& TriggerElement, AActor* InstigatorActor)
{
	if (!HasAuthority() || bHasExploded) return;

	// 遇火 ➔ 触发烈绽放 (Burgeon)
	if (TriggerElement == LSTags::TAG_Element_Pyro)
	{
		TriggerBurgeon(InstigatorActor);
	}
	// 遇雷 ➔ 触发超绽放 (Hyperbloom)
	else if (TriggerElement == LSTags::TAG_Element_Electro)
	{
		TriggerHyperbloom(InstigatorActor);
	}
}

void ALSDendroCore::ExplodeBloom()
{
	if (!HasAuthority() || bHasExploded) return;
	bHasExploded = true;

	GetWorldTimerManager().ClearTimer(LifeTimerHandle);

	// 计算原绽放基础伤害（2.0x 剧变系数）
	const float ReactionDamage = ULSDamageCalculator::CalculateTransformativeReactionDamage(
		LSTags::TAG_Reaction_Bloom, SpawnerLevel, SpawnerEM, 0.1f, 0.0f
	);

	// 范围伤害判定
	TArray<FOverlapResult> Overlaps;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(BloomRadius);
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	if (GetWorld()->OverlapMultiByChannel(Overlaps, GetActorLocation(), FQuat::Identity, ECC_Pawn, Sphere, Params))
	{
		for (const FOverlapResult& Overlap : Overlaps)
		{
			if (AActor* Target = Overlap.GetActor())
			{
				Target->TakeDamage(ReactionDamage, FDamageEvent(), nullptr, SpawnerActor.Get());
			}
		}
	}

	// 广播事件
	if (ULSEventBus* EventBus = ULSEventBus::Get(this))
	{
		EventBus->OnElementReactionTriggered.Broadcast(this, LSTags::TAG_Reaction_Bloom, ReactionDamage, SpawnerActor.Get());
	}

	Destroy();
}

void ALSDendroCore::TriggerBurgeon(AActor* InstigatorActor)
{
	if (!HasAuthority() || bHasExploded) return;
	bHasExploded = true;

	GetWorldTimerManager().ClearTimer(LifeTimerHandle);

	// 计算烈绽放大额伤害（3.0x 剧变系数）
	const float ReactionDamage = ULSDamageCalculator::CalculateTransformativeReactionDamage(
		LSTags::TAG_Reaction_Burgeon, SpawnerLevel, SpawnerEM, 0.1f, 0.0f
	);

	const FVector Origin = GetActorLocation();

	// 5米半径火草混合大爆轰
	TArray<FOverlapResult> Overlaps;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(BurgeonRadius);
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	if (GetWorld()->OverlapMultiByChannel(Overlaps, Origin, FQuat::Identity, ECC_Pawn, Sphere, Params))
	{
		for (const FOverlapResult& Overlap : Overlaps)
		{
			if (AActor* Target = Overlap.GetActor())
			{
				Target->TakeDamage(ReactionDamage, FDamageEvent(), nullptr, InstigatorActor);
				// 对角色施加爆轰击退
				if (ACharacter* Char = Cast<ACharacter>(Target))
				{
					FVector Dir = (Target->GetActorLocation() - Origin).GetSafeNormal();
					Dir.Z = 0.4f;
					Char->LaunchCharacter(Dir * 700.0f, true, true);
				}
			}
		}
	}

	if (ULSEventBus* EventBus = ULSEventBus::Get(this))
	{
		EventBus->OnElementReactionTriggered.Broadcast(this, LSTags::TAG_Reaction_Burgeon, ReactionDamage, InstigatorActor);
	}

	Destroy();
}

void ALSDendroCore::TriggerHyperbloom(AActor* InstigatorActor)
{
	if (!HasAuthority() || bHasExploded || bIsHyperbloomHoming) return;

	GetWorldTimerManager().ClearTimer(LifeTimerHandle);

	// 寻找 15 米范围内最近的目标
	AActor* Target = FindNearestTarget(1500.0f);

	// 关闭物理刚体模拟，转由飞弹组件接管动力
	SphereCollision->SetSimulatePhysics(false);
	SphereCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	SphereCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SphereCollision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);

	bIsHyperbloomHoming = true;

	// 向上弹起 1.5 米后锁定目标俯冲
	FVector LaunchVel = FVector(0.f, 0.f, 600.f);
	if (Target)
	{
		ProjectileMovement->HomingTargetComponent = Target->GetRootComponent();
		FVector DirToTarget = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal();
		LaunchVel += DirToTarget * 800.0f;
	}
	else
	{
		LaunchVel += GetActorForwardVector() * 1000.0f;
	}

	ProjectileMovement->Velocity = LaunchVel;
	ProjectileMovement->Activate();

	// 5 秒内若未命中目标则超时销毁
	SetLifeSpan(5.0f);
}

void ALSDendroCore::OnCoreHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// 仅在超绽放飞行模式下撞击敌人爆炸
	if (!HasAuthority() || !bIsHyperbloomHoming || bHasExploded) return;

	if (OtherActor && OtherActor != this && OtherActor != SpawnerActor.Get())
	{
		bHasExploded = true;

		// 3.0x 超绽放单体巨额伤害
		const float ReactionDamage = ULSDamageCalculator::CalculateTransformativeReactionDamage(
			LSTags::TAG_Reaction_Hyperbloom, SpawnerLevel, SpawnerEM, 0.1f, 0.0f
		);

		OtherActor->TakeDamage(ReactionDamage, FDamageEvent(), nullptr, SpawnerActor.Get());

		if (ULSEventBus* EventBus = ULSEventBus::Get(this))
		{
			EventBus->OnElementReactionTriggered.Broadcast(OtherActor, LSTags::TAG_Reaction_Hyperbloom, ReactionDamage, SpawnerActor.Get());
		}

		Destroy();
	}
}

AActor* ALSDendroCore::FindNearestTarget(float SearchRadius)
{
	TArray<FOverlapResult> Overlaps;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(SearchRadius);
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	if (SpawnerActor.IsValid()) Params.AddIgnoredActor(SpawnerActor.Get());

	AActor* ClosestActor = nullptr;
	float MinDistSq = MAX_flt;

	if (GetWorld()->OverlapMultiByChannel(Overlaps, GetActorLocation(), FQuat::Identity, ECC_Pawn, Sphere, Params))
	{
		for (const FOverlapResult& Overlap : Overlaps)
		{
			if (AActor* Candidate = Overlap.GetActor())
			{
				const float DistSq = FVector::DistSquared(Candidate->GetActorLocation(), GetActorLocation());
				if (DistSq < MinDistSq)
				{
					MinDistSq = DistSq;
					ClosestActor = Candidate;
				}
			}
		}
	}

	return ClosestActor;
}