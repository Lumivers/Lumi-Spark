#include "Character/LSEnergyComponent.h"
#include "Character/LSCharacterBase.h"
#include "Net/UnrealNetwork.h"

ULSEnergyComponent::ULSEnergyComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	SetIsReplicatedByDefault(true);
}

void ULSEnergyComponent::BeginPlay()
{
	Super::BeginPlay();
	CurrentEnergy = 0.0f;
}

void ULSEnergyComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ULSEnergyComponent, CurrentEnergy);
	DOREPLIFETIME(ULSEnergyComponent, MaxEnergy);
}

void ULSEnergyComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 仅权威端执行被动充能
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (bEnablePassiveRecharge && PassiveRechargeRate > 0.0f && CurrentEnergy < MaxEnergy)
	{
		const float Gained = PassiveRechargeRate * DeltaTime * EnergyRechargeRate;
		CurrentEnergy = FMath::Min(CurrentEnergy + Gained, MaxEnergy);
		OnEnergyChanged.Broadcast(CurrentEnergy, MaxEnergy);

		if (FMath::IsNearlyEqual(CurrentEnergy, MaxEnergy))
		{
			OnEnergyFull.Broadcast();
		}
	}
}

void ULSEnergyComponent::AddEnergy(float Amount)
{
	if (Amount <= 0.0f || CurrentEnergy >= MaxEnergy)
	{
		return;
	}

	const float EffectiveAmount = Amount * EnergyRechargeRate;
	CurrentEnergy = FMath::Min(CurrentEnergy + EffectiveAmount, MaxEnergy);
	OnEnergyChanged.Broadcast(CurrentEnergy, MaxEnergy);

	if (FMath::IsNearlyEqual(CurrentEnergy, MaxEnergy))
	{
		OnEnergyFull.Broadcast();
	}
}

bool ULSEnergyComponent::ConsumeEnergy(float Amount)
{
	if (Amount <= 0.0f)
	{
		return true;
	}

	if (CurrentEnergy < Amount)
	{
		return false;
	}

	CurrentEnergy = FMath::Max(0.0f, CurrentEnergy - Amount);
	OnEnergyChanged.Broadcast(CurrentEnergy, MaxEnergy);
	return true;
}

void ULSEnergyComponent::CollectParticle(FGameplayTag ParticleElement, float BaseEnergy)
{
	if (BaseEnergy <= 0.0f)
	{
		return;
	}

	float ElementMultiplier = 1.0f;

	// 获取宿主角色的自身元素属性
	if (ALSCharacterBase* OwnerChar = Cast<ALSCharacterBase>(GetOwner()))
	{
		const FGameplayTag CharacterElement = OwnerChar->GetCharacterElementTag();
		if (CharacterElement.IsValid() && ParticleElement.IsValid())
		{
			if (CharacterElement.MatchesTagExact(ParticleElement))
			{
				ElementMultiplier = 3.0f; // 同属性微粒 3.0x
			}
			else
			{
				ElementMultiplier = 1.0f; // 异属性微粒 1.0x
			}
		}
		else
		{
			ElementMultiplier = 2.0f;     // 无属性白球 2.0x
		}
	}

	AddEnergy(BaseEnergy * ElementMultiplier);
}

void ULSEnergyComponent::InitializeEnergy(float InMaxEnergy, float InRechargeRate)
{
	MaxEnergy = FMath::Max(10.0f, InMaxEnergy);
	EnergyRechargeRate = FMath::Max(0.1f, InRechargeRate);
	CurrentEnergy = 0.0f;

	OnEnergyChanged.Broadcast(CurrentEnergy, MaxEnergy);
}

void ULSEnergyComponent::OnRep_CurrentEnergy()
{
	OnEnergyChanged.Broadcast(CurrentEnergy, MaxEnergy);

	if (CurrentEnergy >= MaxEnergy)
	{
		OnEnergyFull.Broadcast();
	}
}