#include "LSAnimInstance.h"
#include "LSCharacterBase.h"
#include "LSMovementComponent.h"
#include "LSCameraComponent.h"
#include "Kismet/KismetMathLibrary.h"

void ULSAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	// 缓存角色与移动组件指针
	Character = Cast<ALSCharacterBase>(TryGetPawnOwner());
	if (Character)
	{
		MovementComponent = Character->GetLSMovementComponent();
	}
}

void ULSAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// 容错与懒加载重试
	if (!Character)
	{
		Character = Cast<ALSCharacterBase>(TryGetPawnOwner());
		if (Character)
		{
			MovementComponent = Character->GetLSMovementComponent();
		}
	}

	if (!Character || !MovementComponent)
	{
		return;
	}

	// 1. 水平地面速度与方向
	const FVector Velocity = Character->GetVelocity();
	const FVector HorizontalVelocity = FVector(Velocity.X, Velocity.Y, 0.0f);
	GroundSpeed = HorizontalVelocity.Size();

	const FRotator ActorRotation = Character->GetActorRotation();
	Direction = CalculateDirection(Velocity, ActorRotation);

	// 2. 加速度与移动判定
	const FVector Acceleration = MovementComponent->GetCurrentAcceleration();
	bShouldMove = (GroundSpeed > 3.0f) && (!Acceleration.IsNearlyZero(0.1f));

	// 3. 滞空状态
	bIsFalling = MovementComponent->IsFalling();

	// 4. 战术动作状态（从自定义移动组件提取）
	bIsCrouching = MovementComponent->IsCrouching();
	bIsSprinting = MovementComponent->IsSprinting();
	bIsSliding = MovementComponent->IsSliding();
	bIsDashing = MovementComponent->IsDashing();

	// 7. 提取开镜瞄准状态 (ADS)
	if (ULSCameraComponent* Cam = Character->GetCameraComponent())
	{
		bIsAiming = Cam->IsADS();
	}
	else if (MovementComponent)
	{
		bIsAiming = MovementComponent->IsAiming();
	}
	else
	{
		bIsAiming = false;
	}

	const FRotator AimRotation = Character->GetBaseAimRotation();
	const FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(AimRotation, ActorRotation);
	AimPitch = DeltaRot.Pitch;
	AimYaw = DeltaRot.Yaw;
}
