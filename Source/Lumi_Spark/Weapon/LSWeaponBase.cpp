#include "LSWeaponBase.h"
#include "Weapon/LSRecoilComponent.h"
#include "Core/LSEventBus.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
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
	
	// 每帧物理回正程序化枪身物理后坐力回冲
	UpdateProceduralRecoilKick(DeltaTime);

	// 未开火时平滑恢复散布
	if (!bIsFiring && CurrentSpread > BaseSpread)
	{
		CurrentSpread = FMath::FInterpTo(CurrentSpread, BaseSpread, DeltaTime, SpreadRecoveryRate);
	}
}

void ALSWeaponBase::StartFire()
{
	bWantsToFire = true;

	if (!CanFire())
	{
		if (bIsReloading)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, TEXT("⏳ 正在换弹中..."));
		}
		else if (CurrentAmmo <= 0)
		{
			if (CanReload())
			{
				GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Orange, TEXT("⚠️ 弹匣已空，正在自动换弹..."));
				Reload();
			}
			else
			{
				GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("❌ 备弹彻底耗尽！已无弹药！"));
			}
		}
		return;
	}
	
	bIsFiring = true;
	if (RecoilComponent)
	{
		RecoilComponent->StartRecoil();
	}
	
	// 执行单发或首发送发射击
	FireOnce();
	
	// 全自动模式下启动循环定时器
	if (FireMode == ELSFireMode::FullAuto && FireRate > 0.f)
	{
		const float TimeBetweenShots = 60.f / FireRate;
		GetWorldTimerManager().SetTimer(FireTimerHandle, this, &ALSWeaponBase::FireOnce, TimeBetweenShots, true);
	}
}

void ALSWeaponBase::StopFire()
{
	bWantsToFire = false;
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
		GetWorldTimerManager().ClearTimer(FireTimerHandle);
		bIsFiring = false;
		if (CurrentAmmo <= 0 && CanReload())
		{
			GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Orange, TEXT("⚠️ 弹药打光，自动触发换弹！"));
			Reload();
		}
		return;
	}
	
	// 1. 开火时角色的面向对齐 Controller Yaw（确保腰射与连发时身体正对准星，枪口正对目标）
	FVector MuzzleLocation;
	FVector TraceEnd;
	bool bIsADS = false;
	if (ALSCharacterBase* OwnerChar = Cast<ALSCharacterBase>(GetOwner()))
	{
		if (AController* PC = OwnerChar->GetController())
		{
			FRotator DesiredRot = OwnerChar->GetActorRotation();
			DesiredRot.Yaw = PC->GetControlRotation().Yaw;
			OwnerChar->SetActorRotation(DesiredRot);
		}

		if (ULSCameraComponent* Cam = OwnerChar->GetCameraComponent())
		{
			bIsADS = Cam->IsADS();
		}
	}
	
	if (!CalculateTraceEndpoints(MuzzleLocation, TraceEnd, bIsADS))
	{
		return;
	}
	
	// 2. 本地先行视听与后坐力回冲表现
	PlayLocalFireEffects(bIsADS);
	PlayTracerEffect(MuzzleLocation, TraceEnd);
	
	// 客户端本地先行预测扣弹（无限弹药模式下不扣）
	if (!HasAuthority() && !bInfiniteAmmo)
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
	
	// 播放枪口火光粒子（优先 Niagara，备用 Cascade，自适应挂载组件与插槽）
	FName MatchedSocket = NAME_None;
	USceneComponent* AttachComp = GetMuzzleAttachComponent(MatchedSocket);
	FTransform MuzzleTransform;
	GetMuzzleTransform(MuzzleTransform);

	if (MuzzleFlashNiagara)
	{
		UNiagaraComponent* MuzzleNC = nullptr;
		if (AttachComp && MatchedSocket != NAME_None)
		{
			MuzzleNC = UNiagaraFunctionLibrary::SpawnSystemAttached(MuzzleFlashNiagara, AttachComp, MatchedSocket, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true);
		}
		else
		{
			MuzzleNC = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), MuzzleFlashNiagara, MuzzleTransform.GetLocation(), MuzzleTransform.Rotator());
		}
		if (MuzzleNC)
		{
			MuzzleNC->SetVariableBool(FName("User.Trigger"), true);
			MuzzleNC->Activate(true);
		}
	}
	else if (MuzzleFlashEmitter)
	{
		if (AttachComp && MatchedSocket != NAME_None)
		{
			UGameplayStatics::SpawnEmitterAttached(MuzzleFlashEmitter, AttachComp, MatchedSocket, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget);
		}
		else
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleFlashEmitter, MuzzleTransform.GetLocation(), MuzzleTransform.Rotator());
		}
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
	
	// 触发枪身程序化机械后坐力回冲与枪口上跳抖动！
	ApplyProceduralRecoilKick();

	// 后坐力与动态散布增长
	if (RecoilComponent)
	{
		RecoilComponent->ApplyRecoil(bIsADS);
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
	
	// 1，服务端权威扣除弹药（无限弹药模式下不扣）
	if (!bInfiniteAmmo)
	{
		CurrentAmmo = FMath::Max(0, CurrentAmmo - 1);
	}
	
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
	
	// 其他远端客户端播放枪声与火光
	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), FireSound, GetActorLocation());
	}

	FName MatchedSocket = NAME_None;
	USceneComponent* AttachComp = GetMuzzleAttachComponent(MatchedSocket);
	FTransform MuzzleTransform;
	GetMuzzleTransform(MuzzleTransform);

	if (MuzzleFlashNiagara)
	{
		UNiagaraComponent* MuzzleNC = nullptr;
		if (AttachComp && MatchedSocket != NAME_None)
		{
			MuzzleNC = UNiagaraFunctionLibrary::SpawnSystemAttached(MuzzleFlashNiagara, AttachComp, MatchedSocket, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true);
		}
		else
		{
			MuzzleNC = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), MuzzleFlashNiagara, MuzzleTransform.GetLocation(), MuzzleTransform.Rotator());
		}
		if (MuzzleNC)
		{
			MuzzleNC->SetVariableBool(FName("User.Trigger"), true);
			MuzzleNC->Activate(true);
		}
	}
	else if (MuzzleFlashEmitter)
	{
		if (AttachComp && MatchedSocket != NAME_None)
		{
			UGameplayStatics::SpawnEmitterAttached(MuzzleFlashEmitter, AttachComp, MatchedSocket, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget);
		}
		else
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleFlashEmitter, MuzzleTransform.GetLocation(), MuzzleTransform.Rotator());
		}
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
	const int32 AmmoToLoad = bInfiniteAmmo ? AmmoNeeded : FMath::Min(AmmoNeeded, CurrentReserveAmmo);
	
	CurrentAmmo += AmmoToLoad;
	if (!bInfiniteAmmo)
	{
		CurrentReserveAmmo -= AmmoToLoad;
	}
	
	GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Green, FString::Printf(TEXT("✅ 换弹完成！当前弹药: %d / %s"), CurrentAmmo, bInfiniteAmmo ? TEXT("∞") : *FString::FromInt(CurrentReserveAmmo)));

	OnAmmoChanged.Broadcast(CurrentAmmo, MagazineSize, CurrentReserveAmmo);
	OnReloadEnd.Broadcast();

	// 如果换弹前玩家一直长按开火键，换弹完成后自动无缝继续全自动射击！
	if (bWantsToFire && CanFire() && FireMode == ELSFireMode::FullAuto)
	{
		bIsFiring = true;
		FireOnce();
		const float TimeBetweenShots = 60.f / FireRate;
		GetWorldTimerManager().SetTimer(FireTimerHandle, this, &ALSWeaponBase::FireOnce, TimeBetweenShots, true);
	}
}

bool ALSWeaponBase::CanFire() const
{
	return !bIsReloading && (bInfiniteAmmo || CurrentAmmo > 0);
}

bool ALSWeaponBase::CanReload() const
{
	return !bIsReloading && CurrentAmmo < MagazineSize && (bInfiniteAmmo || CurrentReserveAmmo > 0);
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
	
	// 1. 枪口起点位置（多级智能匹配：优先网格体插槽）
	FTransform MuzzleTransform;
	GetMuzzleTransform(MuzzleTransform);
	OutMuzzleLoc = MuzzleTransform.GetLocation();
	
	// 2. 计算散布角（开镜衰减）
	float FinalSpread = CurrentSpread;
	if (bIsADS)
	{
		FinalSpread *= ADSSpreadMultiplier;
	}
	
	if (bFireAlongMuzzleForward)
	{
		// 模式 A：纯正的枪口物理朝向（Muzzle Forward）
		// 子弹严格沿着枪口插槽世界朝向打出，绝无侧向大角度折弯！
		FVector MuzzleForward = MuzzleTransform.GetRotation().Vector();

		// 模型局部轴向容错保护：若模型局部坐标系的 +X 轴恰好被做反（指向后背），点积小于 0 则自动翻转对齐角色前方
		if (FVector::DotProduct(MuzzleForward, OwnerChar->GetActorForwardVector()) < -0.2f)
		{
			MuzzleForward = -MuzzleForward;
		}

		const FVector SpreadDir = FMath::VRandCone(MuzzleForward, FMath::DegreesToRadians(FinalSpread * 0.5f));
		OutTraceEnd = OutMuzzleLoc + SpreadDir * MaxRange;
		return true;
	}
	else
	{
		// 模式 B：摄像机两段式视差对齐
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
		
		const FVector AimDir = CameraRotation.Vector();
		const FVector SpreadDir = FMath::VRandCone(AimDir, FMath::DegreesToRadians(FinalSpread * 0.5f));
		
		// 第一段：从相机向前打超长射线确定准星落点（解决视差）
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
		
		// 第二段：从枪口连接到目标点并延伸
		OutTraceEnd = OutMuzzleLoc + (TargetPoint - OutMuzzleLoc).GetSafeNormal() * MaxRange;
		return true;
	}
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
	
	// 1. 在击中表面法线方向生成撞击火花/碎屑粒子（同时适配常规 Niagara 与 Lyra Array 系统）
	if (ImpactNiagara)
	{
		if (UNiagaraComponent* ImpactComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ImpactNiagara, Hit.ImpactPoint, Hit.ImpactNormal.Rotation()))
		{
			ImpactComp->SetVariableBool(FName("User.Trigger"), true);
			ImpactComp->SetVariableInt(FName("User.NumberOfHits"), 1);
			
			TArray<FVector> ImpactPosArray;
			ImpactPosArray.Add(Hit.ImpactPoint);
			UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayPosition(ImpactComp, FName("User.ImpactPositions"), ImpactPosArray);
			
			TArray<FVector> ImpactNormArray;
			ImpactNormArray.Add(Hit.ImpactNormal);
			UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(ImpactComp, FName("User.ImpactNormals"), ImpactNormArray);
			
			ImpactComp->Activate(true);
		}
	}
	else if (ImpactEmitter)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactEmitter, Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
	}
	
	// 2. 播放撞击音效
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), ImpactSound, Hit.ImpactPoint);
	}
	
	// 3. 在击中表面附着弹孔贴花（支持最多同时存在 MaxDecalCount 个弹孔，先进先出循环复用）
	if (ImpactDecalMaterial)
	{
		FRotator DecalRotation = (-Hit.ImpactNormal).Rotation();
		DecalRotation.Roll = FMath::FRandRange(-180.0f, 180.0f);
		UDecalComponent* SpawnedDecal = nullptr;

		if (Hit.GetComponent())
		{
			SpawnedDecal = UGameplayStatics::SpawnDecalAttached(
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
		else
		{
			SpawnedDecal = UGameplayStatics::SpawnDecalAtLocation(
				GetWorld(),
				ImpactDecalMaterial,
				DecalSize,
				Hit.ImpactPoint,
				DecalRotation,
				DecalLifeSpan
			);
		}

		if (SpawnedDecal)
		{
			// 清理失效弱引用
			ActiveDecals.RemoveAll([](const TWeakObjectPtr<UDecalComponent>& DecalPtr) {
				return !DecalPtr.IsValid();
			});

			// 超出数量上限时，销毁最早生成的弹孔，杜绝堆积卡顿
			while (ActiveDecals.Num() >= MaxDecalCount)
			{
				if (ActiveDecals[0].IsValid())
				{
					ActiveDecals[0]->DestroyComponent();
				}
				ActiveDecals.RemoveAt(0);
			}

			ActiveDecals.Add(SpawnedDecal);
		}
	}
}

void ALSWeaponBase::PlayTracerEffect(const FVector& StartLoc, const FVector& EndLoc)
{
	if (!GetWorld()) return;
	
	// 射线物理阻挡测试：如果中途有障碍物（墙体等），弹道精确止于墙面命中点
	FHitResult ObstacleHit;
	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(this);
	TraceParams.AddIgnoredActor(GetOwner());
	
	FVector VisualEnd = EndLoc;
	if (GetWorld()->LineTraceSingleByChannel(ObstacleHit, StartLoc, EndLoc, ECC_Visibility, TraceParams))
	{
		VisualEnd = ObstacleHit.ImpactPoint;
	}

	// 基础保底：始终绘制高亮金色瞬时射线（持续 0.05 秒），确保任何视角都能看到子弹飞行轨迹！
	DrawDebugLine(GetWorld(), StartLoc, VisualEnd, FColor(255, 215, 0), false, 0.05f, 0, 1.5f);

	// 1. Niagara 曳光粒子（同时兼容普通向量与 Lyra 数组系统）
	if (TracerNiagara)
	{
		if (UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), TracerNiagara, StartLoc))
		{
			NiagaraComp->SetVariableVec3(TracerNiagaraParamName, VisualEnd);
			NiagaraComp->SetVariableVec3(FName("User.TraceEnd"), VisualEnd);
			NiagaraComp->SetVariablePosition(FName("User.MuzzlePosition"), StartLoc);
			NiagaraComp->SetVariableBool(FName("User.Trigger"), true);
			
			TArray<FVector> ImpactPosArray;
			ImpactPosArray.Add(VisualEnd);
			UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayPosition(NiagaraComp, FName("User.ImpactPositions"), ImpactPosArray);
			
			NiagaraComp->Activate(true);
		}
		return;
	}

	// 2. Cascade 曳光粒子
	if (TracerEmitter)
	{
		if (UParticleSystemComponent* TracerComp = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TracerEmitter, StartLoc))
		{
			TracerComp->SetVectorParameter(TracerTargetParamName, VisualEnd);
		}
		return;
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

bool ALSWeaponBase::GetLeftHandSocketTransform(FTransform& OutTransform, FName SocketName) const
{
	// 1. 遍历自身附加的所有 Primitive 组件（包括蓝图里添加的 StaticMeshComponent）
	TInlineComponentArray<UPrimitiveComponent*> PrimComponents(this);
	for (UPrimitiveComponent* Prim : PrimComponents)
	{
		if (Prim && Prim->DoesSocketExist(SocketName))
		{
			OutTransform = Prim->GetSocketTransform(SocketName, RTS_World);
			return true;
		}
	}
	
	// 2. 备用检查默认的 WeaponMesh
	if (WeaponMesh && WeaponMesh->DoesSocketExist(SocketName))
	{
		OutTransform = WeaponMesh->GetSocketTransform(SocketName, RTS_World);
		return true;
	}

	return false;
}

bool ALSWeaponBase::GetMuzzleTransform(FTransform& OutTransform) const
{
	// 优先在所有 Mesh 组件上查找标准的枪口插槽（支持 MuzzleSocketName 与通用 Muzzle / MuzzleFlash）
	static const TArray<FName> CandidateSockets = {
		MuzzleSocketName,
		FName("Muzzle"),
		FName("MuzzleFlash"),
		FName("Socket") // 兼顾当前可能尚未重命名的默认插槽
	};

	TInlineComponentArray<UMeshComponent*> MeshComponents(this);
	for (UMeshComponent* MeshComp : MeshComponents)
	{
		if (!MeshComp) continue;
		for (const FName& SocketCandidate : CandidateSockets)
		{
			if (SocketCandidate != NAME_None && MeshComp->DoesSocketExist(SocketCandidate))
			{
				OutTransform = MeshComp->GetSocketTransform(SocketCandidate, RTS_World);
				return true;
			}
		}
	}

	// 2. 若网格体尚无插槽，基于武器主网格体包围盒进行几何前沿估算（确保子弹与枪火从枪管最前端射出）
	UMeshComponent* BestMeshComp = nullptr;
	for (UMeshComponent* MeshComp : MeshComponents)
	{
		if (UStaticMeshComponent* SMC = Cast<UStaticMeshComponent>(MeshComp))
		{
			if (SMC->GetStaticMesh())
			{
				BestMeshComp = SMC;
				break;
			}
		}
		else if (USkeletalMeshComponent* SKMC = Cast<USkeletalMeshComponent>(MeshComp))
		{
			if (SKMC->GetSkeletalMeshAsset())
			{
				BestMeshComp = SKMC;
				break;
			}
		}
	}

	if (!BestMeshComp && WeaponMesh)
	{
		BestMeshComp = WeaponMesh;
	}

	if (BestMeshComp)
	{
		const FBox LocalBox = BestMeshComp->GetLocalBounds().GetBox();
		// +X 轴为枪口正前方，取最前端 Max.X；Y 轴取居中；Z 轴取偏上 65% 高度（避开弹匣）
		const float ForwardX = LocalBox.Max.X;
		const float CenterY = (LocalBox.Min.Y + LocalBox.Max.Y) * 0.5f;
		const float CenterZ = LocalBox.Min.Z + (LocalBox.Max.Z - LocalBox.Min.Z) * 0.65f;

		const FVector LocalMuzzlePos(ForwardX, CenterY, CenterZ);
		const FVector WorldMuzzlePos = BestMeshComp->GetComponentTransform().TransformPosition(LocalMuzzlePos);
		const FRotator WorldRotation = BestMeshComp->GetComponentRotation();

		OutTransform = FTransform(WorldRotation, WorldMuzzlePos);
		return true;
	}

	// 3. 终极保底：若无任何网格体，取 Actor 沿着朝向前方 50cm
	OutTransform = FTransform(GetActorRotation(), GetActorLocation() + GetActorForwardVector() * 50.0f);
	return false;
}

FVector ALSWeaponBase::GetMuzzleLocation() const
{
	FTransform MuzzleTransform;
	GetMuzzleTransform(MuzzleTransform);
	return MuzzleTransform.GetLocation();
}

USceneComponent* ALSWeaponBase::GetMuzzleAttachComponent(FName& OutSocketName) const
{
	static const TArray<FName> CandidateSockets = {
		MuzzleSocketName,
		FName("Muzzle"),
		FName("MuzzleFlash"),
		FName("Socket")
	};

	TInlineComponentArray<UMeshComponent*> MeshComponents(this);
	for (UMeshComponent* MeshComp : MeshComponents)
	{
		if (!MeshComp) continue;
		for (const FName& SocketCandidate : CandidateSockets)
		{
			if (SocketCandidate != NAME_None && MeshComp->DoesSocketExist(SocketCandidate))
			{
				OutSocketName = SocketCandidate;
				return MeshComp;
			}
		}
	}

	OutSocketName = NAME_None;
	for (UMeshComponent* MeshComp : MeshComponents)
	{
		if (UStaticMeshComponent* SMC = Cast<UStaticMeshComponent>(MeshComp))
		{
			if (SMC->GetStaticMesh()) return SMC;
		}
	}
	return WeaponMesh ? WeaponMesh : RootComponent;
}

void ALSWeaponBase::ApplyProceduralRecoilKick()
{
	// 1. 枪身后坐位移：沿枪管自身局部 -X 轴向后猛挫，Z 轴微量上抬
	CurrentRecoilKickLoc.X = FMath::Clamp(CurrentRecoilKickLoc.X - RecoilKickDistance, -MaxRecoilKickDistance, 0.0f);
	CurrentRecoilKickLoc.Z = FMath::Clamp(CurrentRecoilKickLoc.Z + 0.35f, 0.0f, 1.2f);

	// 2. 枪口上挑与横摆：Pitch 仰角瞬时上挑，随机微量横偏晃动
	CurrentRecoilKickRot.Pitch = FMath::Clamp(CurrentRecoilKickRot.Pitch + RecoilKickPitch, 0.0f, 6.0f);
	CurrentRecoilKickRot.Yaw += FMath::FRandRange(-RecoilKickYaw, RecoilKickYaw);
	CurrentRecoilKickRot.Roll += FMath::FRandRange(-RecoilKickYaw * 0.5f, RecoilKickYaw * 0.5f);
}

void ALSWeaponBase::UpdateProceduralRecoilKick(float DeltaTime)
{
	if (!RootComponent) return;

	// 首次缓存与父级插槽绑定的基础相对变换
	if (!bHasCachedBaseTransform)
	{
		if (GetAttachParentActor() || RootComponent->GetAttachParent())
		{
			BaseAttachRelativeTransform = RootComponent->GetRelativeTransform();
			bHasCachedBaseTransform = true;
		}
	}

	// 阻尼弹性极速平滑回正（弹簧回弹手感）
	CurrentRecoilKickLoc = FMath::VInterpTo(CurrentRecoilKickLoc, FVector::ZeroVector, DeltaTime, RecoilRecoverySpeed);
	CurrentRecoilKickRot = FMath::RInterpTo(CurrentRecoilKickRot, FRotator::ZeroRotator, DeltaTime, RecoilRecoverySpeed);

	// 程序化瞄准俯仰跟随：当玩家向上/向下看时，枪身随之仰俯，消除子弹折射感
	if (bEnableProceduralAimPitch)
	{
		float TargetPitch = 0.0f;
		if (ALSCharacterBase* OwnerChar = Cast<ALSCharacterBase>(GetOwner()))
		{
			bool bIsADS = false;
			if (ULSCameraComponent* Cam = OwnerChar->GetCameraComponent())
			{
				bIsADS = Cam->IsADS();
			}

			// 只有按住右键瞄准 (bIsADS) 或正在开火时才进行枪身程序化俯仰；自由观察视角下枪身平端
			if (bIsADS || bIsFiring)
			{
				if (AController* PC = OwnerChar->GetController())
				{
					const float RawPitch = FRotator::NormalizeAxis(PC->GetControlRotation().Pitch);
					// 核心非对称设计：向上仰视与向下俯视采用独立倍率与极限夹角控制！
					const float CalculatedUpAngle = RawPitch * ProceduralAimPitchUpMultiplier;
					const float ChosenMultiplier = (CalculatedUpAngle >= 0.0f) ? ProceduralAimPitchUpMultiplier : ProceduralAimPitchDownMultiplier;

					TargetPitch = FMath::Clamp(RawPitch * ChosenMultiplier, MinAimPitchLimit, MaxAimPitchLimit);
				}
			}
		}
		CurrentProceduralAimPitch = FMath::FInterpTo(CurrentProceduralAimPitch, TargetPitch, DeltaTime, 18.0f);
	}
	else
	{
		CurrentProceduralAimPitch = 0.0f;
	}

	if (bHasCachedBaseTransform)
	{
		// 计算叠加后坐力、瞄准俯仰与姿态偏移后的局部旋转
		FRotator CombinedRot = CurrentRecoilKickRot + GripRotationOffset;
		CombinedRot.Pitch += CurrentProceduralAimPitch;

		FTransform KickTransform;
		KickTransform.SetLocation(CurrentRecoilKickLoc + GripLocationOffset);
		KickTransform.SetRotation(CombinedRot.Quaternion());

		const FTransform FinalTransform = KickTransform * BaseAttachRelativeTransform;
		RootComponent->SetRelativeTransform(FinalTransform);
	}
}
