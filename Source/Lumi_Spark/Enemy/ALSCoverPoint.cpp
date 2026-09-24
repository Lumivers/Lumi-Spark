#include "ALSCoverPoint.h"
#include "LSCoverPointComponent.h"

AALSCoverPoint::AALSCoverPoint()
{
	PrimaryActorTick.bCanEverTick = false;
	
	CoverComponent = CreateDefaultSubobject<ULSCoverPointComponent>(TEXT("CoverComp"));
	RootComponent = CoverComponent;
}

bool AALSCoverPoint::IsValidAgainst(const FVector& ThreatLocation) const
{
	if (CoverComponent)
	{
		return CoverComponent ? CoverComponent->IsValidAgainst(ThreatLocation) : false;
	}
	return false;
}

bool AALSCoverPoint::IsOccupied() const
{
	return CoverComponent ? CoverComponent->IsOccupied() : false;
}

void AALSCoverPoint::SetOccupied(bool bInOccupied, AActor* InActor)
{
	if (CoverComponent)
	{
		CoverComponent->SetOccupied(bInOccupied, InActor);
	}
}