#include "LSMovementComponent.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "Engine/World.h"

ULSMovementComponent::ULSMovementComponent()
{
	// 设置默认参数
	MaxWalkSpeed = WalkSpeed;
	MaxWalkSpeedCrouched = 300.f;
	BrakingFrictionFactor = 2.f;
	bUseControllerDesiredRotation = false;
	bOrientRotationToMovement = false;
}

void ULSMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	// 保存默认的减速度和地面摩擦力
	DefaultBrakingDeceleration = BrakingDecelerationWalking;
	DefaultGroundFriction = GroundFriction;
}

void ULSMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	UpdateMovementState();
	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);
}

void ULSMovementComponent::StartSprint()
{
	//开镜或下蹲状态下无法冲刺
	if (bIsAiming || IsCrouching()) return;
	
	bWantsToSprint = true;
}

void ULSMovementComponent::StopSprint()
{
	bWantsToSprint = false;
}

void ULSMovementComponent::SetAiming(bool IsAiming)
{
	bIsAiming = IsAiming;
	//开镜状态下无法冲刺
	if (bIsAiming)
	{
		bWantsToSprint = false;
	}
}

bool ULSMovementComponent::TryDash()
{
	//检查冷却和角色有效性
	if (!bCanDash || !CharacterOwner || bIsDashing) return false;
	
	//1，获取闪避方向，优先使用当前移动输入方向，若静止则character朝向为闪避方向
	FVector DashDirection = Velocity.GetSafeNormal2D();
	if (DashDirection.IsNearlyZero())
	{
		DashDirection = CharacterOwner->GetActorForwardVector();
	}
	
	//2,进入闪避状态并施加水平冲量
	bIsDashing = true;
	bCanDash = false;
	bIsInvincible = true;
	OnInvincibilityChanged.Broadcast(true);
	
	//清理原有垂直速度，直接水平爆发
	Velocity = DashDirection * DashImpulse;
	
	//3，启动无敌帧和冷却计时器
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(TimerHandle_DashIFrame, this, &ULSMovementComponent::OnDashIFrameFinished, DashIFrameDuration, false);
		World->GetTimerManager().SetTimer(TimerHandle_DashCooldown, this, &ULSMovementComponent::OnDashCooldownFinished, DashCooldown, false);
	}
	
	return true;
}

void ULSMovementComponent::OnDashIFrameFinished()
{
	bIsInvincible = false;
	bIsDashing = false;
	OnInvincibilityChanged.Broadcast(false);
}

void ULSMovementComponent::OnDashCooldownFinished()
{
	bCanDash = true;
}

void ULSMovementComponent::StartSlide()
{
	//只有在冲刺且在地面的时候才能滑铲
	if (!bWantsToSprint || !IsMovingOnGround() || bIsSliding)
	{
		return;
	}
	
	bIsSliding = true;
	bWantsToSprint = false;
	
	//降低摩擦力，赋予初速度
	GroundFriction = SlideFriction;
	BrakingDecelerationWalking = 400.f;
	
	FVector SlideDirection = Velocity.GetSafeNormal2D();
	if (SlideDirection.IsNearlyZero())
	{
		SlideDirection = CharacterOwner->GetActorForwardVector();
	}
	Velocity = SlideDirection * SlideSpeed;
	
	//开启滑铲最大持续计时
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(TimerHandle_Slide, this, &ULSMovementComponent::StopSlide, SlideDuration, false);
	}
}

void ULSMovementComponent::StopSlide()
{
	if (!bIsSliding) return;
	
	bIsSliding = false;
	
	//恢复摩擦力和减速度
	GroundFriction = DefaultGroundFriction;
	BrakingDecelerationWalking = DefaultBrakingDeceleration;
	
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TimerHandle_Slide);
	}
}

void ULSMovementComponent::UpdateMovementState()
{
	if (bIsDashing)
	{
		CurrentMovementState = ELSMovementState::Dashing;
	}
	else if (bIsSliding)
	{
		CurrentMovementState = ELSMovementState::Sliding;
	}
	else if (bIsAiming)
	{
		CurrentMovementState = ELSMovementState::Aiming;
	}
	else if (bWantsToSprint && Velocity.SizeSquared2D() > 100.f)
	{
		CurrentMovementState = ELSMovementState::Sprinting;
	}
	else
	{
		CurrentMovementState = ELSMovementState::Normal;
	}
}

float ULSMovementComponent::GetMaxSpeed() const
{
	//重写底层速度，让引擎各个移动逻辑自动读取当前状态对应的速度
	switch (CurrentMovementState)
	{
	case ELSMovementState::Aiming:
		return ADSSpeed;
		
	case ELSMovementState::Sprinting:
		return SprintSpeed;
		
	case ELSMovementState::Sliding:
		return SlideSpeed;
		
	case ELSMovementState::Dashing:
		return DashImpulse;
		
	case ELSMovementState::Normal:
	default:
		return IsCrouching() ? MaxWalkSpeedCrouched : WalkSpeed;
	}
}