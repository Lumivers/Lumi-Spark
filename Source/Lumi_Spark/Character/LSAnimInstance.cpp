#include "LSAnimInstance.h"
#include "LSCharacterBase.h"
#include "LSMovementComponent.h"
#include "LSCameraComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Animation/AnimInstance.h"
#include "KismetAnimationLibrary.h"
#include "Weapon/LSWeaponComponent.h"
#include "Weapon/LSWeaponBase.h"
#include "Components/SkeletalMeshComponent.h"


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
	Direction = UKismetAnimationLibrary::CalculateDirection(Velocity, ActorRotation);

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
	// 核心改进：仅在按住右键开镜瞄准 (bIsAiming) 时才触发身体与脊椎俯仰；自由观察视角下平滑归零保持挺拔身姿
	const float TargetPitch = bIsAiming ? DeltaRot.Pitch : 0.0f;
	const float TargetYaw = bIsAiming ? DeltaRot.Yaw : 0.0f;
	AimPitch = FMath::FInterpTo(AimPitch, TargetPitch, DeltaSeconds, 15.0f);
	AimYaw = FMath::FInterpTo(AimYaw, TargetYaw, DeltaSeconds, 15.0f);

	// 8. 武器状态与左手 IK 吸附位置提取
	bIsFiring = false;
	bool bFoundLeftHandSocket = false;
	if (Character)
	{
		if (ULSWeaponComponent* WeaponComp = Character->GetWeaponComponent())
		{
			if (ALSWeaponBase* CurrentWeapon = WeaponComp->GetCurrentWeapon())
			{
				bIsFiring = CurrentWeapon->IsFiring();

				FTransform WeaponLeftHandWorldTransform;
				if (CurrentWeapon->GetLeftHandSocketTransform(WeaponLeftHandWorldTransform))
				{
					if (USkeletalMeshComponent* CharMesh = Character->GetMesh())
					{
						// 转换至角色网格体的组件空间 (Component Space)
						LeftHandIKTransform = WeaponLeftHandWorldTransform.GetRelativeTransform(CharMesh->GetComponentTransform());
						LeftHandIKLocation = LeftHandIKTransform.GetLocation();
						LeftHandIKRotation = LeftHandIKTransform.Rotator();
						bFoundLeftHandSocket = true;
					}
				}
			}
		}
	}

	// 平滑插值 Alpha（切枪或收枪时平滑过渡）
	const float TargetAlpha = bFoundLeftHandSocket ? 1.0f : 0.0f;
	LeftHandIKAlpha = FMath::FInterpTo(LeftHandIKAlpha, TargetAlpha, DeltaSeconds, 15.0f);
}

