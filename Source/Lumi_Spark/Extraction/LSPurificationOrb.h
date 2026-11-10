#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LSPurificationOrb.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class URotatingMovementComponent;
class USoundBase;
class UParticleSystem;

/**
 * 地脉净化晶体/光球实体 (ALSPurificationOrb)
 * 击碎地脉结晶或击杀特定变异体掉落的战术解压道具
 * 玩家触碰后立即扣减地脉侵蚀度并提供正向心流反馈
 */
UCLASS()
class LUMI_SPARK_API ALSPurificationOrb : public AActor
{
	GENERATED_BODY()

public:
	ALSPurificationOrb();

	/** 拾取回退侵蚀度计量（默认 20 点/20%） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Purification|Stats", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float CleanseAmount = 20.0f;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> OrbMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<URotatingMovementComponent> RotatingMovement;

	// 拾取音效与粒子
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Purification|Effects")
	TObjectPtr<USoundBase> PickupSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Purification|Effects")
	TObjectPtr<UParticleSystem> PickupEmitter;

	UFUNCTION()
	void HandleSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	bool bIsCollected = false;
};
