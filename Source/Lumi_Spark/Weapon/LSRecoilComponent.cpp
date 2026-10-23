#include "LSRecoilComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

ULSRecoilComponent::ULSRecoilComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	
	//默认提供一组经典突击步枪弹道模式（前几发强上抬，中段衰减，第8发垂直归零仅水平晃动）
	RecoilPattern = {
		FVector2D(0.00f,  0.80f), // 第1发：强垂直初动
		FVector2D(0.04f,  0.70f), // 第2发：稳定上抬
		FVector2D(-0.06f, 0.55f), // 第3发：开始向左微偏，上抬减缓
		FVector2D(0.08f,  0.40f), // 第4发：向右折返
		FVector2D(-0.10f, 0.25f), // 第5发：继续衰减
		FVector2D(0.12f,  0.15f), // 第6发：接近顶部
		FVector2D(-0.14f, 0.05f), // 第7发：垂直几乎见顶
		FVector2D(0.15f,  0.00f)  // 第8发：垂直彻底封顶，只在水平横向拉扯
	};
}

void ULSRecoilComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	//仅在停火且开启回正时执行平滑插值
	if (bEnableRecovery && !bIsFiring)
	{
		ProcessRecovery(DeltaTime);
	}
	
	//回正完毕后关闭Tick节省性能
	if (!bIsFiring && AccumulatedRecoil.IsNearlyZero(0.01f))
	{
		SetComponentTickEnabled(false);
	}
}

void ULSRecoilComponent::StartRecoil()
{
	bIsFiring = true;
	//开火期间不需要Tick回正，但提前启用避免StopRecoil时遗漏
	SetComponentTickEnabled(true);
}

void ULSRecoilComponent::StopRecoil()
{
	bIsFiring = false;
	CurrentPatternIndex = 0; //重置弹道模式索引
	
	//停火后启用Tick，让ProcessRecovery可以平滑回正
	if (bEnableRecovery && !AccumulatedRecoil.IsNearlyZero(0.01f))
	{
		SetComponentTickEnabled(true);
	}
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
	
	// 1. 获取当前发数的基础弹道偏移：
	FVector2D ShotOffset;
	if (CurrentPatternIndex < RecoilPattern.Num())
	{
		ShotOffset = RecoilPattern[CurrentPatternIndex];
	}
	else
	{
		// 超过预设弹道（全自动连射中后段）：垂直上抬彻底归零，仅保留水平横向拉扯
		const float HorizontalJitter = FMath::FRandRange(-0.16f, 0.16f);
		ShotOffset = FVector2D(HorizontalJitter, 0.0f);
	}
	
	// 2. 叠加随机散布扰动（保留可记忆弹道的同时避免机械宏）
	const float RandomYaw = FMath::FRandRange(-RandomSpreadFactor, RandomSpreadFactor);
	// 垂直已近封顶时，不再产生向上的正随机扰动，避免突破天花板
	const float RandomPitch = (ShotOffset.Y > 0.0f)
		? FMath::FRandRange(-RandomSpreadFactor * 0.3f, RandomSpreadFactor * 0.3f)
		: FMath::FRandRange(-RandomSpreadFactor * 0.2f, 0.0f);
	ShotOffset += FVector2D(RandomYaw, RandomPitch);
	
	// 3. 计算最终倍率（开镜时降低后坐力）
	float FinalMultiplier = RecoilMultiplier;
	if (bIsADS)
	{
		FinalMultiplier *= ADSRecoilMultiplier;
	}
	ShotOffset *= FinalMultiplier;

	// 4. 垂直后坐力天花板强制硬封顶（防御蓝图/热重载零值陷阱）：
	const float EffectiveMaxPitch = (MaxVerticalPitch > 0.5f) ? MaxVerticalPitch : 8.0f;
	if (AccumulatedRecoil.Y >= EffectiveMaxPitch)
	{
		ShotOffset.Y = 0.0f; // 垂直彻底锁死，不再继续往上推
	}
	else if (AccumulatedRecoil.Y + ShotOffset.Y > EffectiveMaxPitch)
	{
		ShotOffset.Y = FMath::Max(0.0f, EffectiveMaxPitch - AccumulatedRecoil.Y); // 削减超限部分
	}
	
	// 5. 将后坐力施加到玩家控制器（Pitch 向上抬，Yaw 左右偏）
	ApplyInputToController(-ShotOffset.Y, ShotOffset.X);
	
	// 6. 累加到回正池中并推进连射发数
	AccumulatedRecoil += ShotOffset;
	CurrentPatternIndex++;

	GEngine->AddOnScreenDebugMessage(9999, 0.4f, FColor::Orange, 
		FString::Printf(TEXT("💥 [后坐力] 第%d发 | 垂直抬升=%.2f, 水平偏转=%.2f | 累计=%.2f/%.2f"), 
			CurrentPatternIndex, ShotOffset.Y, ShotOffset.X, AccumulatedRecoil.Y, EffectiveMaxPitch));
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
	
	// 1. 兼容挂载在Pawn自身、Weapon Actor、或通过组件Attach的层级关系
	APawn* OwnerPawn = Cast<APawn>(OwnerActor);
	if (!OwnerPawn)
	{
		OwnerPawn = Cast<APawn>(OwnerActor->GetOwner());
	}
	if (!OwnerPawn)
	{
		OwnerPawn = OwnerActor->GetInstigator<APawn>();
	}
	if (!OwnerPawn)
	{
		OwnerPawn = Cast<APawn>(OwnerActor->GetAttachParentActor());
	}
	
	if (OwnerPawn && OwnerPawn->GetController())
	{
		return Cast<APlayerController>(OwnerPawn->GetController());
	}

	// 2. 本地后坐力终极兜底（直接抓取本地首位 PlayerController）
	if (UWorld* World = GetWorld())
	{
		return World->GetFirstPlayerController();
	}

	return nullptr;
}

void ULSRecoilComponent::ApplyInputToController(float PitchDelta, float YawDelta)
{
	if (APlayerController* PC = GetPlayerController())
	{
		PC->AddPitchInput(PitchDelta);
		PC->AddYawInput(YawDelta);
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("❌ [后坐力] 获取 PlayerController 失败！"));
	}
}
