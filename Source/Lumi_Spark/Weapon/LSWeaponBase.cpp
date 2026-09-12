#include "LSWeaponBase.h"
#include "Weapon/LSRecoilComponent.h"
#include "Core/LSEventBus.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Character/LSCharacterBase.h"
#include "Net/UnrealNetwork.h"
#include "Element/LSElementComponent.h"
#include "Combat/LSDamageCalculator.h"
#include "Engine/DamageEvents.h"

ALSWeaponBase::ALSWeaponBase()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);
	
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
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, FString::Printf(TEXT("❌ CanFire 失败！Ammo=%d, Reloading=%d"), CurrentAmmo, (int32)bIsReloading));
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
	GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Green, TEXT("🔥 枪械正在发射 Hitscan 射线！"));
	if (!CanFire())
	{
		StopFire();
		if (CurrentAmmo <= 0 && CanReload())
		{
			Reload();
		}
		return;
	}
	
	//1，计算是否开镜与双段射线端点
	bool bIsADS = false;
	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		// 判断角色身上是否有 State.ADS 状态或通过 Tag 查询
	}
	
	FVector MuzzleLocation;
	FVector TraceEnd;
	if (!CalculateTraceEndpoints(MuzzleLocation, TraceEnd, bIsADS))
	{
		return;
	}
	
	// 2，本地先行表现
	PlayLocalFireEffects(bIsADS);
	
	// 3，向服务端发送开火请求（服务端权威处理弹药扣除、射线检测、伤害计算）
	Server_Fire(MuzzleLocation, TraceEnd, bIsADS);
}

void ALSWeaponBase::PlayLocalFireEffects(bool bIsADS)
{
	// 播放枪声
	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), FireSound, GetActorLocation());
	}
	
	// 播放枪口火光粒子
	if (MuzzleFlashEmitter && WeaponMesh)
	{
		UGameplayStatics::SpawnEmitterAttached(MuzzleFlashEmitter, WeaponMesh, MuzzleSocketName, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget);
	}
	
	// 播放角色全身开火动作蒙太奇
	if (ALSCharacterBase* OwnerChar = Cast<ALSCharacterBase>(GetOwner()))
	{
		if (CharacterFireMontage && OwnerChar->GetMesh())
		{
			OwnerChar->PlayAnimMontage(CharacterFireMontage);
		}
		
		if (FPArmsFireMontage && OwnerChar->GetFPArmsMesh())
		{
			if (UAnimInstance* ArmsAnimInst = OwnerChar->GetFPArmsMesh()->GetAnimInstance())
			{
				ArmsAnimInst->Montage_Play(FPArmsFireMontage);
			}
		}
	}
	
	// 后坐力与动态散布增长
	if (RecoilComponent)
	{
		RecoilComponent->ApplyRecoil();
	}
	CurrentSpread = FMath::Min(CurrentSpread + SpreadIncreasePerShot, MaxSpread);
	
	OnWeaponFired.Broadcast();
}

// 服务端权威开火处理

bool ALSWeaponBase::Server_Fire_Validate(const FVector_NetQuantize& MuzzleLoc, const FVector_NetQuantize& TraceEnd, bool bIsADS)
{
	return true;
}

void ALSWeaponBase::Server_Fire_Implementation(const FVector_NetQuantize& MuzzleLoc, const FVector_NetQuantize& TraceEnd, bool bIsADS)
{
	if (!CanFire()) return;
	
	// 1，服务端权威扣除弹药
	CurrentAmmo--;
	
	//2，广播给其他远端观察者播放火光与枪声
	Multicast_FireEffects();
	
	//3，执行服务端射线检测
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(GetOwner());
	QueryParams.bTraceComplex = true;
	QueryParams.bReturnPhysicalMaterial = true;
	
	const bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, MuzzleLoc, TraceEnd, ECC_Visibility, QueryParams);
	
	if (bHit)
	{
		// 4，处理命中目标（伤害计算、弱点爆头、元素附着与总线广播）
		ProcessHit(HitResult);
	}
}

// 远端客户端播放开火视听表现
void ALSWeaponBase::Multicast_FireEffects_Implementation()
{
	//如果是本地玩家自己开火，则不重复播放
	if (const APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		if (OwnerPawn->IsLocallyControlled())
		{
			return;
		}
	}
	
	//其他远端客户端播放枪声与火光
	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), FireSound, GetActorLocation());
	}
	if (MuzzleFlashEmitter && WeaponMesh)
	{
		UGameplayStatics::SpawnEmitterAttached(MuzzleFlashEmitter, WeaponMesh, MuzzleSocketName, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget);
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

	//4, 查询受到攻击目标身上是否有元素组件
	ULSElementComponent* TargetElementComp = HitActor->FindComponentByClass<ULSElementComponent>();
	FGameplayTag AuraTag = TargetElementComp ? TargetElementComp->GetPrimaryAuraTag() : FGameplayTag();

	//5, 组装攻击者与防御者战斗属性包
	FLSAttackerStats AttackerStats;
	AttackerStats.Attack = BaseDamage;
	AttackerStats.CritRate = 0.05f; // 默认暴击率5%
	AttackerStats.CritDamage = 0.5f; // 默认暴击伤害+50%
	AttackerStats.ElementalMastery = 100.0f; //默认元素精通100
	AttackerStats.Level = 90; //默认攻击者等级90

	FLSDefenderStats DefenderStats;
	DefenderStats.Level = 90; //默认防御者等级90
	DefenderStats.Defense = 500.0f;
	DefenderStats.ElementalResistance = 0.1f; //默认抗性10

	//调用伤害计算器计算最终伤害
	FLSDamageResult DamageResult = ULSDamageCalculator::CalculateDamage(DamageContext, AttackerStats, DefenderStats, AuraTag);

	//6,服务端权威附加元素附着
	if (TargetElementComp && ElementTag.IsValid())
	{
		TargetElementComp->ApplyElement(GetOwner() ? GetOwner() : this, ElementTag, ElementGauge);
	}

	//7, 触发引擎标准伤害流程
	HitActor->TakeDamage(DamageContext.FinalDamage, FDamageEvent(), (GetOwner() ? GetOwner()->GetInstigatorController() : nullptr), this);

	// 8. 通过全局事件总线解耦广播（UI 准星跳字、音效、怪物扣血统一监听此事件）
	if (ULSEventBus* EventBus = ULSEventBus::Get(this))
	{
		EventBus->OnDamageDealt.Broadcast(DamageContext);
	}
	
	// 9. 回传给开火客户端，触发本地 HUD 准星 HitMarker 闪红与音效
	Client_HitConfirm(DamageContext.bIsHeadshot, DamageContext.FinalDamage);
}

void ALSWeaponBase::Client_HitConfirm_Implementation(bool bIsHeadshot, float FinalDamage)
{
	//客户端收到服务端的权威命中确认，在本地广播伤害上下文。
	if (ULSEventBus* EventBus = ULSEventBus::Get(this))
	{
		FLSDamageContext DamageContext;
		DamageContext.DamageCauser = Cast<AActor>(GetOwner());
		DamageContext.FinalDamage = FinalDamage;
		DamageContext.bIsHeadshot = bIsHeadshot;
		
		EventBus->OnDamageDealt.Broadcast(DamageContext);
	}
}

void ALSWeaponBase::Reload()
{
	if (!CanReload()) return;
	// 停止正在进行的射击并重置后坐力
	StopFire();
	
	//播放换弹音效
	if (ReloadSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ReloadSound, GetActorLocation());
	}
	
	//播放换弹动作蒙太奇
	if (ALSCharacterBase* OwnerChar = Cast<ALSCharacterBase>(GetOwner()))
	{
		if (CharacterReloadMontage && OwnerChar->GetMesh())
		{
			OwnerChar->PlayAnimMontage(CharacterReloadMontage);
		}
		if (FPArmsReloadMontage && OwnerChar->GetFPArmsMesh())
		{
			if (UAnimInstance* ArmsAnimInst = OwnerChar->GetFPArmsMesh()->GetAnimInstance())
			{
				ArmsAnimInst->Montage_Play(FPArmsReloadMontage);
			}
		}
	}
	
	//发送 Server RPC 在服务端倒计时填充弹药
	Server_Reload();
}

bool ALSWeaponBase::Server_Reload_Validate()
{
	return true;
}

void ALSWeaponBase::Server_Reload_Implementation()
{
	if (!CanReload()) return;
	
	bIsReloading = true;
	OnReloadStart.Broadcast();
	
	//服务端倒计时换弹
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

void ALSWeaponBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ALSWeaponBase, CurrentAmmo);
	DOREPLIFETIME(ALSWeaponBase, CurrentReserveAmmo);
}

void ALSWeaponBase::OnRep_CurrentAmmo()
{
	// 客户端收到服务端权威弹药同步，广播委托刷新本地 UMG
	OnAmmoChanged.Broadcast(CurrentAmmo, MagazineSize, CurrentReserveAmmo);
}

void ALSWeaponBase::OnRep_CurrentReserveAmmo()
{
	OnAmmoChanged.Broadcast(CurrentAmmo, MagazineSize, CurrentReserveAmmo);
}