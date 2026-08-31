// Fill out your copyright notice in the Description page of Project Settings.


#include "LSCameraComponent.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"

ULSCameraComponent::ULSCameraComponent()
{
	//开启Tick支持逐帧平滑插值
	PrimaryComponentTick.bCanEverTick = true;
	
	//默认启用Pawn控制旋转
	bUsePawnControlRotation = true;
	
	//初始化目标值
	TargetOffset = FPSOffset;
	TargetFov = FPSFov;
	FieldOfView	= FPSFov;
	SetRelativeLocation(FPSOffset);
}

void ULSCameraComponent::BeginPlay()
{
	Super::BeginPlay();
	
	PrimaryComponentTick.SetTickFunctionEnable(true);
	//游戏开始的时候应用初始模式与模型显隐
	SetCameraMode(CurrentMode);
}

void ULSCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	//逐帧更新位置与FOV平滑过渡
	UpdateCameraInterpolation(DeltaTime);
}

void ULSCameraComponent::SetCameraMode(ELSCameraMode NewMode)
{
	CurrentMode = NewMode;
	
	switch (CurrentMode)
	{
		case ELSCameraMode::FirstPerson:
			TargetOffset = FPSOffset;
			TargetFov = FPSFov;
			break;
		
		case ELSCameraMode::ThirdPerson:
			//第三人称：摄像机在角色后方TPSArmLength距离处，高度与眼部持平
			TargetOffset = FVector(-TPSArmLength, 0.f, FPSOffset.Z); // 第三人称相机位置由弹簧臂控制
			TargetFov = TPSFov;
			break;
		
		case ELSCameraMode::OverShoulder:
			TargetOffset = ShoulderOffset;
			TargetFov = ShoulderFov;
			break;
		
		default:
			break;
	}
	
	SetRelativeLocation(TargetOffset);
	SetFieldOfView(TargetFov);
	
	UpdateMeshVisibility();
}

void ULSCameraComponent::ToggleCameraMode()
{
	//在第三人称和第一人称之间切换
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
	bIsInADS = true;
	SetCameraMode(ELSCameraMode::OverShoulder);
}

void ULSCameraComponent::ExitADS()
{
	bIsInADS = false;
	SetCameraMode(ELSCameraMode::FirstPerson);
}

void ULSCameraComponent::UpdateCameraInterpolation(float DeltaTime)
{
	//1. 平滑插值位置
	const FVector NewLocation = FMath::VInterpTo(GetRelativeLocation(), TargetOffset, DeltaTime, TransitionSpeed);
	SetRelativeLocation(NewLocation);
	
	//2. 视场角FOV平滑插值
	const float NewFov = FMath::FInterpTo(FieldOfView, TargetFov, DeltaTime, TransitionSpeed);
	SetFieldOfView(NewFov);
	
	/*
	//第三人称防穿墙检测(SweepSphere检测碰撞)
	if (CurrentMode == ELSCameraMode::ThirdPerson && GetOwner())
	{
		FHitResult HitResult;
		const FVector Start = GetOwner()->GetActorLocation() + FVector(0.f, 0.f, FPSOffset.Z); //角色眼部高度
		const FVector End = GetComponentLocation();
		
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(GetOwner());
		
		if (GetWorld()->SweepSingleByChannel(HitResult, Start, End, FQuat::Identity, ECC_Camera, FCollisionShape::MakeSphere(10.f), QueryParams))
		{
			//如果检测到碰撞，将摄像机位置调整到碰撞点前方
			SetWorldLocation(HitResult.Location + HitResult.ImpactNormal * 5.f);
		}
	}
	*/
}

void ULSCameraComponent::UpdateMeshVisibility()
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter || !OwnerCharacter->GetMesh()) return;
	
	USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
	
	if (CurrentMode == ELSCameraMode::FirstPerson)
	{
		//第一人称：隐藏全身Mesh，显示第一人称手臂Mesh
		Mesh->SetOwnerNoSee(true);
		Mesh->bCastHiddenShadow = true; //隐藏时仍投射阴影
	}
	else
	{
		//第三人称或过肩：显示全身Mesh，隐藏第一人称手臂Mesh
		Mesh->SetOwnerNoSee(false);
	}
}
