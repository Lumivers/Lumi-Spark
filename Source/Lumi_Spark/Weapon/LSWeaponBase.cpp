#include "LSWeaponBase.h"
#include "Weapon/LSRecoilComponent.h"
#include "Core/LSEventBus.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "DrawDebugHelpers.h"

ALSWeaponBase::ALSWeaponBase()
{
	PrimaryActorTick.bCanEverTick = true;
	
	//1，初始化武器网格体
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	RootComponent = WeaponMesh;
	WeaponMesh->SetCollisionProfileName(TEXT("NoCollision"));
	WeaponMesh->SetCastShadow(true);
	
	//2，初始化后坐力组件
	RecoilComponent = CreateDefaultSubobject<ULSRecoilComponent>(TEXT("RecoilComponent"));
	
	//默认标签赋值
	ElementTag = LSTags::TAG_Element_Pyro;
	WeaponTypeTag = FGameplayTag::RequestGameplayTag(FName("Weapon.Rifle"), false);
}

void ALSWeaponBase::BeginPlay()
{
	Super::BeginPlay();
	
	CurrentAmmo = MagazineSize;
	CurrentReserveAmmo = MaxReserveAmmo;
	CurrentSpread = BaseSpread;
}

void ALSWeaponBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	//未开火时平滑恢复散布
	if (!bIsFiring && CurrentSpread > BaseSpread)
	{
		CurrentSpread = FMath::FInterpTo(CurrentSpread, BaseSpread, DeltaTime, SpreadRecoveryRate);
	}
}

void ALSWeaponBase::StartFire()
{
	if (!CanFire())
	{
		//弹夹空了自动触发换弹
		if (CurrentAmmo <= 0 && CanReload())
		{
			Reload();
		}
		return;
	}
	
	bIsFiring = true;
	if (RecoilComponent)
	{
		RecoilComponent->StartRecoil();
	}
	
	//执行射击逻辑
	FireOnce();
	
	//全自动模式下启动循环定时器
	if (FireMode == ELSFireMode::FullAuto && FireRate > 0.f)
	{
		const float TimeBetweenShots = 60.f / FireRate; //每分钟射速转换为每秒间隔
		GetWorldTimerManager().SetTimer(FireTimerHandle, this, &ALSWeaponBase::FireOnce, TimeBetweenShots, true);
	}
}

void ALSWeaponBase::StopFire()
{
	bIsFiring = false;
	GetWorldTimerManager().ClearTimer(FireTimerHandle);
	
	if (RecoilComponent)
	{
		RecoilComponent->StopRecoil();
	}
}

void ALSWeaponBase::FireOnce()
{
	if (!CanFire())
	{
		StopFire();
		if (CurrentAmmo <= 0 && CanReload())
		{
			Reload();
		}
		return;
	}
	
	//1，扣除弹药并广播
	CurrentAmmo--;
	OnAmmoChanged.Broadcast(CurrentAmmo, MagazineSize, CurrentReserveAmmo);
	OnWeaponFired.Broadcast();
	
	//2，检测当前是否处于开镜状态（计算ADS散布和后坐力）
	bool bIsADS = false;
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn)
	{
		// 判断角色身上是否有 State.ADS 状态或通过 Tag 查询
	}
	
	// 3. 计算双段 Hitscan 射线端点
	FVector MuzzleLocation;
	FVector TraceEnd;
	if (CalculateTraceEndpoints(MuzzleLocation, TraceEnd, bIsADS))
	{
		FHitResult HitResult;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);
		QueryParams.AddIgnoredActor(GetOwner());
		QueryParams.bTraceComplex = true;
		QueryParams.bReturnPhysicalMaterial = true;
		
		// 使用 ECC_Visibility 通道进行命中检测
		const bool bHit = GetWorld()->LineTraceSingleByChannel(
			HitResult,
			MuzzleLocation,
			TraceEnd,
			ECC_Visibility,
			QueryParams
		);
		
		FVector VisualEnd = bHit ? HitResult.ImpactPoint : TraceEnd;
		DrawDebugLine(GetWorld(), MuzzleLocation, VisualEnd, FColor::Orange, false, 0.5f, 0, 2.0f);
		
		if (bHit)
		{
			DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, 8.0f, 12, FColor::Red, false, 0.5f);
		}
	}
	
	// 4. 触发后坐力与动态散布增长
	if (RecoilComponent)
	{
		RecoilComponent->ApplyRecoil(bIsADS);
	}
	CurrentSpread = FMath::Min(CurrentSpread + SpreadIncreasePerShot, MaxSpread);
	
	// 若弹药打空，自动停火
	if (CurrentAmmo <= 0)
	{
		StopFire();
	}
}
void ALSWeaponBase::ProcessHit(const FHitResult& Hit)
{
	AActor* HitActor = Hit.GetActor();
	if (!HitActor) return;
	
	// 1. 计算距离衰减
	const float Distance = (Hit.ImpactPoint - GetActorLocation()).Size();
	const float Dropoff = CalculateDamageDropoff(Distance);
	float FinalDamage = BaseDamage * Dropoff;
	
	// 2. 弱点 / 爆头检测（骨骼名为 head 或专用弱点碰撞体）
	bool bIsHeadshot = false;
	if (Hit.BoneName.ToString().Contains(TEXT("head"), ESearchCase::IgnoreCase))
	{
		FinalDamage *= HeadshotMultiplier;
		bIsHeadshot = true;
	}
	
	// 3. 组装全局伤害上下文 FLSDamageContext
	FLSDamageContext DamageContext;
	DamageContext.DamageCauser = GetOwner() ? GetOwner() : this;
	DamageContext.TargetActor = HitActor;
	DamageContext.BaseDamage = BaseDamage;
	DamageContext.FinalDamage = FinalDamage;
	DamageContext.ElementTag = ElementTag;
	DamageContext.DamageTypeTag = LSTags::TAG_Damage_Type_Bullet;
	DamageContext.bIsHeadshot = bIsHeadshot;
	DamageContext.HitResult = Hit;
	
	// 4. 通过全局事件总线解耦广播（UI 准星跳字、音效、怪物扣血统一监听此事件）
	if (ULSEventBus* EventBus = ULSEventBus::Get(this))
	{
		EventBus->OnDamageDealt.Broadcast(DamageContext);
	}
}

void ALSWeaponBase::Reload()
{
	if (!CanReload()) return;
	// 停止正在进行的射击并重置后坐力
	StopFire();
	bIsReloading = true;
	OnReloadStart.Broadcast();
	GetWorldTimerManager().SetTimer(ReloadTimerHandle, this, &ALSWeaponBase::FinishReload, ReloadTime, false);
}

void ALSWeaponBase::FinishReload()
{
	bIsReloading = false;
	
	const int32 AmmoNeeded = MagazineSize - CurrentAmmo;
	const int32 AmmoToLoad = FMath::Min(AmmoNeeded, CurrentReserveAmmo);
	
	CurrentAmmo += AmmoToLoad;
	CurrentReserveAmmo -= AmmoToLoad;
	
	OnAmmoChanged.Broadcast(CurrentAmmo, MagazineSize, CurrentReserveAmmo);
	OnReloadEnd.Broadcast();
}
bool ALSWeaponBase::CanFire() const
{
	return !bIsReloading && CurrentAmmo > 0;
}

bool ALSWeaponBase::CanReload() const
{
	return !bIsReloading && CurrentAmmo < MagazineSize && CurrentReserveAmmo > 0;
}

float ALSWeaponBase::CalculateDamageDropoff(float Distance) const
{
	if (Distance <= DamageDropoffStart) return 1.0f;
	if (Distance >= DamageDropoffEnd)   return MinDamageMultiplier;
	
	const float Alpha = (Distance - DamageDropoffStart) / (DamageDropoffEnd - DamageDropoffStart);
	return FMath::Lerp(1.0f, MinDamageMultiplier, Alpha);
}

bool ALSWeaponBase::CalculateTraceEndpoints(FVector& OutMuzzleLoc, FVector& OutTraceEnd, bool bIsADS) const
{
	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!OwnerChar) return false;
	
	// 1. 枪口起点位置
	OutMuzzleLoc = WeaponMesh->DoesSocketExist(MuzzleSocketName) 
		? WeaponMesh->GetSocketLocation(MuzzleSocketName) 
		: GetActorLocation();
	
	// 2. 从摄像机视角获取视线中心与朝向
	FVector CameraLocation;
	FRotator CameraRotation;
	if (APlayerController* PC = Cast<APlayerController>(OwnerChar->GetController()))
	{
		PC->GetPlayerViewPoint(CameraLocation, CameraRotation);
	}
	else
	{
		CameraLocation = OwnerChar->GetActorLocation() + FVector(0, 0, 60.0f);
		CameraRotation = OwnerChar->GetActorRotation();
	}
	
	// 3. 计算散布角（开镜衰减）
	float FinalSpread = CurrentSpread;
	if (bIsADS)
	{
		FinalSpread *= ADSSpreadMultiplier;
	}
	
	const FVector AimDir = CameraRotation.Vector();
	const FVector SpreadDir = FMath::VRandCone(AimDir, FMath::DegreesToRadians(FinalSpread * 0.5f));
	
	// 4. 第一段：从相机向前打超长射线确定准星落点（解决视差）
	const FVector CamTraceEnd = CameraLocation + SpreadDir * MaxRange;
	FHitResult CamHit;
	FCollisionQueryParams CamParams;
	CamParams.AddIgnoredActor(this);
	CamParams.AddIgnoredActor(OwnerChar);
	
	FVector TargetPoint = CamTraceEnd;
	if (GetWorld()->LineTraceSingleByChannel(CamHit, CameraLocation, CamTraceEnd, ECC_Visibility, CamParams))
	{
		TargetPoint = CamHit.ImpactPoint;
	}
	
	// 5. 第二段：从枪口连接到目标点并延伸
	OutTraceEnd = OutMuzzleLoc + (TargetPoint - OutMuzzleLoc).GetSafeNormal() * MaxRange;
	return true;
}