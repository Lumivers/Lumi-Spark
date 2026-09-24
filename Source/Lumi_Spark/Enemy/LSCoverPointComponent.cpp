#include "LSCoverPointComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

ULSCoverPointComponent::ULSCoverPointComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool ULSCoverPointComponent::IsValidAgainst(const FVector& ThreatLocation) const
{
	const FVector CoverStandPoint = GetComponentLocation() + FVector(0.0f, 0.0f, CoverHeight * 0.5f);
	
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());
	if (OccupyingActor.IsValid())
	{
		QueryParams.AddIgnoredActor(OccupyingActor.Get());
	}
	
	//从威胁点向掩体站位点发射视线检测射线
	const bool bBlocked = GetWorld()->LineTraceSingleByChannel(HitResult, ThreatLocation, CoverStandPoint, ECC_Visibility, QueryParams);
	
	//仅在调试构建划线：被阻挡为绿色，未被阻挡为红色
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	DrawDebugLine(GetWorld(), ThreatLocation, bBlocked ? HitResult.Location : CoverStandPoint, bBlocked ? FColor::Emerald : FColor::Red, false, 1.5f, 0, 1.5f);
#endif

	return bBlocked;
}

void ULSCoverPointComponent::SetOccupied(bool bInOccupied, AActor* InActor)
{
	bIsOccupied = bInOccupied;
	OccupyingActor = bInOccupied ? InActor : nullptr;
}
