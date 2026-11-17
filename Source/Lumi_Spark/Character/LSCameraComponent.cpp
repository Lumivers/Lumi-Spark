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

		// 开启弹簧臂平滑位置滞后（灵敏跟手，消除机械式生硬硬拉）
		SpringArm->bEnableCameraLag = bEnableCameraLag;
		SpringArm->CameraLagSpeed = CameraLagSpeed;
		SpringArm->CameraLagMaxDistance = 80.0f;

		// 关键安全修复：将射线探针探头半径从默认 12cm 优化为 6cm，杜绝台阶棱角虚假碰撞
		SpringArm->ProbeSize = 6.0f;

		// 关键安全修复：TargetOffset 严格置零！探针发射起点永远保持在角色肩颈的安全空气中，绝不插进泥土
		SpringArm->TargetOffset = FVector::ZeroVector;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[LSCameraComponent] 未检测到 SpringArm 组件！当前依附父级: %s"), GetAttachParent() ? *GetAttachParent()->GetName() : TEXT("None"));
	}

	// 缓存角色指针，并初始化世界高程基准
	CachedOwnerCharacter = Cast<ACharacter>(GetOwner());
	if (CachedOwnerCharacter.IsValid())
	{
		SmoothedCameraPivotWorldZ = CachedOwnerCharacter->GetActorLocation().Z;
		bHasInitializedWorldZ = true;
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

	// 1. 垂直高程非对称延后阻尼与死区滤波算法（彻底杜绝穿模挤压，实现从容延后上移）
	if (bEnableVerticalDamping)
	{
		if (!CachedOwnerCharacter.IsValid() && GetOwner())
		{
			CachedOwnerCharacter = Cast<ACharacter>(GetOwner());
		}

		if (CachedOwnerCharacter.IsValid())
		{
			const float CurrentCharacterZ = CachedOwnerCharacter->GetActorLocation().Z;

			if (!bHasInitializedWorldZ)
			{
				SmoothedCameraPivotWorldZ = CurrentCharacterZ;
				bHasInitializedWorldZ = true;
			}

			// 计算当前角色世界高程与平滑世界高程的差值
			const float RawDiffZ = CurrentCharacterZ - SmoothedCameraPivotWorldZ;

			// 死区滤波：平地奔跑与微小碎步颠簸在死区内（<= VerticalDeadZone）完全忽略，保持镜头极其平稳
			float FilteredTargetZ = SmoothedCameraPivotWorldZ;
			if (FMath::Abs(RawDiffZ) > VerticalDeadZone)
			{
				FilteredTargetZ = CurrentCharacterZ;
			}

			// 非对称延后插值速率：
			// 爬墙上升使用较慢从容的 UpwardSmoothingSpeed (4.5f)，让动作先做，镜头明显延后平缓悠移上去；
			// 下落使用稍快的 DownwardSmoothingSpeed (8.0f)
			const bool bIsClimbingUp = (FilteredTargetZ > SmoothedCameraPivotWorldZ);
			const float CurrentSmoothingSpeed = bIsClimbingUp ? UpwardSmoothingSpeed : DownwardSmoothingSpeed;

			SmoothedCameraPivotWorldZ = FMath::FInterpTo(SmoothedCameraPivotWorldZ, FilteredTargetZ, DeltaTime, CurrentSmoothingSpeed);

			// 计算延后滞后带来的垂直高度补偿量（负值代表角色已上去但镜头延后在下方）
			float HeightLagZ = SmoothedCameraPivotWorldZ - CurrentCharacterZ;
			// 约束最大滞后距离，防穿模
			HeightLagZ = FMath::Clamp(HeightLagZ, -MaxVerticalLagDistance, MaxVerticalLagDistance);

			CurrentVerticalCompensationZ = FMath::FInterpTo(CurrentVerticalCompensationZ, HeightLagZ, DeltaTime, CurrentSmoothingSpeed);
		}
	}
	else
	{
		CurrentVerticalCompensationZ = 0.0f;
	}

	// 2. 关键几何安全解法：将高度补偿施加在末端 SocketOffset 上，绝不触碰探针起点 TargetOffset！
	// 探针发射起点始终保留在角色肩颈的安全空气中，彻底根除台阶穿模挤压（第二张图现象）
	FVector EffectiveSocketOffset = TargetSocketOffset;
	EffectiveSocketOffset.Z += CurrentVerticalCompensationZ;
	SpringArm->SocketOffset = FMath::VInterpTo(SpringArm->SocketOffset, EffectiveSocketOffset, DeltaTime, TransitionSpeed);

	// 确保探针起点 TargetOffset 始终安全归零，绝不插进泥土
	SpringArm->TargetOffset = FVector::ZeroVector;

	// 3. 平滑插值弹簧臂长
	SpringArm->TargetArmLength = FMath::FInterpTo(SpringArm->TargetArmLength, TargetArmLength, DeltaTime, TransitionSpeed);

	// 4. 平滑插值摄像机广角 FOV
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
