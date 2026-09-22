#include "LSRifle.h"
#include "Core/LSTypes.h"

ALSRifle::ALSRifle()
{
	WeaponDisplayName = FText::FromString(TEXT("脉冲突击步枪"));
	WeaponTypeTag = LSTags::TAG_Weapon_Type_Rifle;

	// 步枪基准战斗参数
	FireMode = ELSFireMode::FullAuto;
	FireRate = 600.0f;                  // 600 RPM
	BaseDamage = 35.0f;                 // 单发 35 伤
	HeadshotMultiplier = 2.0f;         // 2.0x 爆头
	MagazineSize = 30;
	CurrentAmmo = 30;
	MaxReserveAmmo = 180;
	CurrentReserveAmmo = 180;
	ReloadTime = 1.8f;

	// 射程与衰减
	MaxRange = 8000.0f;
	DamageDropoffStart = 2000.0f;       // 20 米开始轻微衰减
	DamageDropoffEnd = 5000.0f;         // 50 米衰减到底
	MinDamageMultiplier = 0.55f;

	// 散布控制
	BaseSpread = 0.8f;
	MaxSpread = 3.5f;
	SpreadIncreasePerShot = 0.2f;
	SpreadRecoveryRate = 5.0f;
	ADSSpreadMultiplier = 0.35f;
}

void ALSRifle::ToggleFireMode()
{
	if (bIsFiring)
	{
		StopFire();
	}

	FireMode = (FireMode == ELSFireMode::FullAuto) ? ELSFireMode::SemiAuto : ELSFireMode::FullAuto;
	OnFireModeSwitched.Broadcast(FireMode);
}

void ALSRifle::StartFire()
{
	if (FireMode == ELSFireMode::SemiAuto)
	{
		// 半自动模式：点一次打一发，不启动循环定时器
		if (!CanFire()) return;
		FireOnce();
	}
	else
	{
		// 全自动模式：走父类定时器连射逻辑
		Super::StartFire();
	}
}