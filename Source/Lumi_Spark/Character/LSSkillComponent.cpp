#include "LSSkillComponent.h"
#include "LSCharacterBase.h"
#include "Core/LSEventBus.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"

ULSSkillComponent::ULSSkillComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false; // 初始关闭 Tick，技能冷却走表时才开启
	SetIsReplicatedByDefault(true);
}

void ULSSkillComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// 仅服务端权威端监听全局事件总线并计算能量充能
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		if (ULSEventBus* EventBus = ULSEventBus::Get(this))
		{
			//监听伤害事件
			EventBus->OnDamageDealt.AddDynamic(this, &ULSSkillComponent::HandleDamageDealt);
			
			//监听元素反应事件
			EventBus->OnElementReactionTriggered.AddDynamic(this, &ULSSkillComponent::HandleReactionTriggered);
		}
	}
}

void ULSSkillComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	//网络同步当前大招能量池
	DOREPLIFETIME(ULSSkillComponent, CurrentEnergy);
}

void ULSSkillComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	//E技能冷却递减倒计时
	if (SkillCooldownTimer > 0.0f)
	{
		SkillCooldownTimer -= DeltaTime;
		OnSkillCooldownChanged.Broadcast(SkillCooldownTimer, MaxSkillCooldown);
		if (SkillCooldownTimer < 0.0f)
		{
			SkillCooldownTimer = 0.0f;
			SetComponentTickEnabled(false);
		}
	}
}

bool ULSSkillComponent::CanCastSkill() const
{
	//1,冷却中无法释放
	if (SkillCooldownTimer > 0.0f) return false;
	
	//2,角色阵亡无法释放
	if (const ALSCharacterBase* Char = Cast<ALSCharacterBase>(GetOwner()))
	{
		if (Char->IsDead()) return false;
	}
	
	return true;
}

bool ULSSkillComponent::CanCastBurst() const
{
	// 1. 能量未满无法释放
	if (CurrentEnergy < MaxEnergy) return false;
	
	// 2. 角色阵亡无法释放
	if (const ALSCharacterBase* Char = Cast<ALSCharacterBase>(GetOwner()))
	{
		if (Char->IsDead()) return false;
	}
	
	return true;
}

bool ULSSkillComponent::CastSkill()
{
	if (!CanCastSkill()) return false;
	
	// 触发冷却并开启 Tick
	SkillCooldownTimer = MaxSkillCooldown;
	SetComponentTickEnabled(true);
	
	// 执行具体技能逻辑（C++ 虚函数或蓝图事件）
	ExecuteSkillLogic();
	
	// 广播事件
	OnSkillCast.Broadcast(SkillTag, GetOwner());
	OnSkillCooldownChanged.Broadcast(SkillCooldownTimer, MaxSkillCooldown);
	
	return true;
}

bool ULSSkillComponent::CastBurst()
{
	if (!CanCastBurst()) return false;
	
	// 清空大招能量槽
	CurrentEnergy = 0.0f;
	OnEnergyChanged.Broadcast(CurrentEnergy, MaxEnergy);
	
	// 执行毁灭性大招逻辑
	ExecuteBurstLogic();
	
	// 广播事件
	OnBurstCast.Broadcast(BurstTag, GetOwner());
	
	return true;
}

void ULSSkillComponent::AddEnergy(float EnergyAmount)
{
	if (EnergyAmount <= 0.0f) return;
	
	// 仅服务端或单机模式权威修改属性
	if (GetOwner() && !GetOwner()->HasAuthority()) return;
	
	const float OldEnergy = CurrentEnergy;
	CurrentEnergy = FMath::Clamp(CurrentEnergy + EnergyAmount, 0.0f, MaxEnergy);
	
	if (!FMath::IsNearlyEqual(OldEnergy, CurrentEnergy))
	{
		OnEnergyChanged.Broadcast(CurrentEnergy, MaxEnergy);
	}
}

void ULSSkillComponent::ResetSkillCooldown()
{
	SkillCooldownTimer = 0.0f;
	SetComponentTickEnabled(false);
	OnSkillCooldownChanged.Broadcast(0.0f, MaxSkillCooldown);
}

float ULSSkillComponent::GetSkillCooldownRatio() const
{
	return MaxSkillCooldown > 0.0f ? FMath::Clamp(SkillCooldownTimer / MaxSkillCooldown, 0.0f, 1.0f) : 0.0f;
}

float ULSSkillComponent::GetEnergyRatio() const
{
	return MaxEnergy > 0.0f ? FMath::Clamp(CurrentEnergy / MaxEnergy, 0.0f, 1.0f) : 0.0f;
}

void ULSSkillComponent::ExecuteSkillLogic_Implementation()
{
	// 默认基础表现：可在派生类或蓝图中重写
}

void ULSSkillComponent::ExecuteBurstLogic_Implementation()
{
	// 默认基础表现：可在派生类或蓝图中重写
}

void ULSSkillComponent::OnRep_CurrentEnergy()
{
	// 客户端接收服务端权威能量同步，更新本地 UI
	OnEnergyChanged.Broadcast(CurrentEnergy, MaxEnergy);
}

void ULSSkillComponent::HandleDamageDealt(const FLSDamageContext& DamageContext)
{
	// 仅当伤害制造者是自己时，才吸取直伤转换的能量
	if (DamageContext.DamageCauser == GetOwner())
	{
		AddEnergy(DamageContext.FinalDamage * EnergyDamageConversionRate);
	}
}

void ULSSkillComponent::HandleReactionTriggered(AActor* Target, FGameplayTag InReactionTag, float ReactionDamage, AActor* Instigator)
{
	// 仅当元素反应发起者是自己时，奖励大额能量粒子
	if (Instigator == GetOwner())
	{
		AddEnergy(EnergyPerReaction);
	}
}