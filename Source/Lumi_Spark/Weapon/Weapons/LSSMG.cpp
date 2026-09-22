#include "LSSMG.h"
#include "Core/LSTypes.h"

ALSSMG::ALSSMG()
{
	WeaponDisplayName = FText::FromString(TEXT("疾风微型冲锋枪"));
	WeaponTypeTag = LSTags::TAG_Weapon_Type_SMG;

	FireMode = ELSFireMode::FullAuto;
	FireRate = 900.0f;                  // 900 RPM 高射速泼水
	BaseDamage = 22.0f;                 // 单发基础伤较低
	HeadshotMultiplier = 1.5f;
	MagazineSize = 40;
	CurrentAmmo = 40;
	MaxReserveAmmo = 240;
	CurrentReserveAmmo = 240;
	ReloadTime = 1.4f;                  // 极速换弹

	// 近距离衰减区间（10米开始衰减，25米衰减到底）
	MaxRange = 4000.0f;
	DamageDropoffStart = 1000.0f;
	DamageDropoffEnd = 2500.0f;
	MinDamageMultiplier = 0.35f;

	// 散布特征：腰射散布略大，但单发扩散小，回正极快
	BaseSpread = 1.5f;
	MaxSpread = 5.0f;
	SpreadIncreasePerShot = 0.12f;
	SpreadRecoveryRate = 6.0f;
	ADSSpreadMultiplier = 0.5f;
}

float ALSSMG::CalculateDamageDropoff(float Distance) const
{
	if (Distance <= DamageDropoffStart) return 1.0f;
	if (Distance >= DamageDropoffEnd)   return MinDamageMultiplier;

	// 平滑的二次方快速衰减曲线，突出近战优势
	const float Alpha = (Distance - DamageDropoffStart) / (DamageDropoffEnd - DamageDropoffStart);
	return FMath::Lerp(1.0f, MinDamageMultiplier, Alpha * Alpha);
}