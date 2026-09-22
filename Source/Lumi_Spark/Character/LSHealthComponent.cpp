#include "Character/LSHealthComponent.h"
#include "Combat/LSShieldComponent.h"
#include "Net/UnrealNetwork.h"

ULSHealthComponent::ULSHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	SetIsReplicatedByDefault(true);
}

void ULSHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	CurrentHealth = MaxHealth;
	TimeSinceLastDamage = RegenDelay;
}

void ULSHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ULSHealthComponent, CurrentHealth);
	DOREPLIFETIME(ULSHealthComponent, MaxHealth);
	DOREPLIFETIME(ULSHealthComponent, bIsDead);
}

void ULSHealthComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!GetOwner() || !GetOwner()->HasAuthority() || bIsDead)
	{
		return;
	}

	TimeSinceLastDamage += DeltaTime;

	if (RegenRate > 0.0f && TimeSinceLastDamage >= RegenDelay && CurrentHealth < MaxHealth)
	{
		const float Restored = FMath::Min(RegenRate * DeltaTime, MaxHealth - CurrentHealth);
		if (Restored > 0.0f)
		{
			CurrentHealth += Restored;
			OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, false);

			if (bLowHealthWarningTriggered && (CurrentHealth / MaxHealth) >= LowHealthThreshold)
			{
				bLowHealthWarningTriggered = false;
			}
		}
	}
}

float ULSHealthComponent::TakeDamage(float DamageAmount, FGameplayTag DamageElement, AActor* DamageCauser, AController* InstigatorController)
{
	if (bIsDead || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}

	float RemainingDamage = DamageAmount;

	// 1. 优先尝试由元素护盾吸收（3 个实参传入）
	if (AActor* OwnerActor = GetOwner())
	{
		if (ULSShieldComponent* ShieldComp = OwnerActor->FindComponentByClass<ULSShieldComponent>())
		{
			RemainingDamage = ShieldComp->AbsorbDamage(RemainingDamage, DamageElement, DamageCauser);
		}
	}

	// 2. 剩余穿透伤害扣减生命
	if (RemainingDamage > 0.0f)
	{
		CurrentHealth = FMath::Clamp(CurrentHealth - RemainingDamage, 0.0f, MaxHealth);
		TimeSinceLastDamage = 0.0f;

		OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, true);

		// 低血量 20% 告警
		if (!bLowHealthWarningTriggered && MaxHealth > 0.0f && (CurrentHealth / MaxHealth) <= LowHealthThreshold)
		{
			bLowHealthWarningTriggered = true;
			OnLowHealth.Broadcast(CurrentHealth, MaxHealth);
		}

		// 濒死判定
		if (CurrentHealth <= 0.0f)
		{
			bIsDead = true;
			OnDeath.Broadcast();
		}
	}

	return RemainingDamage;
}

float ULSHealthComponent::Heal(float HealAmount)
{
	if (bIsDead || HealAmount <= 0.0f || CurrentHealth >= MaxHealth)
	{
		return 0.0f;
	}

	const float OldHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(CurrentHealth + HealAmount, 0.0f, MaxHealth);
	const float ActualHealed = CurrentHealth - OldHealth;

	if (ActualHealed > 0.0f)
	{
		OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, false);

		if (bLowHealthWarningTriggered && (CurrentHealth / MaxHealth) >= LowHealthThreshold)
		{
			bLowHealthWarningTriggered = false;
		}
	}

	return ActualHealed;
}

void ULSHealthComponent::InitializeHealth(float InMaxHealth, float InCurrentHealth)
{
	MaxHealth = FMath::Max(1.0f, InMaxHealth);
	CurrentHealth = (InCurrentHealth >= 0.0f) ? FMath::Clamp(InCurrentHealth, 0.0f, MaxHealth) : MaxHealth;
	bIsDead = (CurrentHealth <= 0.0f);
	bLowHealthWarningTriggered = (CurrentHealth / MaxHealth) <= LowHealthThreshold;
	TimeSinceLastDamage = RegenDelay;

	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, false);
}

void ULSHealthComponent::OnRep_CurrentHealth()
{
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, false);

	if (!bLowHealthWarningTriggered && MaxHealth > 0.0f && (CurrentHealth / MaxHealth) <= LowHealthThreshold)
	{
		bLowHealthWarningTriggered = true;
		OnLowHealth.Broadcast(CurrentHealth, MaxHealth);
	}
	else if (bLowHealthWarningTriggered && MaxHealth > 0.0f && (CurrentHealth / MaxHealth) > LowHealthThreshold)
	{
		bLowHealthWarningTriggered = false;
	}
}

void ULSHealthComponent::OnRep_IsDead()
{
	if (bIsDead)
	{
		OnDeath.Broadcast();
	}
}