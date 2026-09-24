#include "LSEnemy_Sniper.h"
#include "Engine/DamageEvents.h"
#include "DrawDebugHelpers.h"

ALSEnemy_Sniper::ALSEnemy_Sniper()
{
	PrimaryActorTick.bCanEverTick = true;
	ActiveGameplayTags.AddTag(LSTags::TAG_Enemy_Type_Sniper);
}

void ALSEnemy_Sniper::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 瞄准预警期间，每帧绘制红色瞄准激光
	if (bIsAiming && AimTarget.IsValid())
	{
		const FVector MuzzleLoc = GetActorLocation() + FVector(0, 0, 70.0f);
		DrawDebugLine(GetWorld(), MuzzleLoc, AimTarget->GetActorLocation(), FColor::Red, false, -1.0f, 0, 2.0f);
	}
}

void ALSEnemy_Sniper::StartAimingAt(AActor* Target)
{
	if (bIsDead || bIsAiming || !Target) return;

	bIsAiming = true;
	AimTarget = Target;

	// 1.5 秒后击发
	GetWorldTimerManager().SetTimer(AimTimerHandle, this, &ALSEnemy_Sniper::FireSniperShot, AimDuration, false);
}

void ALSEnemy_Sniper::CancelAiming()
{
	bIsAiming = false;
	GetWorldTimerManager().ClearTimer(AimTimerHandle);
}

void ALSEnemy_Sniper::FireSniperShot()
{
	bIsAiming = false;
	if (bIsDead || !AimTarget.IsValid()) return;

	const FVector MuzzleLoc = GetActorLocation() + FVector(0, 0, 70.0f);
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(HitResult, MuzzleLoc, AimTarget->GetActorLocation(), ECC_Visibility, QueryParams))
	{
		if (HitResult.GetActor() == AimTarget.Get())
		{
			AimTarget->TakeDamage(SniperDamage, FDamageEvent(), GetController(), this);
		}
	}
}