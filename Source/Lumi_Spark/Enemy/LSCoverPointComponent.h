#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Core/LSTypes.h"
#include "LSCoverPointComponent.generated.h"

/**
 * 掩体感知与空间遮挡判定组件 (ULSCoverPointComponent)
 */
UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class LUMI_SPARK_API ULSCoverPointComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	ULSCoverPointComponent();

	// 核心遮挡算法：自威胁坐标发射检测射线，验证从威胁方向望来是否被掩体几何体阻隔
	UFUNCTION(BlueprintCallable, Category = "AI|Cover")
	bool IsValidAgainst(const FVector& ThreatLocation) const;

	UFUNCTION(BlueprintCallable, Category = "AI|Cover")
	void SetOccupied(bool bInOccupied, AActor* InActor = nullptr);

	UFUNCTION(BlueprintPure, Category = "AI|Cover")
	bool IsOccupied() const { return bIsOccupied; }

	UFUNCTION(BlueprintPure, Category = "AI|Cover")
	ELSCoverType GetCoverType() const { return CoverType; }

	UFUNCTION(BlueprintPure, Category = "AI|Cover")
	float GetCoverHeight() const { return CoverHeight; }

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cover")
	ELSCoverType CoverType = ELSCoverType::Full;

	// 掩体防护高度（全高 180cm，半高 90cm）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cover")
	float CoverHeight = 180.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Cover")
	bool bIsOccupied = false;

	UPROPERTY()
	TWeakObjectPtr<AActor> OccupyingActor = nullptr;
};