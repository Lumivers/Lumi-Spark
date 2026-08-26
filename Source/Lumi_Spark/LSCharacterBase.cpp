// Fill out your copyright notice in the Description page of Project Settings.


#include "LSCharacterBase.h"
#include "LSCameraComponent.h"
#include "LSMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

// 构造函数：用自定义的ULSMovementComponent 替换默认的CharacterMovementComponent
ALSCharacterBase::ALSCharacterBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer.SetDefaultSubobjectClass<ULSMovementComponent>(ACharacter::CharacterMovementComponentName))
{
 	// 开启Tick
	PrimaryActorTick.bCanEverTick = true;
	
	//1. 创建摄像机组件并附加到根碰撞胶囊体
	CameraComponent = CreateDefaultSubobject<ULSCameraComponent>(TEXT("LSCameraComponent"));
	CameraComponent->SetupAttachment(GetCapsuleComponent());
	CameraComponent->bUsePawnControlRotation = true; //摄像机跟随控制器旋转
	
	//2. 创建第一人称手臂Mesh组件并附加到摄像机组件
	FPArmsMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FPArmsMesh"));
	FPArmsMesh->SetupAttachment(CameraComponent);
	FPArmsMesh->SetOnlyOwnerSee(true); //仅本地玩家可见
	FPArmsMesh->bCastDynamicShadow = false; //不投射动态阴影
	FPArmsMesh->CastShadow = false; //不投射阴影
	FPArmsMesh->SetCollisionProfileName(TEXT("NoCollision")); //不参与碰撞
	
	//基础角色Mesh设置（第三人称全身模型）
	GetMesh()->SetupAttachment(GetCapsuleComponent());
	GetMesh()->bCastHiddenShadow = true; //隐藏时仍投射阴影
}

// Called when the game starts or when spawned
void ALSCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	
	//注册默认输入映射上下文
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (DefaultMappingContext){
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}
}

// Called every frame
void ALSCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void ALSCharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	//转化为Enhanced Input Component进行绑定
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		//移动
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ALSCharacterBase::Move);
		}
		
		//视角旋转
		if (LookAction)
		{
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ALSCharacterBase::Look);
		}
		
		//跳跃
		if (JumpAction)
		{
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		}
		
		//冲刺
		if (SprintAction)
		{
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &ALSCharacterBase::StartSprint);
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &ALSCharacterBase::StopSprint);
		}
		
		//闪避
		if (DashAction)
		{
			EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Started, this, &ALSCharacterBase::HandleDash);
		}
		
		//下蹲/滑铲
		if (CrouchAction)
		{
			EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &ALSCharacterBase::StartCrouchOrSlide);
			EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Completed, this, &ALSCharacterBase::StopCrouchOrSlide);
		}
		
		//切换视角模式
		if (ToggleCameraModeAction)
		{
			EnhancedInputComponent->BindAction(ToggleCameraModeAction, ETriggerEvent::Started, this, &ALSCharacterBase::HandleToggleCameraMode);
		}
		
		//开镜/过肩瞄准
		if (ADSAction)
		{
			EnhancedInputComponent->BindAction(ADSAction, ETriggerEvent::Started, this, &ALSCharacterBase::StartADS);
			EnhancedInputComponent->BindAction(ADSAction, ETriggerEvent::Completed, this, &ALSCharacterBase::StopADS);
			EnhancedInputComponent->BindAction(ADSAction, ETriggerEvent::Canceled, this, &ALSCharacterBase::StopADS);
		}
	}
}

//移动处理：根据控制器yaw朝向施加前后左右位移
void ALSCharacterBase::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();
	
	if (Controller != nullptr)
	{
		// 获取控制器的旋转
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
		
		// 计算前向和右向向量
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		
		// 添加移动输入
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

//视角旋转处理：根据输入值添加控制器的Yaw和Pitch旋转
void ALSCharacterBase::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();
	
	if (Controller != nullptr)
	{
		// 添加视角旋转输入
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

//切换一三人称视角
void ALSCharacterBase::HandleToggleCameraMode(const FInputActionValue& Value)
{
	if (CameraComponent)
	{
		CameraComponent->ToggleCameraMode();
	}
}

//开镜瞄准：摄像机拉进+移动组件减速
void ALSCharacterBase::StartADS(const FInputActionValue& Value)
{
	if (CameraComponent)
	{
		CameraComponent->EnterADS();
	}
	if (LSMovementComponent)
	{
		LSMovementComponent->SetAiming(true);
	}
}

//停止开镜瞄准：摄像机拉出+移动组件恢复速度
void ALSCharacterBase::StopADS(const FInputActionValue& Value)
{
	if (CameraComponent)
	{
		CameraComponent->ExitADS();
	}
	if (LSMovementComponent)
	{
		LSMovementComponent->SetAiming(false);
	}
}

//开始冲刺
void ALSCharacterBase::StartSprint(const FInputActionValue& Value)
{
	if (LSMovementComponent)
	{
		LSMovementComponent->StartSprint();
	}
}

//停止冲刺
void ALSCharacterBase::StopSprint(const FInputActionValue& Value)
{
	if (LSMovementComponent)
	{
		LSMovementComponent->StopSprint();
	}
}

//触发闪避与无敌帧
void ALSCharacterBase::HandleDash(const FInputActionValue& Value)
{
	if (LSMovementComponent)
	{
		LSMovementComponent->TryDash();
	}
}

//下蹲或滑铲：按下时尝试滑铲，若不满足条件则下蹲
void ALSCharacterBase::StartCrouchOrSlide(const FInputActionValue& Value)
{
	if (!LSMovementComponent) return;

	if (LSMovementComponent->IsSprinting())
	{
		LSMovementComponent->StartSlide();
	}
	else
	{
		Crouch();
	}
}

//结束滑铲/下蹲：松开时停止滑铲或下蹲
void ALSCharacterBase::StopCrouchOrSlide(const FInputActionValue& Value)
{
	if (LSMovementComponent && LSMovementComponent->IsSliding())
	{
		LSMovementComponent->StopSlide();
	}
	UnCrouch();
}