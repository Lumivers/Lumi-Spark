#include "LSThreatComponent.h"
#include "GameFramework/Pawn.h"

ULSThreatComponent::ULSThreatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.5f; // 每 0.5 秒更新一次仇恨衰减
}

void ULSThreatComponent::BeginPlay()
{
	Super::BeginPlay();
}

void ULSThreatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	// 仇恨自然衰减
	TArray<TWeakObjectPtr<AActor>> KeysToRemove;
	for (auto& Pair : ThreatMap)
	{
		if (!Pair.Key.IsValid())
		{
			KeysToRemove.Add(Pair.Key);
			continue;
		}
		
		Pair.Value = FMath::Max(0.0f, Pair.Value * (1.0f - ThreatDecayRate * DeltaTime));
		if (Pair.Value <= 0.0f)
		{
			KeysToRemove.Add(Pair.Key);
		}
	}
	
	for (const auto& Key : KeysToRemove)
	{
		ThreatMap.Remove(Key);
	}
	
	// 检查当前仇恨最高目标是否发生变化
	EvaluateHighestThreatTarget();
}

void ULSThreatComponent::AddThreat(AActor* ThreatCauser, float ThreatAmount)
{
	if (!ThreatCauser || ThreatAmount <= 0.0f || !GetOwner() || !GetOwner()->HasAuthority()) return;
	
	float& CurrentVal = ThreatMap.FindOrAdd(ThreatCauser);
	CurrentVal += ThreatAmount;
	
	EvaluateHighestThreatTarget();
}

void ULSThreatComponent::AddThreatFromDamage(AActor* DamageCauser, float Damage, bool bIsReaction)
{
	if (!DamageCauser) return;
	
	// 反应伤害享有 1.5x 仇恨加权
	const float Multiplier = bIsReaction ? 1.5f : 1.0f;
	AddThreat(DamageCauser, Damage * Multiplier);
}

void ULSThreatComponent::AddThreatFromRevive(AActor* Reviver, float DeltaSeconds)
{
	if (!Reviver) return;
	AddThreat(Reviver, ReviveThreatPerSecond * DeltaSeconds);
}

AActor* ULSThreatComponent::GetHighestThreatTarget() const
{
	return CurrentHighestTarget.IsValid() ? CurrentHighestTarget.Get() : nullptr;
}

void ULSThreatComponent::ClearAllThreat()
{
	ThreatMap.Empty();
	CurrentHighestTarget = nullptr;
}

void ULSThreatComponent::EvaluateHighestThreatTarget()
{
	AActor* BestTarget = nullptr;
	float MaxThreat = 0.0f;
	
	for (const auto& Pair : ThreatMap)
	{
		if (Pair.Key.IsValid() && Pair.Value > MaxThreat)
		{
			// 过滤掉已处于倒地或阵亡的目标
			if (Pair.Key->ActorHasTag(TEXT("State.Downed")) || Pair.Key->ActorHasTag(TEXT("State.Dead")))
			{
				continue;
			}
			
			MaxThreat = Pair.Value;
			BestTarget = Pair.Key.Get();
		}
	}
	
	if (BestTarget != CurrentHighestTarget.Get())
	{
		AActor* OldTarget = CurrentHighestTarget.Get();
		CurrentHighestTarget = BestTarget;
		OnThreatTargetChanged.Broadcast(OldTarget, BestTarget);
	}
}
