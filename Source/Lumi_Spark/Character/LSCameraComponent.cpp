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
		// 仅当摄像机尚未挂载在弹簧臂下时才执行 Attach，避免破坏原生装配树
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
		if (CurrentMode == ELSCameraMode::FirstPerson)
		{
			// 第一人称：臂长 0，无偏移，关碰撞
			TargetArmLength = 0.0f;
			TargetSocketOffset = FVector::ZeroVector;
			SpringArm->bDoCollisionTest = false;
			TargetFov = FPSFov;
		}
		else if (CurrentMode == ELSCameraMode::ThirdPerson)
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
	TargetFov = ShoulderFov;
	
	// 开镜瞄准：身体正对准星，指哪打哪
	if (ALSCharacterBase* Char = Cast<ALSCharacterBase>(GetOwner()))
	{
		Char->bUseControllerRotationYaw = true;
		if (ULSMovementComponent* MoveComp = Char->GetLSMovementComponent())
		{
			MoveComp->bOrientRotationToMovement = false;
		}
	}

	// 无论之前是第一人称还是第三人称，瞄准均切入过肩瞄准
	SetCameraMode(ELSCameraMode::OverShoulder);
}

void ULSCameraComponent::ExitADS()
{
	if (!bIsInADS) return;
	bIsInADS = false;
	
	// 退出瞄准：解绑视角，镜头可 360 度自由旋转观察角色
	if (ALSCharacterBase* Char = Cast<ALSCharacterBase>(GetOwner()))
	{
		Char->bUseControllerRotationYaw = false;
		if (ULSMovementComponent* MoveComp = Char->GetLSMovementComponent())
		{
			MoveComp->bOrientRotationToMovement = true;
		}
	}

	if (PreADSMode == ELSCameraMode::FirstPerson)
	{
		TargetFov = FPSFov;
	}
	else if (PreADSMode == ELSCameraMode::ThirdPerson)
	{
		TargetFov = TPSFov;
		SetCameraMode(ELSCameraMode::ThirdPerson);
	}
}


void ULSCameraComponent::UpdateCameraInterpolation(float DeltaTime)
{
	FVector FinalSocketOffset = TargetSocketOffset;
	float FinalFov = TargetFov;
	
	//检测滑铲状态
	if (ACharacter* Char = Cast<ACharacter>(GetOwner()))
	{
		if (ULSMovementComponent* MoveComp = Cast<ULSMovementComponent>(Char->GetCharacterMovement()))
		{
			if (MoveComp->IsSliding())
			{
				//滑铲时摄像机降低高度
				FinalSocketOffset.Z -= 45.0f;
				FinalFov += 6.0f; //略微拉远视野
			}
		}
	}
	
	if (SpringArm)
	{
		// 1. 平滑伸缩弹簧臂（第一人称 0 <-> 第三人称 300）
		SpringArm->TargetArmLength = FMath::FInterpTo(SpringArm->TargetArmLength, TargetArmLength, DeltaTime, TransitionSpeed);

		// 2. 平滑过渡右肩偏移
		SpringArm->SocketOffset = FMath::VInterpTo(SpringArm->SocketOffset, FinalSocketOffset, DeltaTime, TransitionSpeed);
	}

	// 3. 平滑过渡 FOV
	FieldOfView = FMath::FInterpTo(FieldOfView, FinalFov, DeltaTime, TransitionSpeed);
	
	// 摄像机始终固定在弹簧臂末端
	SetRelativeLocation(FVector::ZeroVector);
	SetRelativeRotation(FRotator::ZeroRotator);
}

void ULSCameraComponent::UpdateMeshVisibility()
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter || !OwnerCharacter->GetMesh()) return;

	USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
	ALSCharacterBase* LSChar = Cast<ALSCharacterBase>(OwnerCharacter);
	USkeletalMeshComponent* ArmsMesh = LSChar ? LSChar->GetFPArmsMesh() : nullptr;

	if (CurrentMode == ELSCameraMode::FirstPerson)
	{
		// 第一人称：隐藏全身，投射地面阴影
		Mesh->SetOwnerNoSee(true);
		Mesh->bCastHiddenShadow = true;
		if (ArmsMesh) ArmsMesh->SetVisibility(true);
	}
	else
	{
		// 第三人称：显示全身
		Mesh->SetOwnerNoSee(false);
		if (ArmsMesh) ArmsMesh->SetVisibility(false);
	}
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