#include "LSMovementComponent.h"
#include "LSCharacterBase.h"
#include "Character/LSStaminaComponent.h"
#include "Weapon/LSWeaponComponent.h"
#include "Weapon/LSWeaponBase.h"
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
	// 开镜、下蹲、或正在开火状态下无法进入冲刺
	if (bIsAiming || IsCrouching()) return;
	if (ALSCharacterBase* LSChar = Cast<ALSCharacterBase>(CharacterOwner))
	{
		if (ULSWeaponComponent* WeaponComp = LSChar->GetWeaponComponent())
		{
			if (ALSWeaponBase* CurWeapon = WeaponComp->GetCurrentWeapon())
			{
				if (CurWeapon->IsFiring())
				{
					return;
				}
			}
		}
	}

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
		// 若当前仍在开镜或正在开火，必须维持端枪姿势 2 (Aiming)
		bool bCombatActive = bIsAiming;
		if (ULSWeaponComponent* WeaponComp = LSChar->GetWeaponComponent())
		{
			if (ALSWeaponBase* CurWeapon = WeaponComp->GetCurrentWeapon())
			{
				if (CurWeapon->IsFiring())
				{
					bCombatActive = true;
				}
			}
		}

		// 核心：战术射击游戏非冲刺状态永远是 Gait 0（持枪稳定，绝不进入 Gait 1 奔跑甩手动画）！
		LSChar->SetALSDesiredGait(0);
		LSChar->SetALSRotationMode(bCombatActive ? 2 : (LSChar->bAlwaysOrientToControlRotation ? 1 : 0));
	}
}

void ULSMovementComponent::SetAiming(bool IsAiming)
{
	bIsAiming = IsAiming;
	if (ALSCharacterBase* LSChar = Cast<ALSCharacterBase>(CharacterOwner))
	{
		// 无论腰射还是开镜，常态步态全部锁死为 0 (Walking 战术端枪)，彻底关掉 Run_F_Arms 跑动甩枪！
		LSChar->SetALSDesiredGait(0);
		LSChar->SetALSRotationMode(bIsAiming ? 2 : (LSChar->bAlwaysOrientToControlRotation ? 1 : 0));
	}

	if (bIsAiming)
	{
		bWantsToSprint = false;
		MaxWalkSpeed = ADSSpeed;
	}
	else
	{
		MaxWalkSpeed = WalkSpeed;
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
