#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Core/LSTypes.h"
#include "LSLauncherProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class UParticleSystem;
class USoundBase;

/**
 * 榴弹物理投射物实体 (ALSLauncherProjectile)
 * 物理弹道抛物线飞行、撞击敌人直接引爆或撞地后 2.0s 延时引信、范围 AoE 伤害与 2U 强元素附着
 */
UCLASS()
class LUMI_SPARK_API ALSLauncherProjectile : public AActor
{
	GENERATED_BODY()

public:
	ALSLauncherProjectile();

	virtual void BeginPlay() override;

	// 初始化弹头属性（由发射器在 Spawn 时注入）
	void InitializeProjectile(float InDamage, FGameplayTag InElement, ELSElementGauge InGauge, AActor* InShooter);

	UFUNCTION()
	void OnProjectileHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	// 引爆
	UFUNCTION(BlueprintCallable, Category = "Launcher|Combat")
	void Explode();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ProjectileMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	// 爆炸半斤
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float ExplosionRadius = 450.0f;

	// 基础爆炸伤害
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float ExplosionDamage = 200.0f;

	// 撞击硬表面后的自爆延时
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float FuseDelay = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	FGameplayTag ElementTag;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	ELSElementGauge ElementGauge = ELSElementGauge::Heavy;

	// 视听表现
	UPROPERTY(EditDefaultsOnly, Category = "FX")
	TObjectPtr<UParticleSystem> ExplosionFX;

	UPROPERTY(EditDefaultsOnly, Category = "FX")
	TObjectPtr<USoundBase> ExplosionSound;

private:
	UPROPERTY()
	TObjectPtr<AActor> ShooterActor = nullptr;

	FTimerHandle FuseTimerHandle;
	bool bHasExploded = false;
};