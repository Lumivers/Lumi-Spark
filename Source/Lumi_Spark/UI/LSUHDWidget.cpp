#include "LSUHDWidget.h"
#include "Weapon/LSWeaponBase.h"
#include "Weapon/LSWeaponComponent.h"
#include "Core/LSEventBus.h"
#include "Kismet/GameplayStatics.h"
#include "Character/LSCharacterBase.h"
#include "Character/LSHealthComponent.h"
#include "Character/LSStaminaComponent.h"
#include "Character/LSEnergyComponent.h"
#include "Character/LSSkillComponent.h"

void ULSHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	APawn* OwningPawn = GetOwningPlayerPawn();
	if (!OwningPawn) return;
	
	CachedWeaponComp = OwningPawn->FindComponentByClass<ULSWeaponComponent>();
	if (CachedWeaponComp)
	{
		CachedWeaponComp->OnWeaponChanged.AddDynamic(this, &ULSHUDWidget::HandleWeaponChanged);
		BindToWeapon(CachedWeaponComp->GetCurrentWeapon());
	}
	
	if (ULSEventBus* EventBus = ULSEventBus::Get(this))
	{
		EventBus->OnDamageDealt.AddDynamic(this, &ULSHUDWidget::HandleDamageDealt);
		EventBus->OnRaidWiped.AddDynamic(this, &ULSHUDWidget::HandleGlobalRaidWiped);
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
		EventBus->OnRaidWiped.RemoveDynamic(this, &ULSHUDWidget::HandleGlobalRaidWiped);
	}
	
	Super::NativeDestruct();
}

void ULSHUDWidget::BindToWeapon(ALSWeaponBase* Weapon)
{
	if (CurrentBoundWeapon)
	{
		CurrentBoundWeapon->OnAmmoChanged.RemoveDynamic(this, &ULSHUDWidget::HandleAmmoChanged);
	}
	
	CurrentBoundWeapon = Weapon;
	
	if (CurrentBoundWeapon)
	{
		CurrentBoundWeapon->OnAmmoChanged.AddDynamic(this, &ULSHUDWidget::HandleAmmoChanged);
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
	if (DamageContext.DamageCauser != GetOwningPlayerPawn()) return;
	
	USoundBase* SoundToPlay = DamageContext.bIsHeadshot ? HitHeadshotSound : HitNormalSound;
	if (SoundToPlay)
	{
		UGameplayStatics::PlaySound2D(this, SoundToPlay);
	}
	
	OnHitMarkerTriggered(DamageContext.bIsHeadshot);
}

float ULSHUDWidget::GetCurrentSpreadRatio() const
{
	return CurrentBoundWeapon ? CurrentBoundWeapon->GetSpreadRatio() : 0.0f;
}

void ULSHUDWidget::BindToCharacter(ALSCharacterBase* NewCharacter)
{
    if (NewCharacter == BoundCharacter && BoundCharacter != nullptr) return;

    // 1. 直连解绑旧组件的委托
    if (BoundHealthComp)
    {
        BoundHealthComp->OnHealthChanged.RemoveDynamic(this, &ULSHUDWidget::HandleHealthChanged);
        BoundHealthComp->OnLowHealth.RemoveDynamic(this, &ULSHUDWidget::HandleLowHealth);
    }
    if (BoundStaminaComp)
    {
        BoundStaminaComp->OnStaminaChanged.RemoveDynamic(this, &ULSHUDWidget::HandleStaminaChanged);
    }
    if (BoundEnergyComp)
    {
        BoundEnergyComp->OnEnergyChanged.RemoveDynamic(this, &ULSHUDWidget::HandleEnergyChanged);
    }
    if (BoundSkillComp)
    {
        BoundSkillComp->OnSkillCooldownChanged.RemoveDynamic(this, &ULSHUDWidget::HandleSkillCooldownChanged);
    }

    BoundCharacter = NewCharacter;
    if (!BoundCharacter) return;

    // 重新绑定武器
    if (ULSWeaponComponent* NewWeaponComp = BoundCharacter->GetWeaponComponent())
    {
        BindToWeapon(NewWeaponComp->GetCurrentWeapon());
    }

    // 2. 直连绑定新角色的资源组件，并立即主动推流初始值！
    BoundHealthComp = BoundCharacter->GetHealthComponent();
    if (BoundHealthComp)
    {
        BoundHealthComp->OnHealthChanged.AddDynamic(this, &ULSHUDWidget::HandleHealthChanged);
        BoundHealthComp->OnLowHealth.AddDynamic(this, &ULSHUDWidget::HandleLowHealth);

        OnHealthUpdated(BoundHealthComp->GetCurrentHealth(), BoundHealthComp->GetMaxHealth());
        OnLowHealthWarning(BoundHealthComp->GetHealthPercent() <= 0.2f);
    }

    BoundStaminaComp = BoundCharacter->GetStaminaComponent();
    if (BoundStaminaComp)
    {
        BoundStaminaComp->OnStaminaChanged.AddDynamic(this, &ULSHUDWidget::HandleStaminaChanged);
        OnStaminaUpdated(BoundStaminaComp->GetCurrentStamina(), BoundStaminaComp->GetMaxStamina(), BoundStaminaComp->GetStaminaRatio());
    }

    BoundEnergyComp = BoundCharacter->GetEnergyComponent();
    if (BoundEnergyComp)
    {
        BoundEnergyComp->OnEnergyChanged.AddDynamic(this, &ULSHUDWidget::HandleEnergyChanged);
        OnBurstEnergyUpdated(BoundEnergyComp->GetCurrentEnergy(), BoundEnergyComp->GetMaxEnergy(), BoundEnergyComp->GetEnergyRatio());
    }

    BoundSkillComp = BoundCharacter->GetSkillComponent();
    if (BoundSkillComp)
    {
        BoundSkillComp->OnSkillCooldownChanged.AddDynamic(this, &ULSHUDWidget::HandleSkillCooldownChanged);
        OnSkillCooldownUpdated(BoundSkillComp->GetSkillCooldownRemaining(), 
                               BoundSkillComp->GetSkillCooldownRemaining(), 
                               BoundSkillComp->GetSkillCooldownRatio());
    }
}

void ULSHUDWidget::HandleHealthChanged(float CurrentHealth, float MaxHealth, bool bIsDamage)
{
    OnHealthUpdated(CurrentHealth, MaxHealth);
    if (MaxHealth > 0.0f && (CurrentHealth / MaxHealth) > 0.2f)
    {
        OnLowHealthWarning(false);
    }
}

void ULSHUDWidget::HandleLowHealth(float CurrentHealth, float MaxHealth)
{
    OnLowHealthWarning(true);
}

void ULSHUDWidget::HandleStaminaChanged(float CurrentStamina, float MaxStamina)
{
    const float Ratio = (MaxStamina > 0.0f) ? (CurrentStamina / MaxStamina) : 0.0f;
    OnStaminaUpdated(CurrentStamina, MaxStamina, Ratio);
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

void ULSHUDWidget::HandleGlobalRaidWiped()
{
    OnRaidWipedTriggered();
}