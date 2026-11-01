#include "LSCameraComponent.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "LSMovementComponent.h"
#include "Character/LSCharacterBase.h"

ULSCameraComponent::ULSCameraComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bUsePawnControlRotation = false; // 摄像机自身不旋转，由弹簧臂带动
}

void ULSCameraComponent::BeginPlay()
{
	Super::BeginPlay();

	PrimaryComponentTick.SetTickFunctionEnable(true);

	// 【安全获取弹簧臂】：优先使用父级（C++原生CameraBoom），备选通过Owner查找
	if (!SpringArm)
	{
		SpringArm = Cast<USpringArmComponent>(GetAttachParent());
		if (!SpringArm && GetOwner())
		{
			SpringArm = GetOwner()->FindComponentByClass<USpringArmComponent>();
		}
	}

	if (SpringArm)
	{
		if (GetAttachParent() != SpringArm)
		{
			AttachToComponent(SpringArm, FAttachmentTransformRules::SnapToTargetNotIncludingScale, USpringArmComponent::SocketName);
		}
		SetRelativeLocation(FVector::ZeroVector);
		SetRelativeRotation(FRotator::ZeroRotator);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[LSCameraComponent] 未检测到 SpringArm 组件！当前依附父级: %s"), GetAttachParent() ? *GetAttachParent()->GetName() : TEXT("None"));
	}

	SetCameraMode(CurrentMode);
}

void ULSCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateCameraInterpolation(DeltaTime);
}

void ULSCameraComponent::SetCameraMode(ELSCameraMode NewMode)
{
	CurrentMode = NewMode;

	if (!SpringArm && GetOwner())
	{
		SpringArm = GetOwner()->FindComponentByClass<USpringArmComponent>();
	}

	if (SpringArm)
	{
		SpringArm->bUsePawnControlRotation = true;
		if (CurrentMode == ELSCameraMode::ThirdPerson)
		{
			// 第三人称：恢复玩家滚轮设置的臂长，右肩偏 (0, 50, 65)
			TargetArmLength = CustomTPSArmLength;
			TargetSocketOffset = FVector(0.0f, 50.0f, 65.0f);
			SpringArm->bDoCollisionTest = true;
			TargetFov = TPSFov;
		}
		else if (CurrentMode == ELSCameraMode::OverShoulder)
		{
			// 过肩瞄准：臂长 140，右肩偏 (0, 65, 65)，视线越过右肩瞄准
			TargetArmLength = 140.0f;
			TargetSocketOffset = FVector(0.0f, 65.0f, 65.0f);
			SpringArm->bDoCollisionTest = true;
			TargetFov = ShoulderFov;
		}
	}
}

void ULSCameraComponent::EnterADS()
{
	if (bIsInADS) return;
	bIsInADS = true;
	PreADSMode = CurrentMode;
	TargetFov = ShoulderFov;
	
	// 开镜瞄准：切入过肩瞄准
	SetCameraMode(ELSCameraMode::OverShoulder);
}

void ULSCameraComponent::ExitADS()
{
	if (!bIsInADS) return;
	bIsInADS = false;
	
	TargetFov = TPSFov;
	SetCameraMode(ELSCameraMode::ThirdPerson);
}

void ULSCameraComponent::UpdateCameraInterpolation(float DeltaTime)
{
	if (!SpringArm) return;

	// 1. 平滑插值弹簧臂长与偏移
	SpringArm->TargetArmLength = FMath::FInterpTo(SpringArm->TargetArmLength, TargetArmLength, DeltaTime, TransitionSpeed);
	SpringArm->SocketOffset = FMath::VInterpTo(SpringArm->SocketOffset, TargetSocketOffset, DeltaTime, TransitionSpeed);

	// 2. 平滑插值摄像机广角 FOV
	FieldOfView = FMath::FInterpTo(FieldOfView, TargetFov, DeltaTime, TransitionSpeed);
}

void ULSCameraComponent::AdjustCameraZoom(float ZoomDelta)
{
	if (!bIsInADS)
	{
		CustomTPSArmLength = FMath::Clamp(CustomTPSArmLength - (ZoomDelta * ZoomStep), MinArmLength, MaxArmLength);
		TargetArmLength = CustomTPSArmLength;
		if (SpringArm)
		{
			GEngine->AddOnScreenDebugMessage(54321, 1.5f, FColor::Yellow, FString::Printf(TEXT("🔭 [Camera] 臂长已调整: %.0f (范围: %.0f~%.0f)"), TargetArmLength, MinArmLength, MaxArmLength));
		}
	}
}
