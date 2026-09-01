// Fill out your copyright notice in the Description page of Project Settings.


#include "LSCharacterBase.h"
#include "LSCameraComponent.h"
#include "LSMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "weapon/LSWeaponComponent.h"

// 构造函数：用自定义的ULSMovementComponent 替换默认的CharacterMovementComponent
ALSCharacterBase::ALSCharacterBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer.SetDefaultSubobjectClass<ULSMovementComponent>(ACharacter::CharacterMovementComponentName))
{
 	// 开启Tick
	PrimaryActorTick.bCanEverTick = true;
	
	//1,创建弹簧臂并插在角色眼部高度（0.0.65）
	USpringArmComponent* SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	SpringArm->SetupAttachment(GetCapsuleComponent());
	SpringArm->SetRelativeLocation(FVector(0.f, 0.f, 65.f)); //角色眼部高度
	SpringArm->TargetArmLength = 0.f; //默认第一人称，长度0
	SpringArm->bUsePawnControlRotation = true; //弹簧臂跟随控制器旋转
	SpringArm->bDoCollisionTest = false; //禁用弹簧臂碰撞检测，避免摄像机被遮挡
	
	//1. 创建摄像机组件并附加到根碰撞胶囊体
	CameraComponent = CreateDefaultSubobject<ULSCameraComponent>(TEXT("LSCameraComponent"));
	CameraComponent->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	CameraComponent->bUsePawnControlRotation = false; //摄像机交由弹簧臂控制旋转
	CameraComponent->SpringArm = SpringArm; //将弹簧臂引用传递给摄像机组件，以便在切换视角时调整位置和FOV
	
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
	
	WeaponComponent = CreateDefaultSubobject<ULSWeaponComponent>(TEXT("LSWeaponComp"));
}

// Called every frame
void ALSCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	//后坐力恢复现已由 WeaponBase 上的 LSRecoilComponent 自动处理
}