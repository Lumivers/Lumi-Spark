#include "LSSniperRifle.h"
#include "Character/LSCharacterBase.h"
#include "Character/LSCameraComponent.h"
#include "Core/LSTypes.h"
#include "Combat/LSDamageCalculator.h"
#include "Element/LSElementComponent.h"
#include "Core/LSEventBus.h"
#include "Engine/DamageEvents.h"

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

	// 1. 结合蓄力增伤倍率计算
	const float Distance = (Hit.ImpactPoint - GetActorLocation()).Size();
	const float Dropoff = CalculateDamageDropoff(Distance);
	const float ChargedBaseDamage = BaseDamage * LastFiredChargeMultiplier;
	float FinalDamage = ChargedBaseDamage * Dropoff;

	// 2. 3.0x 爆头判定
	bool bIsHeadshot = false;
	if (Hit.BoneName.ToString().Contains(TEXT("head"), ESearchCase::IgnoreCase) || 
		(Hit.Component.IsValid() && Hit.Component->ComponentHasTag(TEXT("head"))))
	{
		FinalDamage *= HeadshotMultiplier;
		bIsHeadshot = true;
	}

	// 3. 组装全局伤害上下文
	FLSDamageContext DamageContext;
	DamageContext.DamageCauser = GetOwner() ? GetOwner() : this;
	DamageContext.TargetActor = HitActor;
	DamageContext.BaseDamage = ChargedBaseDamage;
	DamageContext.FinalDamage = FinalDamage;
	DamageContext.ElementTag = ElementTag;
	DamageContext.DamageTypeTag = LSTags::TAG_Damage_Type_Bullet;
	DamageContext.bIsHeadshot = bIsHeadshot;
	DamageContext.HitResult = Hit;

	ULSElementComponent* TargetElementComp = HitActor->FindComponentByClass<ULSElementComponent>();
	FGameplayTag AuraTag = TargetElementComp ? TargetElementComp->GetPrimaryAuraTag() : FGameplayTag();

	FLSAttackerStats AttackerStats;
	AttackerStats.Attack = ChargedBaseDamage;
	AttackerStats.CritRate = 0.1f;
	AttackerStats.CritDamage = 0.5f;
	AttackerStats.ElementalMastery = 120.0f;
	AttackerStats.Level = 90;

	FLSDefenderStats DefenderStats;
	DefenderStats.Level = 90;
	DefenderStats.Defense = 500.0f;
	DefenderStats.ElementalResistance = 0.1f;

	FLSDamageResult DamageResult = ULSDamageCalculator::CalculateDamage(DamageContext, AttackerStats, DefenderStats, AuraTag);

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