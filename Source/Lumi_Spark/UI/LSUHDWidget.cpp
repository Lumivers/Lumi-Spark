#include "LSUHDWidget.h"
#include "Weapon/LSWeaponBase.h"
#include "Weapon/LSWeaponComponent.h"
#include "Core/LSEventBus.h"
#include "Kismet/GameplayStatics.h"

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
