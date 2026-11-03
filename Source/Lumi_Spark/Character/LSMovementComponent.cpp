#include "LSMovementComponent.h"
#include "LSCharacterBase.h"
#include "Character/LSStaminaComponent.h"
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
	bOrientRotationToMovement = true; // 角色移动时平滑转向移动方向
	RotationRate = FRotator(0.0f, 720.0f, 0.0f);
}

void ULSMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	// 保存默认的减速度、地面摩擦力和最大加速度
	DefaultBrakingDeceleration = BrakingDecelerationWalking;
	DefaultGroundFriction = GroundFriction;
	DefaultMaxAcceleration = MaxAcceleration;
}

void ULSMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	UpdateMovementState();
	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);

	if (bWantsToSprint && IsMovingOnGround())
	{
		if (ALSCharacterBase* LSChar = Cast<ALSCharacterBase>(CharacterOwner))
		{
			if (ULSStaminaComponent* StaminaComp = LSChar->GetStaminaComponent())
			{
				const float Cost = StaminaComp->GetSprintCostPerSecond() * DeltaSeconds;
				if (!StaminaComp->ConsumeStamina(Cost))
				{
					StopSprint(); // 体力耗尽，强制切回慢跑
				}
			}
		}
	}
}

void ULSMovementComponent::StartSprint()
{
	// 开镜或下蹲状态下无法冲刺
	if (bIsAiming || IsCrouching()) return;

	bWantsToSprint = true;
	MaxWalkSpeed = SprintSpeed;
	if (ALSCharacterBase* LSChar = Cast<ALSCharacterBase>(CharacterOwner))
	{
		LSChar->SetALSRotationMode(LSChar->bAlwaysOrientToControlRotation ? 1 : 0);
		LSChar->SetALSDesiredGait(2); // 2 = Sprinting
	}
}

void ULSMovementComponent::StopSprint()
{
	bWantsToSprint = false;
	MaxWalkSpeed = WalkSpeed;
	if (ALSCharacterBase* LSChar = Cast<ALSCharacterBase>(CharacterOwner))
	{
		LSChar->SetALSDesiredGait(1); // 1 = Running
		LSChar->SetALSRotationMode(bIsAiming ? 2 : 1);
	}
}

void ULSMovementComponent::SetAiming(bool IsAiming)
{
	bIsAiming = IsAiming;
	if (bIsAiming)
	{
		bWantsToSprint = false;
		MaxWalkSpeed = WalkSpeed;
		if (ALSCharacterBase* LSChar = Cast<ALSCharacterBase>(CharacterOwner))
		{
			LSChar->SetALSDesiredGait(1); // 1 = Running
			LSChar->SetALSRotationMode(2);
		}
	}
	else
	{
		if (ALSCharacterBase* LSChar = Cast<ALSCharacterBase>(CharacterOwner))
		{
			LSChar->SetALSRotationMode(LSChar->bAlwaysOrientToControlRotation ? 1 : 0);
		}
	}
}

bool ULSMovementComponent::TryDash()
{
	// 检查冷却和角色有效性
	if (!bCanDash || !CharacterOwner || bIsDashing) return false;

	// 接入体力校验
	if (ALSCharacterBase* LSChar = Cast<ALSCharacterBase>(CharacterOwner))
	{
		if (ULSStaminaComponent* StaminaComp = LSChar->GetStaminaComponent())
		{
			if (!StaminaComp->ConsumeStamina(StaminaComp->GetDashCost()))
			{
				return false; // 体力不足，拒绝闪避
			}
		}
	}
	
	// 1. 获取闪避方向，优先使用当前移动输入方向，若静止则 character 朝向为闪避方向
	FVector DashDirection = Velocity.GetSafeNormal2D();
	if (DashDirection.IsNearlyZero())
	{
		DashDirection = CharacterOwner->GetActorForwardVector();
	}

	// 2. 进入闪避状态并施加水平冲量
	bIsDashing = true;
	bCanDash = false;
	bIsInvincible = true;
	OnInvincibilityChanged.Broadcast(true);

	// 清理原有垂直速度，直接水平爆发
	Velocity = DashDirection * DashImpulse;

	// 3. 启动无敌帧和冷却计时器
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

void ULSMovementComponent::UpdateMovementState()
{
	if (bIsDashing)
	{
		CurrentMovementState = ELSMovementState::Dashing;
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
	// 重写底层速度，让引擎各个移动逻辑自动读取当前状态对应的速度
	switch (CurrentMovementState)
	{
	case ELSMovementState::Aiming:
		return ADSSpeed;
	case ELSMovementState::Sprinting:
		return SprintSpeed;
	case ELSMovementState::Dashing:
		return DashImpulse;
	case ELSMovementState::Normal:
	default:
		return IsCrouching() ? MaxWalkSpeedCrouched : WalkSpeed;
	}
}
