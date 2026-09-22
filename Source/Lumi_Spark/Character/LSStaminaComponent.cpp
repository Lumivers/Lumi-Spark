#include "Character/LSStaminaComponent.h"

ULSStaminaComponent::ULSStaminaComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false; // 初始满体力，关闭 Tick 节省性能
}

void ULSStaminaComponent::BeginPlay()
{
	Super::BeginPlay();
	CurrentStamina = MaxStamina;
	TimeSinceLastConsume = RecoveryDelay;
}

void ULSStaminaComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	TimeSinceLastConsume += DeltaTime;

	if (TimeSinceLastConsume >= RecoveryDelay && CurrentStamina < MaxStamina)
	{
		CurrentStamina = FMath::Min(CurrentStamina + RecoveryRate * DeltaTime, MaxStamina);
		OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);

		if (CurrentStamina >= DashCost)
		{
			bIsExhausted = false;
		}

		// 回满后关闭 Tick 零空跑
		if (FMath::IsNearlyEqual(CurrentStamina, MaxStamina))
		{
			SetComponentTickEnabled(false);
		}
	}
}

bool ULSStaminaComponent::ConsumeStamina(float Amount)
{
	if (Amount <= 0.0f)
	{
		return true;
	}

	if (CurrentStamina < Amount)
	{
		if (!bIsExhausted)
		{
			bIsExhausted = true;
			OnStaminaExhausted.Broadcast();
		}
		return false;
	}

	CurrentStamina = FMath::Max(0.0f, CurrentStamina - Amount);
	TimeSinceLastConsume = 0.0f;

	// 激活 Tick 以便后续平滑回体
	SetComponentTickEnabled(true);
	OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);

	if (FMath::IsNearlyZero(CurrentStamina))
	{
		bIsExhausted = true;
		OnStaminaExhausted.Broadcast();
	}

	return true;
}

void ULSStaminaComponent::RestoreStamina(float Amount)
{
	if (Amount <= 0.0f) return;

	CurrentStamina = FMath::Min(CurrentStamina + Amount, MaxStamina);
	OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);

	if (CurrentStamina >= DashCost)
	{
		bIsExhausted = false;
	}
}

void ULSStaminaComponent::InitializeStamina(float InMaxStamina)
{
	MaxStamina = FMath::Max(10.0f, InMaxStamina);
	CurrentStamina = MaxStamina;
	bIsExhausted = false;
	TimeSinceLastConsume = RecoveryDelay;
	SetComponentTickEnabled(false);

	OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
}