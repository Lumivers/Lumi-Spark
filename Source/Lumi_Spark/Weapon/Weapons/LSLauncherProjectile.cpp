#include "LSLauncherProjectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Core/LSEventBus.h"
#include "Element/LSElementComponent.h"
#include "Engine/World.h"
#include "Engine/DamageEvents.h"
#include "Engine/OverlapResult.h"

ALSLauncherProjectile::ALSLauncherProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	CollisionSphere->InitSphereRadius(12.0f);
	CollisionSphere->SetCollisionProfileName(TEXT("Projectile"));
	CollisionSphere->OnComponentHit.AddDynamic(this, &ALSLauncherProjectile::OnProjectileHit);
	RootComponent = CollisionSphere;

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	ProjectileMesh->SetupAttachment(CollisionSphere);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionSphere;
	ProjectileMovement->InitialSpeed = 2600.0f;
	ProjectileMovement->MaxSpeed = 2600.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = true;
	ProjectileMovement->Bounciness = 0.25f;
	ProjectileMovement->Friction = 0.5f;
	ProjectileMovement->ProjectileGravityScale = 1.0f;
}

void ALSLauncherProjectile::BeginPlay()
{
	Super::BeginPlay();
	SetLifeSpan(8.0f); // 8 秒保底销毁
}

void ALSLauncherProjectile::InitializeProjectile(float InDamage, FGameplayTag InElement, ELSElementGauge InGauge, AActor* InShooter)
{
	ExplosionDamage = InDamage;
	ElementTag = InElement;
	ElementGauge = InGauge;
	ShooterActor = InShooter;
}

void ALSLauncherProjectile::OnProjectileHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (bHasExploded) return;

	// 若撞击到 Pawn 角色（敌人或木桩），直接瞬发引爆！
	if (OtherActor && OtherActor != ShooterActor && OtherActor->IsA<APawn>())
	{
		Explode();
		return;
	}

	// 若撞击到静态掩体/地面，启动延时自爆引信
	if (!GetWorldTimerManager().IsTimerActive(FuseTimerHandle))
	{
		GetWorldTimerManager().SetTimer(FuseTimerHandle, this, &ALSLauncherProjectile::Explode, FuseDelay, false);
	}
}

void ALSLauncherProjectile::Explode()
{
	if (bHasExploded) return;
	bHasExploded = true;

	const FVector ExplosionLoc = GetActorLocation();

	// 1. 视听表现
	if (ExplosionFX)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionFX, ExplosionLoc, FRotator::ZeroRotator, FVector(1.5f));
	}
	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, ExplosionLoc);
	}

	// 2. 权威端执行防穿墙视线碰撞与伤害结算
	if (HasAuthority())
	{
		TArray<FOverlapResult> Overlaps;
		FCollisionShape Sphere = FCollisionShape::MakeSphere(ExplosionRadius);
		FCollisionQueryParams OverlapParams;
		OverlapParams.AddIgnoredActor(this);
		if (ShooterActor) OverlapParams.AddIgnoredActor(ShooterActor);

		GetWorld()->OverlapMultiByChannel(Overlaps, ExplosionLoc, FQuat::Identity, ECC_Pawn, Sphere, OverlapParams);

		TSet<AActor*> DamagedActors;
		for (const FOverlapResult& Overlap : Overlaps)
		{
			AActor* Victim = Overlap.GetActor();
			if (!Victim || DamagedActors.Contains(Victim)) continue;

			// 视线防穿墙遮挡检测 (Line of Sight Raycast)
			FHitResult HitCheck;
			FCollisionQueryParams CheckParams;
			CheckParams.AddIgnoredActor(this);
			CheckParams.AddIgnoredActor(Victim);

			const bool bBlocked = GetWorld()->LineTraceSingleByChannel(HitCheck, ExplosionLoc, Victim->GetActorLocation(), ECC_Visibility, CheckParams);
			if (bBlocked) continue;

			DamagedActors.Add(Victim);

			// 径向线性伤害衰减
			const float Dist = FVector::Dist(ExplosionLoc, Victim->GetActorLocation());
			const float DamagePercent = FMath::Clamp(1.0f - (Dist / ExplosionRadius), 0.3f, 1.0f);
			const float FinalDmg = ExplosionDamage * DamagePercent;

			// 附加 2U 强元素
			if (ULSElementComponent* TargetElemComp = Victim->FindComponentByClass<ULSElementComponent>())
			{
				if (ElementTag.IsValid())
				{
					TargetElemComp->ApplyElement(ShooterActor ? ShooterActor : this, ElementTag, ElementGauge);
				}
			}

			// 扣除生命
			Victim->TakeDamage(FinalDmg, FDamageEvent(), ShooterActor ? ShooterActor->GetInstigatorController() : nullptr, ShooterActor ? ShooterActor : this);

			// 全局总线广播
			if (ULSEventBus* EventBus = ULSEventBus::Get(this))
			{
				FLSDamageContext Context;
				Context.DamageCauser = ShooterActor;
				Context.TargetActor = Victim;
				Context.FinalDamage = FinalDmg;
				Context.ElementTag = ElementTag;
				Context.DamageTypeTag = LSTags::TAG_Damage_Type_Explosion;
				EventBus->OnDamageDealt.Broadcast(Context);
			}
		}
	}

	Destroy();
}