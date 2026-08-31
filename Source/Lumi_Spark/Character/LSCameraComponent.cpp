#include "LSCameraComponent.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"

ULSCameraComponent::ULSCameraComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bUsePawnControlRotation = false; // 摄像机自身不旋转，由弹簧臂带动
}

void ULSCameraComponent::BeginPlay()
{
	Super::BeginPlay();

	PrimaryComponentTick.SetTickFunctionEnable(true);

	// 【自愈防空指针】：如果蓝图缓存导致 SpringArm 为空，主动在 Owner 身上抓取！
	if (!SpringArm && GetOwner())
	{
		SpringArm = GetOwner()->FindComponentByClass<USpringArmComponent>();
	}

	if (SpringArm)
	{
		AttachToComponent(SpringArm, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
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
		if (CurrentMode == ELSCameraMode::FirstPerson)
		{
			// 第一人称：臂长 0，无偏移，关碰撞
			TargetArmLength = 0.0f;
			TargetSocketOffset = FVector::ZeroVector;
			SpringArm->bDoCollisionTest = false;
		}
		else if (CurrentMode == ELSCameraMode::ThirdPerson)
		{
			// 第三人称：臂长 300，右肩偏 (0, 50, 15)，开物理防穿墙！
			TargetArmLength = 300.0f;
			TargetSocketOffset = FVector(0.0f, 50.0f, 15.0f);
			SpringArm->bDoCollisionTest = true;
		}
		else if (CurrentMode == ELSCameraMode::OverShoulder)
		{
			if (PreADSMode == ELSCameraMode::FirstPerson)
			{
				// 【第一人称开镜】：臂长依然为 0，不偏移，纯拉近 FOV 到 60度！
				TargetArmLength = 0.0f;
				TargetSocketOffset = FVector::ZeroVector;
				SpringArm->bDoCollisionTest = false;
			}
			else
			{
				// 【第三人称开镜】：拉近到右肩 (0, 60, 10)，臂长 120
				TargetArmLength = 120.0f;
				TargetSocketOffset = FVector(0.0f, 60.0f, 10.0f);
				SpringArm->bDoCollisionTest = true;
			}
		}
	}

	UpdateMeshVisibility();
}

void ULSCameraComponent::ToggleCameraMode()
{
	if (bIsInADS) return;

	if (CurrentMode == ELSCameraMode::FirstPerson)
	{
		SetCameraMode(ELSCameraMode::ThirdPerson);
	}
	else
	{
		SetCameraMode(ELSCameraMode::FirstPerson);
	}
}

void ULSCameraComponent::EnterADS()
{
	if (bIsInADS) return;
	bIsInADS = true;
	PreADSMode = CurrentMode;
	SetCameraMode(ELSCameraMode::OverShoulder);
}

void ULSCameraComponent::ExitADS()
{
	if (!bIsInADS) return;
	bIsInADS = false;
	SetCameraMode(PreADSMode);
}

void ULSCameraComponent::UpdateCameraInterpolation(float DeltaTime)
{
	if (SpringArm)
	{
		// 1. 平滑伸缩弹簧臂（第一人称 0 <-> 第三人称 300）
		SpringArm->TargetArmLength = FMath::FInterpTo(SpringArm->TargetArmLength, TargetArmLength, DeltaTime, TransitionSpeed);

		// 2. 平滑过渡右肩偏移
		SpringArm->SocketOffset = FMath::VInterpTo(SpringArm->SocketOffset, TargetSocketOffset, DeltaTime, TransitionSpeed);
	}

	// 摄像机始终固定在弹簧臂末端
	SetRelativeLocation(FVector::ZeroVector);
	const float TargetFov = (CurrentMode == ELSCameraMode::OverShoulder) ? 60.0f : 90.0f;
	SetFieldOfView(FMath::FInterpTo(FieldOfView, TargetFov, DeltaTime, TransitionSpeed));
}

void ULSCameraComponent::UpdateMeshVisibility()
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter || !OwnerCharacter->GetMesh()) return;
	USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
	// 只要是第一人称，或者是在第一人称下开镜，统统隐藏身体！
	const bool bShouldHideBody = (CurrentMode == ELSCameraMode::FirstPerson) || (CurrentMode == ELSCameraMode::OverShoulder && PreADSMode == ELSCameraMode::FirstPerson);
	Mesh->SetOwnerNoSee(bShouldHideBody);
	Mesh->bCastHiddenShadow = bShouldHideBody;
}