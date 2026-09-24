#include "LSEnemy_Ranged.h"
#include "Engine/DamageEvents.h"
#include "DrawDebugHelpers.h"

ALSEnemy_Ranged::ALSEnemy_Ranged()
{
	ActiveGameplayTags.AddTag(LSTags::TAG_Enemy_Type_Ranged);
}

bool ALSEnemy_Ranged::ShouldRetreatFrom(const AActor* Target) const
{
	if (!Target) return false;
	return FVector::Dist(GetActorLocation(), Target->GetActorLocation()) < MinKeepDistance;
}

void ALSEnemy_Ranged::FireBurst(AActor* Target)
{
	if (bIsDead || !Target || RemainingBurstShots > 0) return;

	CurrentTarget = Target;
	RemainingBurstShots = 3;

	// 三连发点射定时器（0.12s 间隔）
	GetWorldTimerManager().SetTimer(BurstTimerHandle, this, &ALSEnemy_Ranged::ExecuteSingleShot, 0.12f, true, 0.0f);
}

void ALSEnemy_Ranged::ExecuteSingleShot()
{
	if (bIsDead || RemainingBurstShots <= 0 || !CurrentTarget.IsValid())
	{
		GetWorldTimerManager().ClearTimer(BurstTimerHandle);
		RemainingBurstShots = 0;
		return;
	}

	RemainingBurstShots--;

	const FVector MuzzleLoc = GetActorLocation() + GetActorForwardVector() * 50.0f + FVector(0, 0, 50.0f);
	const FVector TargetLoc = CurrentTarget->GetActorLocation();

	// 射线检测射击
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(HitResult, MuzzleLoc, TargetLoc, ECC_Visibility, QueryParams))
	{
		if (HitResult.GetActor() == CurrentTarget.Get())
		{
			CurrentTarget->TakeDamage(BulletDamage, FDamageEvent(), GetController(), this);
		}
	}

	DrawDebugLine(GetWorld(), MuzzleLoc, HitResult.bBlockingHit ? HitResult.ImpactPoint : TargetLoc, FColor::Yellow, false, 0.2f, 0, 1.2f);
}