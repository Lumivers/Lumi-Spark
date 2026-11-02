#include "LSMovementComponent.h"
#include "LSCharacterBase.h"
#include "Character/LSStaminaComponent.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Animation/AnimMontage.h"

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

	// 滑铲状态下的物理自然衰减检查
	if (bIsSliding)
	{
		// 腾空或速度由于地面阻尼已自然衰减至慢跑速度以下，顺畅起身退出滑铲
		if (!IsMovingOnGround() || Velocity.Size2D() < SlideMinSpeedToEnd)
		{
			StopSlide();
		}
	}

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

	GEngine->AddOnScreenDebugMessage(1110, 2.5f, FColor::Purple, 
		FString::Printf(TEXT("💨 [TryDash] 闪避爆发冲刺！冲量: %.1f"), DashImpulse));

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

void ULSMovementComponent::StartSlide()
{
	// 1. 地面检测
	if (!IsMovingOnGround())
	{
		GEngine->AddOnScreenDebugMessage(1105, 2.0f, FColor::Red, TEXT("❌ [StartSlide] 失败：角色不在地面上！"));
		return;
	}

	// 2. 已经在滑铲中
	if (bIsSliding)
	{
		return;
	}

	// 3. 速度判定：只要在移动（速度 > 100）或正在疾跑，即可进入滑铲
	if (Velocity.Size2D() < 100.f && !bWantsToSprint)
	{
		GEngine->AddOnScreenDebugMessage(1105, 2.0f, FColor::Red, TEXT("❌ [StartSlide] 原地静止无法滑铲！"));
		return;
	}

	bIsSliding = true;
	bWantsToSprint = false;
	MaxWalkSpeed = WalkSpeed;
	if (ALSCharacterBase* LSChar = Cast<ALSCharacterBase>(CharacterOwner))
	{
		LSChar->SetALSDesiredGait(1);
		LSChar->SetALSRotationMode(bIsAiming ? 2 : 1);
	}

	// 4. 下蹲身位
	if (CharacterOwner)
	{
		CharacterOwner->Crouch();
	}

	// 5. 切断动力推进，直接应用在 UE 细节面板配置的摩擦力与减速度
	MaxAcceleration = 0.f;
	GroundFriction = SlideFriction;
	BrakingDecelerationWalking = SlideBrakingDeceleration;

	// 6. 方向与初速度：直接使用 UE 细节面板里填的 SlideSpeed，不做任何隐藏公式计算！
	FVector SlideDirection = Velocity.GetSafeNormal2D();
	if (SlideDirection.IsNearlyZero())
	{
		SlideDirection = CharacterOwner ? CharacterOwner->GetActorForwardVector() : FVector::ForwardVector;
	}
	Velocity = SlideDirection * SlideSpeed;

	GEngine->AddOnScreenDebugMessage(1106, 2.5f, FColor::Green, 
		FString::Printf(TEXT("🛷 [StartSlide] 成功进入滑铲！初速度: %.1f | 摩擦阻尼: %.2f | 减速度: %.1f"), SlideSpeed, SlideFriction, SlideBrakingDeceleration));

	// 7. 播放滑铲动作蒙太奇
	if (SlideMontage && CharacterOwner)
	{
		CharacterOwner->PlayAnimMontage(SlideMontage);
	}
	
	// 8. 开启最大保护计时器（直接使用 UE 细节面板里的 SlideDuration）
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(TimerHandle_Slide, this, &ULSMovementComponent::StopSlide, SlideDuration, false);
	}
}

void ULSMovementComponent::StopSlide()
{
	if (!bIsSliding) return;
	
	GEngine->AddOnScreenDebugMessage(1108, 1.5f, FColor::Yellow, TEXT("🏁 [StopSlide] 滑铲结束，起身恢复常规步态"));

	bIsSliding = false;
	
	// 1. 滑铲结束站起来
	if (CharacterOwner)
	{
		CharacterOwner->UnCrouch();
		if (SlideMontage)
		{
			CharacterOwner->StopAnimMontage(SlideMontage);
		}
	}
	
	// 2. 彻底恢复物理马达动力、地面摩擦力和减速度
	MaxAcceleration = DefaultMaxAcceleration;
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
	// 重写底层速度，让引擎各个移动逻辑自动读取当前状态对应的速度
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
