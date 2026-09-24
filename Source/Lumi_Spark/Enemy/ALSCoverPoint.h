#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/LSTypes.h"
#include "ALSCoverPoint.generated.h"

class ULSCoverPointComponent;

/**
 * 关卡掩体点实体 (AALSCoverPoint)
 */
UCLASS()
class LUMI_SPARK_API AALSCoverPoint : public AActor
{
	GENERATED_BODY()

public:
	AALSCoverPoint();

	FORCEINLINE ULSCoverPointComponent* GetCoverComponent() const { return CoverComponent; }

	bool IsValidAgainst(const FVector& ThreatLocation) const;
	bool IsOccupied() const;
	void SetOccupied(bool bInOccupied, AActor* InActor = nullptr);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cover")
	TObjectPtr<ULSCoverPointComponent> CoverComponent;
};