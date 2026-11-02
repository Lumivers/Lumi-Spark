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

class UAnimMontage;

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


	//闪避参数（DASH）
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Dash")
	float DashImpulse = 2200.f; //闪避冲量

	UPROPERTY(EditDefaultsOnly, Category = "Movement|Dash")
	float DashCooldown = 0.5f; //闪避冷却时间
	
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Dash")
	float DashIFrameDuration = 0.2f; //闪避无敌时间

	// 滑铲参数（在 UE 细节面板 Movement | Slide 中直接修改，不在 C++ 里写任何隐藏计算）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Slide")
	float SlideSpeed = 1200.f; // 滑铲初速度（你在 UE 细节面板填多少就是多少）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Slide")
	float SlideDuration = 1.2f; // 滑铲最大保护时间（超时强制结束）

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Slide")
	float SlideFriction = 0.4f; // 滑铲阻尼摩擦力（0.3~0.5 像抹了黄油一样丝滑）
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Slide")
	float SlideBrakingDeceleration = 150.f; // 滑铲制动减速度

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Slide")
	float SlideMinSpeedToEnd = 400.f; // 速度自然衰减至该值时平滑起身

	// 滑铲动作蒙太奇
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Slide")
	TObjectPtr<UAnimMontage> SlideMontage;

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
	float DefaultMaxAcceleration = 2048.f;

	//计时器结束回调
	void OnDashCooldownFinished();
	void OnDashIFrameFinished();
	void UpdateMovementState();
};

