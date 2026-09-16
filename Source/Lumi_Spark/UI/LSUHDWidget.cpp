#include "LSUHDWidget.h"
#include "Weapon/LSWeaponBase.h"
#include "Weapon/LSWeaponComponent.h"
#include "Core/LSEventBus.h"
#include "Kismet/GameplayStatics.h"
#include "Character/LSCharacterBase.h"
#include "Character/LSSkillComponent.h"

void ULSHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	APawn* OwningPawn = GetOwningPlayerPawn();
	if (!OwningPawn) return;
	
	//1，获取武器组件并绑定事件
	CachedWeaponComp = OwningPawn->FindComponentByClass<ULSWeaponComponent>();
	if (CachedWeaponComp)
	{
		CachedWeaponComp->OnWeaponChanged.AddDynamic(this, &ULSHUDWidget::HandleWeaponChanged);
		BindToWeapon(CachedWeaponComp->GetCurrentWeapon());
	}
	
	//2,监听全局事件总线的命中伤害广播
	if (ULSEventBus* EventBus = ULSEventBus::Get(this))
	{
		EventBus->OnDamageDealt.AddDynamic(this, &ULSHUDWidget::HandleDamageDealt);
	}
}

void ULSHUDWidget::NativeDestruct()
{
	if (CachedWeaponComp)
	{
		CachedWeaponComp->OnWeaponChanged.RemoveDynamic(this, &ULSHUDWidget::HandleWeaponChanged);
	}
	
	if (CurrentBoundWeapon)
	{
		CurrentBoundWeapon->OnAmmoChanged.RemoveDynamic(this, &ULSHUDWidget::HandleAmmoChanged);
	}
	
	if (ULSEventBus* EventBus = ULSEventBus::Get(this))
	{
		EventBus->OnDamageDealt.RemoveDynamic(this, &ULSHUDWidget::HandleDamageDealt);
	}
	
	Super::NativeDestruct();
}

void ULSHUDWidget::BindToWeapon(ALSWeaponBase* Weapon)
{
	//解绑旧武器
	if (CurrentBoundWeapon)
	{
		CurrentBoundWeapon->OnAmmoChanged.RemoveDynamic(this, &ULSHUDWidget::HandleAmmoChanged);
	}
	
	CurrentBoundWeapon = Weapon;
	
	//绑定新武器
	if (CurrentBoundWeapon)
	{
		CurrentBoundWeapon->OnAmmoChanged.AddDynamic(this, &ULSHUDWidget::HandleAmmoChanged);
		//初始刷新一次弹药UI
		OnAmmoUpdated(CurrentBoundWeapon->GetCurrentAmmo(), CurrentBoundWeapon->GetMagazineSize(), CurrentBoundWeapon->GetCurrentReserveAmmo());
	}
	
	OnWeaponSwitched(CurrentBoundWeapon);
}

void ULSHUDWidget::HandleWeaponChanged(ALSWeaponBase* NewWeapon)
{
	BindToWeapon(NewWeapon);
}

void ULSHUDWidget::HandleAmmoChanged(int32 CurrentAmmo, int32 MagazineSize, int32 ReserveAmmo)
{
	OnAmmoUpdated(CurrentAmmo, MagazineSize, ReserveAmmo);
}

void ULSHUDWidget::HandleDamageDealt(const FLSDamageContext& DamageContext)
{
	//只对本地玩家的伤害显示反馈
	if (DamageContext.DamageCauser != GetOwningPlayerPawn()) return;
	
	//播放命中音效
	USoundBase* SoundToPlay = DamageContext.bIsHeadshot ? HitHeadshotSound : HitNormalSound;
	if (SoundToPlay)
	{
		UGameplayStatics::PlaySound2D(this, SoundToPlay);
	}
	
	//触发蓝图动画
	OnHitMarkerTriggered(DamageContext.bIsHeadshot);
}

float ULSHUDWidget::GetCurrentSpreadRatio() const
{
	return CurrentBoundWeapon ? CurrentBoundWeapon->GetSpreadRatio() : 0.0f;
}

void ULSHUDWidget::BindToCharacter(ALSCharacterBase* NewCharacter)
{
    if (NewCharacter == BoundCharacter && BoundCharacter != nullptr) return;

    // 1. 解绑旧角色的生命与技能委托（防止切人后产生悬挂委托或重复回调）
    if (BoundCharacter)
    {
        BoundCharacter->OnHealthChanged.RemoveDynamic(this, &ULSHUDWidget::HandleHealthChanged);
    }
    if (BoundSkillComp)
    {
        BoundSkillComp->OnSkillCooldownChanged.RemoveDynamic(this, &ULSHUDWidget::HandleSkillCooldownChanged);
        BoundSkillComp->OnEnergyChanged.RemoveDynamic(this, &ULSHUDWidget::HandleEnergyChanged);
    }

    // 2. 绑定新角色
    BoundCharacter = NewCharacter;
    if (!BoundCharacter) return;

    BoundCharacter->OnHealthChanged.AddDynamic(this, &ULSHUDWidget::HandleHealthChanged);

    // 重新绑定新角色的武器组件
    if (ULSWeaponComponent* NewWeaponComp = BoundCharacter->GetWeaponComponent())
    {
        // 绑定武器切枪与弹药委托...
        BindToWeapon(NewWeaponComp->GetCurrentWeapon());
    }

    // 重新绑定新角色的技能组件
    BoundSkillComp = BoundCharacter->GetSkillComponent();
    if (BoundSkillComp)
    {
        BoundSkillComp->OnSkillCooldownChanged.AddDynamic(this, &ULSHUDWidget::HandleSkillCooldownChanged);
        BoundSkillComp->OnEnergyChanged.AddDynamic(this, &ULSHUDWidget::HandleEnergyChanged);

        // 3. 关键：绑定成功瞬间，主动向蓝图派发一次当前全量初始数据，消除 UI 滞后！
        OnSkillCooldownUpdated(BoundSkillComp->GetSkillCooldownRemaining(), 
                               BoundSkillComp->GetSkillCooldownRemaining(), 
                               BoundSkillComp->GetSkillCooldownRatio());

        OnBurstEnergyUpdated(BoundSkillComp->GetCurrentEnergy(), 
                             BoundSkillComp->GetMaxEnergy(), 
                             BoundSkillComp->GetEnergyRatio());
    }
}

void ULSHUDWidget::HandleHealthChanged(float CurrentHealth, float MaxHealth)
{
    OnHealthUpdated(CurrentHealth, MaxHealth);
}

void ULSHUDWidget::HandleSkillCooldownChanged(float CurrentCooldown, float MaxCooldown)
{
    const float Ratio = (MaxCooldown > 0.0f) ? (CurrentCooldown / MaxCooldown) : 0.0f;
    OnSkillCooldownUpdated(CurrentCooldown, MaxCooldown, Ratio);
}

void ULSHUDWidget::HandleEnergyChanged(float CurrentEnergy, float MaxEnergy)
{
    const float Ratio = (MaxEnergy > 0.0f) ? (CurrentEnergy / MaxEnergy) : 0.0f;
    OnBurstEnergyUpdated(CurrentEnergy, MaxEnergy, Ratio);
}