#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "LSMovementComponent.generated.h"

UENUM(BlueprintType)
enum class ELSMovementState : uint8
{
	Normal UMETA(DisplayName = "Normal Walk"),
	Sprinting UMETA(DisplayName = "Sprinting"),
	Aiming UMETA(DisplayName = "Aiming(ADS)"),
	Dashing UMETA(DisplayName = "Dashing"),
	Sliding UMETA(DisplayName = "Sliding"),
};

//闪避/受击无敌委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInvincibilityChanged, bool, bIsInvincible);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LUMI_SPARK_API ULSMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	ULSMovementComponent();
	
	//移动速率设置
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Speed")
	float WalkSpeed = 600.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Speed")
	float SprintSpeed = 950.f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement|Speed")
	float ADSSpeed = 300.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Speed")
	float SlideSpeed = 1100.f;
	
	//闪避参数（DASH）
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Dash")
	float DashImpulse = 2200.f; //闪避冲量
	
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Dash")
	float DashCooldown = 0.5f; //闪避冷却时间
	
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Dash")
	float DashIFrameDuration = 0.2f; //闪避无敌时间
	
	//滑铲参数（SLIDE）
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Slide")
	float SlideDuration = 0.7f; //滑铲持续时间
	
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Slide")
	float SlideFriction = 0.3f; //滑铲摩擦力
	
	//无敌状态变更事件
	UPROPERTY(BlueprintAssignable, Category = "Movement|Events")
	FOnInvincibilityChanged OnInvincibilityChanged;
	
	//状态控制接口
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void StartSprint();
	
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void StopSprint();
	
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void SetAiming(bool bNewAiming);
	
	UFUNCTION(BlueprintCallable, Category = "Movement")
	bool TryDash();
	
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void StartSlide();
	
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void StopSlide();
	
	//状态查询
	UFUNCTION(BlueprintCallable, Category = "Movement")
	FORCEINLINE bool IsSprinting() const { return bWantsToSprint; }
	
	UFUNCTION(BlueprintCallable, Category = "Movement")
	FORCEINLINE bool IsDashing() const { return bIsDashing; }
	
	UFUNCTION(BlueprintCallable, Category = "Movement")
	FORCEINLINE bool IsSliding() const { return bIsSliding; }
	
	UFUNCTION(BlueprintCallable, Category = "Movement")
	FORCEINLINE bool IsInvincible() const { return bIsInvincible; }
	
	UFUNCTION(BlueprintCallable, Category = "Movement")
	FORCEINLINE bool IsAiming() const { return bIsAiming; }
	
	UFUNCTION(BlueprintCallable, Category = "Movement")
	FORCEINLINE ELSMovementState GetCurrentMovementState() const { return CurrentMovementState; }
	
	//重写底层速度计算
	virtual float GetMaxSpeed() const override;
	
protected:
	virtual void BeginPlay() override;
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
	
private:
	//内部标记
	bool bWantsToSprint = false;
	bool bIsAiming = false;
	bool bIsDashing = false;
	bool bIsSliding = false;
	bool bIsInvincible = false;
	bool bCanDash = true;
	
	ELSMovementState CurrentMovementState = ELSMovementState::Normal;
	
	//计时器句柄
	FTimerHandle TimerHandle_DashCooldown;
	FTimerHandle TimerHandle_DashIFrame;
	FTimerHandle TimerHandle_Slide;
	
	float DefaultBrakingDeceleration = 2048.f;
	float DefaultGroundFriction = 8.f;
	
	//计时器结束回调
	void OnDashCooldownFinished();
	void OnDashIFrameFinished();
	void UpdateMovementState();
};
