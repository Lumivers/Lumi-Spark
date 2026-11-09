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

	// 8. 武器开火与左手 IK 状态预提取
	bIsFiring = false;
	if (ULSWeaponComponent* WeaponComp = Character->GetWeaponComponent())
	{
		if (ALSWeaponBase* CurrentWeapon = WeaponComp->GetCurrentWeapon())
		{
			bIsFiring = CurrentWeapon->IsFiring();
		}
	}

	const FRotator AimRotation = Character->GetBaseAimRotation();
	const FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(AimRotation, ActorRotation);
	// 战斗对齐：无论是右键开镜瞄准 (bIsAiming) 还是左键开火射击 (bIsFiring)，均激活身体与脊椎俯仰对齐准星！
	const bool bCombatActive = bIsAiming || bIsFiring;
	const float TargetPitch = bCombatActive ? DeltaRot.Pitch : 0.0f;
	const float TargetYaw = bCombatActive ? DeltaRot.Yaw : 0.0f;
	if (bIsFiring)
	{
		// 开火时极速对齐准星，消灭脊椎仰俯滞后
		AimPitch = TargetPitch;
		AimYaw = TargetYaw;
	}
	else
	{
		AimPitch = FMath::FInterpTo(AimPitch, TargetPitch, DeltaSeconds, 15.0f);
		AimYaw = FMath::FInterpTo(AimYaw, TargetYaw, DeltaSeconds, 15.0f);
	}

	bool bFoundLeftHandSocket = false;
	if (Character)
	{
		if (ULSWeaponComponent* WeaponComp = Character->GetWeaponComponent())
		{
			if (ALSWeaponBase* CurrentWeapon = WeaponComp->GetCurrentWeapon())
			{
				bIsFiring = CurrentWeapon->IsFiring();

				// 换弹过程中左手需要离开护木拔插弹匣，必须禁用左手 IK
				if (!CurrentWeapon->IsReloading())
				{
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
	}

	// 平滑插值 Alpha（切枪或收枪时平滑过渡）
	const float TargetAlpha = bFoundLeftHandSocket ? 1.0f : 0.0f;
	LeftHandIKAlpha = FMath::FInterpTo(LeftHandIKAlpha, TargetAlpha, DeltaSeconds, 15.0f);

	// 9. 脊椎开火受力后坐力数据同步与回正
	if (Character)
	{
		SpineRecoilRotation = Character->SpineRecoilRotation;
		SpineRecoilLocation = Character->SpineRecoilLocation;
	}
	else
	{
		SpineRecoilRotation = FMath::RInterpTo(SpineRecoilRotation, FRotator::ZeroRotator, DeltaSeconds, RecoilSpineRecoverySpeed);
		SpineRecoilLocation = FMath::VInterpTo(SpineRecoilLocation, FVector::ZeroVector, DeltaSeconds, RecoilSpineRecoverySpeed);
	}
}

void ULSAnimInstance::AddRecoilKick(float Multiplier)
{
	if (Character)
	{
		Character->AddSpineRecoilKick(Multiplier);
		SpineRecoilRotation = Character->SpineRecoilRotation;
		SpineRecoilLocation = Character->SpineRecoilLocation;
		return;
	}

	if (Multiplier <= 0.0f)
	{
		return;
	}

	const float PitchDelta = RecoilSpinePitch * Multiplier;
	SpineRecoilRotation.Pitch = FMath::Clamp(SpineRecoilRotation.Pitch + PitchDelta, -MaxRecoilSpinePitch, MaxRecoilSpinePitch);

	const float YawDelta = FMath::FRandRange(RecoilSpineYaw * 0.5f, RecoilSpineYaw) * Multiplier;
	SpineRecoilRotation.Yaw = FMath::Clamp(SpineRecoilRotation.Yaw + YawDelta, -3.0f, 3.0f);

	const float RollDelta = FMath::FRandRange(-RecoilSpineRoll, RecoilSpineRoll) * Multiplier;
	SpineRecoilRotation.Roll = FMath::Clamp(SpineRecoilRotation.Roll + RollDelta, -2.0f, 2.0f);

	const float BackDelta = RecoilSpineBack * Multiplier;
	SpineRecoilLocation.X = FMath::Clamp(SpineRecoilLocation.X - BackDelta, -MaxRecoilSpineBack, 0.0f);
	SpineRecoilLocation.Z = FMath::Clamp(SpineRecoilLocation.Z + (BackDelta * 0.2f), -1.0f, 1.0f);
}

