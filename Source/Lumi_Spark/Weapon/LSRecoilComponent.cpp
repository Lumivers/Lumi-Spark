#include "LSRecoilComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

ULSRecoilComponent::ULSRecoilComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	
	//默认提供一组经典突击步枪弹道模式
	RecoilPattern = {
		FVector2D(0.0f,   0.8f),
		FVector2D(0.05f,  0.75f),
		FVector2D(-0.06f, 0.7f),
		FVector2D(0.10f,  0.65f),
		FVector2D(0.15f,  0.6f),
		FVector2D(-0.12f, 0.55f),
		FVector2D(0.20f,  0.5f)
	};
}

void ULSRecoilComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	//仅在停火且开启回正时执行平滑插值
	if (bEnableRecovery & !bIsFiring)
	{
		ProcessRecovery(DeltaTime);
	}
}

void ULSRecoilComponent::StartRecoil()
{
	bIsFiring = true;
}

void ULSRecoilComponent::StopRecoil()
{
	bIsFiring = false;
	CurrentPatternIndex = 0; //重置弹道模式索引
}

void ULSRecoilComponent::ResetRecoil()
{
	bIsFiring = false;
	CurrentPatternIndex = 0;
	AccumulatedRecoil = FVector2D::ZeroVector;
}

void ULSRecoilComponent::ApplyRecoil(bool bIsADS)
{
	if (RecoilPattern.IsEmpty()) return;
	
	//1，获取当前发数的基础弹道偏移（超出数组长度后循环最后一发）
	const int32 SafeIndex = FMath::Clamp(CurrentPatternIndex, 0, RecoilPattern.Num() - 1);
	FVector2D ShotOffset = RecoilPattern[SafeIndex];
	
	//2，叠加随机散步扰动（保留可记忆弹道的同时避免机械宏
	const float RandomYaw = FMath::FRandRange(-RandomSpreadFactor, RandomSpreadFactor);
	const float RandomPitch = FMath::FRandRange(-RandomSpreadFactor * 0.5f, RandomSpreadFactor * 0.5f); //垂直扰动较小
	ShotOffset += FVector2D(RandomYaw, RandomPitch);
	
	//3，计算最终倍率（开镜时降低后坐力）
	float FinalMultiplier = RecoilMultiplier;
	if (bIsADS)
	{
		FinalMultiplier *= ADSRecoilMultiplier;
	}
	ShotOffset *= FinalMultiplier;
	
	//4, 将后坐力施加到玩家控制器
	ApplyInputToController(-ShotOffset.Y, ShotOffset.X); //Pitch向上抬，Yaw左右偏
	
	//5, 累加到回正池中并推进连射序列
	AccumulatedRecoil += ShotOffset;
	CurrentPatternIndex++;
}

void ULSRecoilComponent::ProcessRecovery(float DeltaTime)
{
	//若剩余回正量已接近0，直接清零并返回
	if (AccumulatedRecoil.IsNearlyZero(0.01f))
	{
		AccumulatedRecoil = FVector2D::ZeroVector;
		return;
	}
	
	//沿剩余量平滑插值回正步长
	const FVector2D RecoveryStep = AccumulatedRecoil * FMath::Clamp(RecoverySpeed * DeltaTime, 0.f, 1.f);
	
	//反向补偿输入给controller
	ApplyInputToController(RecoveryStep.Y, -RecoveryStep.X);
	
	AccumulatedRecoil -= RecoveryStep;
}

APlayerController* ULSRecoilComponent::GetPlayerController() const
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor) return nullptr;
	
	//兼容挂载在Pawn自身或挂在Weapon Actor上的层级关系
	APawn* OwnerPawn = Cast<APawn>(OwnerActor);
	if (!OwnerPawn)
	{
		OwnerPawn = Cast<APawn>(OwnerActor->GetOwner());
	}
	if (!OwnerPawn)
	{
		OwnerPawn = OwnerActor->GetInstigator<APawn>();
	}
	
	return OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
}

void ULSRecoilComponent::ApplyInputToController(float PitchDelta, float YawDelta)
{
	if (APlayerController* PC = GetPlayerController())
	{
		PC->AddPitchInput(PitchDelta);
		PC->AddYawInput(YawDelta);
	}
}
