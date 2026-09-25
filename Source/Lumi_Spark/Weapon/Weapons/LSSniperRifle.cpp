#include "LSSniperRifle.h"
#include "Character/LSCharacterBase.h"
#include "Character/LSCameraComponent.h"
#include "Core/LSTypes.h"
#include "Combat/LSDamageCalculator.h"
#include "Element/LSElementComponent.h"
#include "Core/LSEventBus.h"
#include "Engine/DamageEvents.h"
#include "Core/LSPlayerController.h"
#include "Equipment/ULSDriveCoreComponent.h"

ALSSniperRifle::ALSSniperRifle()
{
	PrimaryActorTick.bCanEverTick = true;

	WeaponDisplayName = FText::FromString(TEXT("极星重型狙击步枪"));
	WeaponTypeTag = LSTags::TAG_Weapon_Type_Sniper;

	FireMode = ELSFireMode::SemiAuto;
	FireRate = 45.0f;                   // 45 RPM 重型栓动节拍
	BaseDamage = 120.0f;                // 120 点高额白值（满蓄达 264！）
	HeadshotMultiplier = 3.0f;         // 3.0x 爆头（满蓄爆头近 800 点天顶伤害！）
	MagazineSize = 5;
	CurrentAmmo = 5;
	MaxReserveAmmo = 30;
	CurrentReserveAmmo = 30;
	ReloadTime = 2.6f;

	// 超远射程与保底
	MaxRange = 25000.0f;
	DamageDropoffStart = 8000.0f;
	DamageDropoffEnd = 20000.0f;
	MinDamageMultiplier = 0.9f;

	// 极度精准（开镜 0 散布，腰射极散）
	BaseSpread = 0.05f;
	MaxSpread = 5.5f;
	ADSSpreadMultiplier = 0.0f;
}

void ALSSniperRifle::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 仅在开镜瞄准且未处于换弹时蓄力
	bool bIsADS = false;
	if (ALSCharacterBase* OwnerChar = Cast<ALSCharacterBase>(GetOwner()))
	{
		if (ULSCameraComponent* Cam = OwnerChar->GetCameraComponent())
		{
			bIsADS = Cam->IsADS();
		}
	}

	if (bIsADS && !bIsReloading && CurrentAmmo > 0)
	{
		CurrentChargeTime = FMath::Min(CurrentChargeTime + DeltaTime, MaxChargeTime);
		OnChargeRatioChanged.Broadcast(GetCurrentChargeRatio());
	}
	else if (CurrentChargeTime > 0.0f)
	{
		CurrentChargeTime = 0.0f;
		OnChargeRatioChanged.Broadcast(0.0f);
	}
}

float ALSSniperRifle::GetCurrentChargeMultiplier() const
{
	return FMath::Lerp(1.0f, MaxChargeMultiplier, GetCurrentChargeRatio());
}

void ALSSniperRifle::FireOnce()
{
	// 锁定本发射击的蓄力增伤倍率，开火后清零
	LastFiredChargeMultiplier = GetCurrentChargeMultiplier();
	Super::FireOnce();
	CurrentChargeTime = 0.0f;
	OnChargeRatioChanged.Broadcast(0.0f);
}

void ALSSniperRifle::ProcessHit(const FHitResult& Hit)
{
	AActor* HitActor = Hit.GetActor();
	if (!HitActor) return;

	// 1. 计算距离衰减与爆头判定
	const float Distance = (Hit.ImpactPoint - GetActorLocation()).Size();
	const float Dropoff = CalculateDamageDropoff(Distance);

	bool bIsHeadshot = false;
	if (Hit.BoneName.ToString().Contains(TEXT("head"), ESearchCase::IgnoreCase) || 
		(Hit.Component.IsValid() && Hit.Component->ComponentHasTag(TEXT("head"))))
	{
		bIsHeadshot = true;
	}

	// 2. 组装伤害上下文（只传入距离衰减后的物理基准伤害，不提前乘爆头，交给计算器算暴击）
	FLSDamageContext DamageContext;
	DamageContext.DamageCauser = GetOwner() ? GetOwner() : this;
	DamageContext.TargetActor = HitActor;
	DamageContext.BaseDamage = BaseDamage;
	DamageContext.FinalDamage = BaseDamage * Dropoff;
	DamageContext.ElementTag = ElementTag;
	DamageContext.DamageTypeTag = LSTags::TAG_Damage_Type_Bullet;
	DamageContext.bIsHeadshot = bIsHeadshot;
	DamageContext.HitResult = Hit;

	ULSElementComponent* TargetElementComp = HitActor->FindComponentByClass<ULSElementComponent>();
	FGameplayTag AuraTag = TargetElementComp ? TargetElementComp->GetPrimaryAuraTag() : FGameplayTag();

	// 3. 动态组装攻击者面板（读取全队驱动核心）
	FLSAttackerStats AttackerStats;
	AttackerStats.Attack = BaseDamage;
	AttackerStats.CritRate = 0.10f; // 狙击枪基础自带10%暴击
	AttackerStats.CritDamage = 0.50f;
	AttackerStats.ElementalMastery = 0.0f;
	AttackerStats.Level = 90;

	if (ALSCharacterBase* OwnerChar = Cast<ALSCharacterBase>(GetOwner()))
	{
		if (ALSPlayerController* PC = Cast<ALSPlayerController>(OwnerChar->GetController()))
		{
			if (ULSDriveCoreComponent* DriveCore = PC->GetDriveCoreComponent())
			{
				AttackerStats = DriveCore->BuildAttackerStats(OwnerChar);
			}
		}
	}

	// 狙击枪核心机制 1：蓄力倍率直接放大攻击力
	AttackerStats.Attack *= LastFiredChargeMultiplier;

	// 狙击枪核心机制 2：3.0x 爆头倍率由暴伤保障（保底 200% 暴伤，即 1 + 2.0 = 3.0 倍伤害）
	if (bIsHeadshot)
	{
		AttackerStats.CritDamage = FMath::Max(AttackerStats.CritDamage, HeadshotMultiplier - 1.0f);
	}

	FLSDefenderStats DefenderStats;
	DefenderStats.Level = 90;
	DefenderStats.Defense = 500.0f;
	DefenderStats.ElementalResistance = 0.1f;

	// 4. 交给伤害计算器统一部署全乘区
	FLSDamageResult DamageResult = ULSDamageCalculator::CalculateDamage(DamageContext, AttackerStats, DefenderStats, AuraTag);

	// 5. 元素附着与扣血广播
	if (TargetElementComp && ElementTag.IsValid())
	{
		TargetElementComp->ApplyElement(GetOwner() ? GetOwner() : this, ElementTag, ElementGauge);
	}

	HitActor->TakeDamage(DamageContext.FinalDamage, FDamageEvent(), (GetOwner() ? GetOwner()->GetInstigatorController() : nullptr), this);

	if (ULSEventBus* EventBus = ULSEventBus::Get(this))
	{
		EventBus->OnDamageDealt.Broadcast(DamageContext);
	}

	Client_HitConfirm(DamageContext.bIsHeadshot, DamageContext.FinalDamage);
}