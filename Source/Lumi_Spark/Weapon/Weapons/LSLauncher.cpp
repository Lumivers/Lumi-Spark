#include "LSLauncher.h"
#include "LSLauncherProjectile.h"
#include "Character/LSCharacterBase.h"
#include "Character/LSCameraComponent.h"
#include "Core/LSTypes.h"
#include "Engine/World.h"

ALSLauncher::ALSLauncher()
{
	WeaponDisplayName = FText::FromString(TEXT("地脉榴弹发射器"));
	WeaponTypeTag = LSTags::TAG_Weapon_Type_Launcher;

	FireMode = ELSFireMode::SemiAuto;
	FireRate = 75.0f;                   // 75 RPM 重型榴弹
	BaseDamage = 200.0f;                // 200 点范围高爆伤害
	ElementGauge = ELSElementGauge::Heavy; // 标配 2U 强元素！
	MagazineSize = 4;
	CurrentAmmo = 4;
	MaxReserveAmmo = 20;
	CurrentReserveAmmo = 20;
	ReloadTime = 2.8f;

	BaseSpread = 0.5f;
	MaxSpread = 2.0f;

	// 默认指定为我们自带的榴弹实体类
	ProjectileClass = ALSLauncherProjectile::StaticClass();
}

void ALSLauncher::FireOnce()
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

	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!OwnerChar) return;

	// 1. 枪口生成点与视口朝向
	const FVector MuzzleLoc = WeaponMesh->DoesSocketExist(MuzzleSocketName)
		? WeaponMesh->GetSocketLocation(MuzzleSocketName)
		: GetActorLocation();

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

	// 2. 扣弹药
	if (HasAuthority())
	{
		CurrentAmmo--;
		OnAmmoChanged.Broadcast(CurrentAmmo, MagazineSize, CurrentReserveAmmo);
	}
	else
	{
		CurrentAmmo = FMath::Max(0, CurrentAmmo - 1);
		OnAmmoChanged.Broadcast(CurrentAmmo, MagazineSize, CurrentReserveAmmo);
	}

	// 3. 服务端权威生成物理榴弹实体
	if (HasAuthority() && ProjectileClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = OwnerChar;
		SpawnParams.Instigator = OwnerChar;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		// 视口朝向前向发射
		if (ALSLauncherProjectile* Proj = GetWorld()->SpawnActor<ALSLauncherProjectile>(ProjectileClass, MuzzleLoc, CameraRotation, SpawnParams))
		{
			Proj->InitializeProjectile(BaseDamage, ElementTag, ElementGauge, OwnerChar);
		}
	}

	// 4. 本地开火视听表现
	PlayLocalFireEffects(false);
}