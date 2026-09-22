#include "LSSkillComponent.h"
#include "Character/LSCharacterBase.h"
#include "Character/LSEnergyComponent.h"
#include "Core/LSEventBus.h"

ULSSkillComponent::ULSSkillComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

ULSEnergyComponent* ULSSkillComponent::GetOwnerEnergyComponent() const
{
	if (ALSCharacterBase* OwnerChar = Cast<ALSCharacterBase>(GetOwner()))
	{
		return OwnerChar->GetEnergyComponent();
	}
	return nullptr;
}

void ULSSkillComponent::BeginPlay()
{
	Super::BeginPlay();

	// 监听总线直伤与反应事件，将充能点数注入 EnergyComponent
	if (ULSEventBus* EventBus = ULSEventBus::Get(this))
	{
		EventBus->OnDamageDealt.AddDynamic(this, &ULSSkillComponent::HandleDamageDealt);
		EventBus->OnElementReactionTriggered.AddDynamic(this, &ULSSkillComponent::HandleReactionTriggered);
	}
}

void ULSSkillComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// E 战技独立冷却流逝（前台后台均独立计算）
	if (SkillCooldownTimer > 0.0f)
	{
		SkillCooldownTimer = FMath::Max(0.0f, SkillCooldownTimer - DeltaTime);
		OnSkillCooldownChanged.Broadcast(SkillCooldownTimer, MaxSkillCooldown);
	}
}

bool ULSSkillComponent::CanCastSkill() const
{
	return SkillCooldownTimer <= 0.0f;
}

bool ULSSkillComponent::CanCastBurst() const
{
	if (ULSEnergyComponent* EnergyComp = GetOwnerEnergyComponent())
	{
		return EnergyComp->GetCurrentEnergy() >= BurstRequiredEnergy;
	}
	return false;
}

bool ULSSkillComponent::CastSkill()
{
	if (!CanCastSkill()) return false;

	SkillCooldownTimer = MaxSkillCooldown;
	OnSkillCooldownChanged.Broadcast(SkillCooldownTimer, MaxSkillCooldown);

	ExecuteSkillLogic();
	OnSkillCast.Broadcast(SkillTag, GetOwner());
	return true;
}

bool ULSSkillComponent::CastBurst()
{
	if (!CanCastBurst()) return false;

	// 直接从能量组件扣除所需能量
	if (ULSEnergyComponent* EnergyComp = GetOwnerEnergyComponent())
	{
		if (!EnergyComp->ConsumeEnergy(BurstRequiredEnergy))
		{
			return false;
		}
	}

	ExecuteBurstLogic();
	OnBurstCast.Broadcast(BurstTag, GetOwner());
	return true;
}

void ULSSkillComponent::ResetSkillCooldown()
{
	SkillCooldownTimer = 0.0f;
	OnSkillCooldownChanged.Broadcast(0.0f, MaxSkillCooldown);
}

float ULSSkillComponent::GetSkillCooldownRatio() const
{
	return (MaxSkillCooldown > 0.0f) ? (SkillCooldownTimer / MaxSkillCooldown) : 0.0f;
}

float ULSSkillComponent::GetCurrentEnergy() const
{
	return GetOwnerEnergyComponent() ? GetOwnerEnergyComponent()->GetCurrentEnergy() : 0.0f;
}

float ULSSkillComponent::GetMaxEnergy() const
{
	return GetOwnerEnergyComponent() ? GetOwnerEnergyComponent()->GetMaxEnergy() : BurstRequiredEnergy;
}

void ULSSkillComponent::ExecuteSkillLogic_Implementation()
{
	// 默认空实现，由蓝图或具体角色子类重写特效与动作
}

void ULSSkillComponent::ExecuteBurstLogic_Implementation()
{
	// 默认空实现
}

void ULSSkillComponent::HandleDamageDealt(const FLSDamageContext& DamageContext)
{
	if (DamageContext.DamageCauser != GetOwner()) return;

	if (ULSEnergyComponent* EnergyComp = GetOwnerEnergyComponent())
	{
		EnergyComp->AddEnergy(DamageContext.FinalDamage * EnergyDamageConversionRate);
	}
}

void ULSSkillComponent::HandleReactionTriggered(AActor* Target, FGameplayTag InReactionTag, float ReactionDamage, AActor* Instigator)
{
	if (Instigator != GetOwner()) return;

	if (ULSEnergyComponent* EnergyComp = GetOwnerEnergyComponent())
	{
		EnergyComp->AddEnergy(EnergyPerReaction);
	}
}