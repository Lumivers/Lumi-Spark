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
#include "Components/DecalComponent.h"
#include "Character/LSCameraComponent.h"
#include "Weapon/LSWeaponDataAsset.h"
#include "Core/LSPlayerController.h"
#include "Equipment/ULSDriveCoreComponent.h"
#include "Enemy/LSEnemyBase.h"
#include "Enemy/LSEnemyDataAsset.h"

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
	FVector MuzzleLocation;
	FVector TraceEnd;
	bool bIsADS = false;
	if (ALSCharacterBase* OwnerChar = Cast<ALSCharacterBase>(GetOwner()))
	{
		if (ULSCameraComponent* Cam = OwnerChar->GetCameraComponent())
		{
			bIsADS = Cam->IsADS();
		}
	}
	
	if (!CalculateTraceEndpoints(MuzzleLocation, TraceEnd, bIsADS))
	{
		return;
	}
	
	// 2，本地先行表现
	PlayLocalFireEffects(bIsADS);
	PlayTracerEffect(MuzzleLocation, TraceEnd);
	
	// 客户端本地先行预测扣弹
	if (!HasAuthority())
	{
		CurrentAmmo = FMath::Max(0, CurrentAmmo - 1);
		OnAmmoChanged.Broadcast(CurrentAmmo, MagazineSize, CurrentReserveAmmo);
	}
	
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
	
	// 广播通知本地HUD刷新弹药数字
	OnAmmoChanged.Broadcast(CurrentAmmo, MagazineSize, CurrentReserveAmmo);
	
	//2，广播给其他远端观察者播放火光与枪声
	Multicast_FireEffects(MuzzleLoc, TraceEnd);
	
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
		
		// 5，广播击中表面物理表现给所有客户端
		Multicast_ImpactEffects(HitResult);
	}
}

// 远端客户端播放开火视听表现
void ALSWeaponBase::Multicast_FireEffects_Implementation(const FVector_NetQuantize& MuzzleLoc, const FVector_NetQuantize& TraceEnd)
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
	
	// 播放子弹曳光和枪口粒子
	PlayTracerEffect(MuzzleLoc, TraceEnd);
}

void ALSWeaponBase::Multicast_ImpactEffects_Implementation(const FHitResult& Hit)
{
	//播放击中表面物理表现（火花、音效、贴花）
	PlayImpactEffects(Hit);
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
	if (Hit.BoneName.ToString().Contains(TEXT("head"), ESearchCase::IgnoreCase) || (Hit.Component.IsValid() && Hit.Component->ComponentHasTag(TEXT("head"))))
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

float ALSWeaponBase::GetReloadSpeedMultiplier() const
{
	if (ALSCharacterBase* OwnerChar = Cast<ALSCharacterBase>(GetOwner()))
	{
		if (ALSPlayerController* PC = Cast<ALSPlayerController>(OwnerChar->GetController()))
		{
			if (ULSDriveCoreComponent* DriveCore = PC->GetDriveCoreComponent())
			{
				const FLSCombatAttributes Attr = DriveCore->CalculateCombatAttributes(OwnerChar);
				return 1.0f + Attr.ReloadSpeedBonus;
			}
		}
	}
	return 1.0f;
}

void ALSWeaponBase::Reload()
{
	if (!CanReload()) return;
	StopFire();
	
	if (ReloadSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ReloadSound, GetActorLocation());
	}
	
	// 动态获取换弹倍率，动作按比例加速播放！
	const float AnimSpeed = GetReloadSpeedMultiplier();

	if (ALSCharacterBase* OwnerChar = Cast<ALSCharacterBase>(GetOwner()))
	{
		if (CharacterReloadMontage && OwnerChar->GetMesh())
		{
			OwnerChar->PlayAnimMontage(CharacterReloadMontage, AnimSpeed);
		}
		if (FPArmsReloadMontage && OwnerChar->GetFPArmsMesh())
		{
			if (UAnimInstance* ArmsAnimInst = OwnerChar->GetFPArmsMesh()->GetAnimInstance())
			{
				ArmsAnimInst->Montage_Play(FPArmsReloadMontage, AnimSpeed);
			}
		}
	}
	
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
	
	// 服务端使用缩短后的时间倒计时填充弹药
	const float EffectiveTime = ReloadTime / GetReloadSpeedMultiplier();
	GetWorldTimerManager().SetTimer(ReloadTimerHandle, this, &ALSWeaponBase::FinishReload, EffectiveTime, false);
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

void ALSWeaponBase::PlayImpactEffects(const FHitResult& Hit)
{
	if (!GetWorld()) return;
	
	// 1. 在击中表面法线方向生成撞击火花/碎屑粒子
	if (ImpactEmitter)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactEmitter, Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
	}
	
	// 2. 播放撞击音效
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), ImpactSound, Hit.ImpactPoint);
	}
	
	// 3. 在击中表面附着弹孔贴花（支持附着在移动物体上）
	if (ImpactDecalMaterial && Hit.GetComponent())
	{
		// 贴花投影方向沿法线反向切入
		const FRotator DecalRotation = Hit.ImpactNormal.Rotation() + FRotator(-90.0f, 0.0f, 0.0f);
		UGameplayStatics::SpawnDecalAttached(
			ImpactDecalMaterial,
			DecalSize,
			Hit.GetComponent(),
			Hit.BoneName,
			Hit.ImpactPoint,
			DecalRotation,
			EAttachLocation::KeepWorldPosition,
			DecalLifeSpan
		);
	}
}

void ALSWeaponBase::PlayTracerEffect(const FVector& StartLoc, const FVector& EndLoc)
{
	if (!TracerEmitter || !GetWorld()) return;
	
	// 生成曳光光束粒子，并设置 Target 向量为射线终点
	if (UParticleSystemComponent* TracerComp = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TracerEmitter, StartLoc))
	{
		TracerComp->SetVectorParameter(TracerTargetParamName, EndLoc);
	}
}

void ALSWeaponBase::InitializeFromDataAsset(const ULSWeaponDataAsset* InDataAsset)
{
	if (!InDataAsset)
	{
		return;
	}

	WeaponDisplayName = InDataAsset->DisplayName;
	WeaponTypeTag = InDataAsset->WeaponTypeTag;
	ElementTag = InDataAsset->ElementTag;
	ElementGauge = InDataAsset->ElementGauge;
	FireMode = InDataAsset->FireMode;

	FireRate = InDataAsset->FireRate;
	BaseDamage = InDataAsset->BaseDamage;
	HeadshotMultiplier = InDataAsset->HeadshotMultiplier;
	MaxRange = InDataAsset->MaxRange;
	DamageDropoffStart = InDataAsset->DamageDropoffStart;
	DamageDropoffEnd = InDataAsset->DamageDropoffEnd;
	MinDamageMultiplier = InDataAsset->MinDamageMultiplier;

	MagazineSize = InDataAsset->MagazineSize;
	MaxReserveAmmo = InDataAsset->MaxReserveAmmo;
	ReloadTime = InDataAsset->ReloadTime;
	CurrentAmmo = MagazineSize;
	CurrentReserveAmmo = MaxReserveAmmo;

	BaseSpread = InDataAsset->BaseSpread;
	MaxSpread = InDataAsset->MaxSpread;
	SpreadIncreasePerShot = InDataAsset->SpreadIncreasePerShot;
	SpreadRecoveryRate = InDataAsset->SpreadRecoveryRate;
	ADSSpreadMultiplier = InDataAsset->ADSSpreadMultiplier;
	CurrentSpread = BaseSpread;

	// 后坐力序列与倍率灌注
	if (RecoilComponent)
	{
		RecoilComponent->SetRecoilPattern(InDataAsset->RecoilPattern);
	}

	// 软引用同步/异步加载装配（若已加载则直接应用骨骼网格体）
	if (InDataAsset->WeaponMeshAsset.IsValid())
	{
		if (WeaponMesh)
		{
			WeaponMesh->SetSkeletalMesh(InDataAsset->WeaponMeshAsset.Get());
		}
	}

	// 广播一次弹药更新，确保 HUD 同步
	OnAmmoChanged.Broadcast(CurrentAmmo, MagazineSize, CurrentReserveAmmo);
}