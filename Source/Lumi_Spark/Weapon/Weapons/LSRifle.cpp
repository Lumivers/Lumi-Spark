#include "LSRifle.h"
#include "Core/LSTypes.h"
#include "Weapon/LSRecoilComponent.h"

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

	// 散布控制：前 1~2 发速点/短点极高精准度，持续扫射散布急剧放大以鼓励点射
	BaseSpread = 0.25f;
	MaxSpread = 6.8f;
	SpreadIncreasePerShot = 0.60f;      // 约 11 发无脑扫射打满扩散惩罚
	SpreadRecoveryRate = 8.5f;          // 停火速点间隙迅速回正
	ADSSpreadMultiplier = 0.30f;

	// 步枪专用镜头后坐力：前两发温和无横甩适合速点，连射后段加重上抬与横向摆动；开启停火平滑回弹
	if (RecoilComponent)
	{
		const TArray<FVector2D> RiflePattern = {
			FVector2D(0.00f,  0.38f),
			FVector2D(0.02f,  0.46f),
			FVector2D(-0.08f, 0.56f),
			FVector2D(0.14f,  0.64f),
			FVector2D(-0.24f, 0.66f),
			FVector2D(0.32f,  0.62f),
			FVector2D(-0.40f, 0.54f),
			FVector2D(0.46f,  0.46f)
		};
		RecoilComponent->ConfigureRecoilDefaults(RiflePattern, 1.0f, 9.0f, 0.72f, 10.5f, true, 0.12f);
	}

	// 步枪战术飞行弹道参数（800 m/s 高初速，2.0 米尾迹，开启飞行受击时钟同步）
	BulletTracerSpeed = 800.0f;
	TracerRibbonLength = 2.0f;
	bUseDirectTraceEndTiming = true;
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