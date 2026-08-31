// Fill out your copyright notice in the Description page of Project Settings.


#include "LSCharacterBase.h"
#include "LSCameraComponent.h"
#include "LSMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"

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

// Called every frame
void ALSCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	//处理后坐力恢复：每帧减少累积的俯仰后坐力，直到归零
	if (AccumulatedPitchRecoil > 0.01f)
	{
		const float RecoveryStep = AccumulatedPitchRecoil * FMath::Clamp(DeltaTime * 10.0f, 0.0f, 1.0f); //每秒恢复10倍的后坐力
		AddControllerPitchInput(RecoveryStep);
		AccumulatedPitchRecoil -= RecoveryStep;
	}
}

void ALSCharacterBase::StartFire()
{
	FVector EyeLoc;
	FRotator EyeRot;
	GetActorEyesViewPoint(EyeLoc, EyeRot); //获取角色眼睛位置和朝向
	
	FVector TraceEnd = EyeLoc + (EyeRot.Vector() * 5000.f); //发射终点
	FHitResult Hit;
	GetWorld()->LineTraceSingleByChannel(Hit, EyeLoc, TraceEnd, ECC_Visibility); //射线检测
	
	//4，绘制调试射线
	FVector EndPoint = Hit.bBlockingHit ? Hit.ImpactPoint : TraceEnd;
	DrawDebugLine(GetWorld(), EyeLoc, EndPoint, FColor::Red, false, 1.0f, 0, 2.0f);
	if (Hit.bBlockingHit)
	{
		DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 10.0f, 12, FColor::Green, false, 1.0f);
	}
	
	//5,开火瞬间需要有后坐力
	AddControllerPitchInput(-0.8f);
	AddControllerYawInput(FMath::FRandRange(-0.2f, 0.2f));
	AddControllerPitchInput(-0.8f);
	AccumulatedPitchRecoil += 0.8f; //累积俯仰后坐力
}