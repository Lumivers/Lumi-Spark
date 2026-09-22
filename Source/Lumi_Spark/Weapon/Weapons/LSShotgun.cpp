#include "LSShotgun.h"
#include "Weapon/LSRecoilComponent.h"
#include "Character/LSCharacterBase.h"
#include "Character/LSCameraComponent.h"
#include "Core/LSTypes.h"
#include "Kismet/GameplayStatics.h"

ALSShotgun::ALSShotgun()
{
	WeaponDisplayName = FText::FromString(TEXT("破片重型霰弹枪"));
	WeaponTypeTag = LSTags::TAG_Weapon_Type_Shotgun;

	FireMode = ELSFireMode::SemiAuto;
	FireRate = 100.0f;                  // 100 RPM 泵动/半自动节拍
	BaseDamage = 16.0f;                 // 16 伤/弹丸 × 8 = 128 点基础总伤！
	HeadshotMultiplier = 1.5f;
	MagazineSize = 8;
	CurrentAmmo = 8;
	MaxReserveAmmo = 48;
	CurrentReserveAmmo = 48;
	ReloadTime = 2.4f;

	// 射程与极速衰减（超过 15 米伤害剧烈衰退）
	MaxRange = 2500.0f;
	DamageDropoffStart = 400.0f;
	DamageDropoffEnd = 1500.0f;
	MinDamageMultiplier = 0.2f;

	// 宽散布面
	BaseSpread = 4.2f;
	MaxSpread = 7.0f;
	SpreadIncreasePerShot = 1.0f;
	SpreadRecoveryRate = 4.0f;
	ADSSpreadMultiplier = 0.65f;
}

void ALSShotgun::FireOnce()
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

	// 1. 枪口起点与视口朝向
	const FVector MuzzleLocation = WeaponMesh->DoesSocketExist(MuzzleSocketName)
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

	bool bIsADS = false;
	if (ALSCharacterBase* LSChar = Cast<ALSCharacterBase>(OwnerChar))
	{
		if (ULSCameraComponent* Cam = LSChar->GetCameraComponent())
		{
			bIsADS = Cam->IsADS();
		}
	}

	const float SpreadAngle = bIsADS ? (CurrentSpread * ADSSpreadMultiplier) : CurrentSpread;
	const FVector AimDir = CameraRotation.Vector();

	// 2. 权威端扣除 1 发大口径弹药
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

	// 3. 循环发射 8 颗独立弹丸
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(OwnerChar);
	QueryParams.bTraceComplex = true;
	QueryParams.bReturnPhysicalMaterial = true;

	for (int32 i = 0; i < PelletCount; ++i)
	{
		// 独立锥形随机偏角
		const FVector PelletSpreadDir = FMath::VRandCone(AimDir, FMath::DegreesToRadians(SpreadAngle * 0.5f));
		const FVector CamTraceEnd = CameraLocation + PelletSpreadDir * MaxRange;

		FHitResult CamHit;
		FVector TargetPoint = CamTraceEnd;
		if (GetWorld()->LineTraceSingleByChannel(CamHit, CameraLocation, CamTraceEnd, ECC_Visibility, QueryParams))
		{
			TargetPoint = CamHit.ImpactPoint;
		}

		// 枪口连线校验
		const FVector MuzzleTraceEnd = MuzzleLocation + (TargetPoint - MuzzleLocation).GetSafeNormal() * MaxRange;
		FHitResult PelletHit;
		const bool bHit = GetWorld()->LineTraceSingleByChannel(PelletHit, MuzzleLocation, MuzzleTraceEnd, ECC_Visibility, QueryParams);

		if (bHit && HasAuthority())
		{
			ProcessHit(PelletHit);
			Multicast_ImpactEffects(PelletHit);
		}

		// 播放每颗弹丸的独立曳光烟道
		PlayTracerEffect(MuzzleLocation, bHit ? PelletHit.ImpactPoint : MuzzleTraceEnd);
	}

	// 4. 霰弹枪整枪后坐力与开火视听表现
	PlayLocalFireEffects(bIsADS);
}