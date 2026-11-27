#include "LSRecoilComponent.h"
#include "Core/LSDebugHelpers.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

ULSRecoilComponent::ULSRecoilComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	
	MaxVerticalPitch = 9.0f;
	RecoverySpeed = 10.5f;

	// 默认弹道序列（前两发温和稳重方便速点，中后段扫射逐渐放大上抬与左右横摆）
	RecoilPattern = {
		FVector2D(0.00f,  0.38f), // 第1发：精准点射黄金窗，轻微上抬、无水平甩动
		FVector2D(0.02f,  0.46f), // 第2发：二连短点依然紧凑可控
		FVector2D(-0.08f, 0.56f), // 第3发：开始进入扫射爬升阶段
		FVector2D(0.14f,  0.64f), // 第4发：垂直加速上抬并伴随明显横向拉扯
		FVector2D(-0.24f, 0.66f), // 第5发：大幅左摆
		FVector2D(0.32f,  0.62f), // 第6发：大幅右摆
		FVector2D(-0.40f, 0.54f), // 第7发：强横向摆动持续放大
		FVector2D(0.46f,  0.46f)  // 第8发：未松手将进入高频不稳定扫射区
	};
}

void ULSRecoilComponent::ConfigureRecoilDefaults(const TArray<FVector2D>& InPattern, float InMultiplier, float InMaxVerticalPitch, float InADSRecoilMultiplier, float InRecoverySpeed, bool bInEnableRecovery, float InRandomSpreadFactor)
{
	if (!InPattern.IsEmpty())
	{
		RecoilPattern = InPattern;
	}
	RecoilMultiplier = InMultiplier;
	MaxVerticalPitch = InMaxVerticalPitch;
	ADSRecoilMultiplier = InADSRecoilMultiplier;
	RecoverySpeed = InRecoverySpeed;
	bEnableRecovery = bInEnableRecovery;
	RandomSpreadFactor = InRandomSpreadFactor;
}

void ULSRecoilComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 1. 将开火产生的待施加后坐力按帧平滑推给控制器，避免单帧硬跳打断甩枪轨迹
	if (bUseSmoothRecoilApply && !PendingRecoilOffset.IsNearlyZero(0.001f))
	{
		const float Alpha = FMath::Clamp(RecoilApplyInterpSpeed * DeltaTime, 0.0f, 1.0f);
		const FVector2D ApplyStep = PendingRecoilOffset.IsNearlyZero(0.005f) ? PendingRecoilOffset : (PendingRecoilOffset * Alpha);
		ApplyInputToController(-ApplyStep.Y, ApplyStep.X);
		PendingRecoilOffset -= ApplyStep;
	}
	else if (!bUseSmoothRecoilApply && !PendingRecoilOffset.IsNearlyZero(0.001f))
	{
		ApplyInputToController(-PendingRecoilOffset.Y, PendingRecoilOffset.X);
		PendingRecoilOffset = FVector2D::ZeroVector;
	}
	
	// 2. 仅在停火且上抬缓冲推完、并开启回正时执行平滑回正（避免上抬与下拉同帧抵消）
	if (bEnableRecovery && !bIsFiring && PendingRecoilOffset.IsNearlyZero(0.001f))
	{
		ProcessRecovery(DeltaTime);
	}
	
	// 3. 缓冲与回正都归零后关闭 Tick 节省性能
	const bool bRecoveryFinished = (!bEnableRecovery || AccumulatedRecoil.IsNearlyZero(0.01f));
	if (!bIsFiring && PendingRecoilOffset.IsNearlyZero(0.001f) && bRecoveryFinished)
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
	
	// 若未启用停火自动回正，停火时必须清空累计池，否则下次开火会直接撞上 MaxVerticalPitch 锁死垂直后坐力
	if (!bEnableRecovery)
	{
		CurrentPatternIndex = 0;
		AccumulatedRecoil = FVector2D::ZeroVector;
		return;
	}

	// 仅在已明显停顿或后坐基本回正时才重置序列索引，防止极限微松左键白嫖第 1 发低后坐
	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : -1.0;
	if (AccumulatedRecoil.IsNearlyZero(0.15f) ||
		(LastRecoilShotTime >= 0.0 && Now >= 0.0 && (Now - LastRecoilShotTime) >= static_cast<double>(PatternResetDelay)))
	{
		CurrentPatternIndex = 0;
	}

	//停火后启用Tick，让ProcessRecovery可以平滑回正
	if (!AccumulatedRecoil.IsNearlyZero(0.01f))
	{
		SetComponentTickEnabled(true);
	}
}

void ULSRecoilComponent::ResetRecoil()
{
	bIsFiring = false;
	CurrentPatternIndex = 0;
	LastRecoilShotTime = -1.0;
	AccumulatedRecoil = FVector2D::ZeroVector;
	PendingRecoilOffset = FVector2D::ZeroVector;
}

void ULSRecoilComponent::ApplyRecoil(bool bIsADS)
{
	if (RecoilPattern.IsEmpty()) return;

	// 若与上一枪间隔足够长（节奏速点），重置弹道序列回到稳重的第一发
	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : -1.0;
	if (LastRecoilShotTime >= 0.0 && Now >= 0.0 && (Now - LastRecoilShotTime) >= static_cast<double>(PatternResetDelay))
	{
		CurrentPatternIndex = 0;
	}
	
	// 1. 获取当前发数的基础弹道偏移：
	FVector2D ShotOffset;
	if (CurrentPatternIndex < RecoilPattern.Num())
	{
		ShotOffset = RecoilPattern[CurrentPatternIndex];
	}
	else
	{
		// 超过预设弹道（持续全自动扫射中后段）：保留持续上抬压力并交替放大水平横向拉扯
		const float DirSign = (CurrentPatternIndex % 2 == 0) ? 1.0f : -1.0f;
		const float HorizontalPull = DirSign * FMath::FRandRange(SprayPatternHorizontalJitter * 0.55f, SprayPatternHorizontalJitter * 1.15f);
		ShotOffset = FVector2D(HorizontalPull, SprayPatternVerticalClimb);
	}
	
	// 2. 叠加渐进式随机散布扰动：前几发点射极稳，连射发数越多随机晃动越大
	float ActiveRandomFactor = RandomSpreadFactor;
	if (CurrentPatternIndex < FirstShotPrecisionCount)
	{
		ActiveRandomFactor *= EarlyShotRandomScale;
	}
	else
	{
		const int32 SprayShots = CurrentPatternIndex - FirstShotPrecisionCount;
		ActiveRandomFactor *= FMath::Min(2.8f, 1.0f + static_cast<float>(SprayShots) * SprayRandomBoostPerShot);
	}

	const float RandomYaw = FMath::FRandRange(-ActiveRandomFactor, ActiveRandomFactor);
	// 垂直已近封顶时，不再产生向上的正随机扰动，避免突破天花板
	const float RandomPitch = (ShotOffset.Y > 0.0f)
		? FMath::FRandRange(-ActiveRandomFactor * 0.25f, ActiveRandomFactor * 0.25f)
		: FMath::FRandRange(-ActiveRandomFactor * 0.2f, 0.0f);
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
	
	// 5. 将后坐力施加到玩家控制器（平滑缓冲模式在 Tick 逐帧推入，防止单帧硬跳打断甩枪）
	if (bUseSmoothRecoilApply)
	{
		PendingRecoilOffset += ShotOffset;
		if (!PendingRecoilOffset.IsNearlyZero(0.001f))
		{
			SetComponentTickEnabled(true);
		}
	}
	else
	{
		ApplyInputToController(-ShotOffset.Y, ShotOffset.X);
	}
	
	// 6. 累加到回正池中，记录时间戳并推进连射发数
	AccumulatedRecoil += ShotOffset;
	LastRecoilShotTime = Now;
	CurrentPatternIndex++;

	LS_SCREEN_DEBUG(9999, 0.4f, FColor::Orange, 
		FString::Printf(TEXT("💥 [后坐力] 第%d发 | 垂直抬升=%.2f, 水平偏转=%.2f | 累计=%.2f/%.2f"), 
			CurrentPatternIndex, ShotOffset.Y, ShotOffset.X, AccumulatedRecoil.Y, EffectiveMaxPitch));
}

void ULSRecoilComponent::ProcessRecovery(float DeltaTime)
{
	//若剩余回正量已接近0，直接清零并同步复位连射计数
	if (AccumulatedRecoil.IsNearlyZero(0.01f))
	{
		AccumulatedRecoil = FVector2D::ZeroVector;
		CurrentPatternIndex = 0;
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
		LS_SCREEN_DEBUG(-1, 2.0f, FColor::Red, TEXT("❌ [后坐力] 获取 PlayerController 失败！"));
	}
}
