#include "Element/LSElementComponent.h"
#include "Core/LSEventBus.h"
#include "Combat/LSDamageCalculator.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"

ULSElementComponent::ULSElementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false; // 初始关闭 Tick，身上有元素才开启（极致性能）
	SetIsReplicatedByDefault(true);
}

void ULSElementComponent::BeginPlay()
{
	Super::BeginPlay();

	// 仅服务端仲裁元素附着与反应
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		if (ULSEventBus* EventBus = ULSEventBus::Get(this))
		{
			EventBus->OnElementApplied.AddDynamic(this, &ULSElementComponent::HandleGlobalElementApplied);
		}
	}
}

void ULSElementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ULSElementComponent, ActiveAuras);
}

void ULSElementComponent::HandleGlobalElementApplied(AActor* Target, FGameplayTag ElementTag, ELSElementGauge Gauge)
{
	// 仅拦截打在自己身上的元素
	if (Target == GetOwner())
	{
		ApplyElement(nullptr, ElementTag, Gauge);
	}
}

void ULSElementComponent::ApplyElement(AActor* InstigatorActor, const FGameplayTag& IncomingElement, ELSElementGauge Gauge)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	if (!IncomingElement.IsValid() || Gauge == ELSElementGauge::None) return;

	// 1. ICD 内置冷却拦截（2.5 秒 / 3 次命中）
	if (bApplyICD && !CheckAndUpdateICD(InstigatorActor))
	{
		return;
	}

	float InitialGauge = 1.0f;
	float DecayRate = 0.1053f;
	ConvertGaugeData(Gauge, InitialGauge, DecayRate);

	// 风与岩不可附着为底元素：若身上无反应底，直接消散
	const bool bIsAnemo = (IncomingElement == LSTags::TAG_Element_Anemo);
	const bool bIsGeo = (IncomingElement == LSTags::TAG_Element_Geo);
	if ((bIsAnemo || bIsGeo) && ActiveAuras.Num() == 0)
	{
		return;
	}

	float RemainingIncomingGauge = InitialGauge;

	// 2. 遍历现有附着池进行反应仲裁
	for (int32 i = ActiveAuras.Num() - 1; i >= 0; --i)
	{
		FLSElementAura& Aura = ActiveAuras[i];

		// 同属性元素刷新（取最大剩余寿命与量级）
		if (Aura.ElementTag == IncomingElement)
		{
			const float FreshAttachedGauge = InitialGauge * 0.8f;
			Aura.CurrentGauge = FMath::Max(Aura.CurrentGauge, FreshAttachedGauge);
			Aura.DecayRate = FMath::Max(Aura.DecayRate, DecayRate);
			RemainingIncomingGauge = 0.0f;
			break;
		}

		// 异属性反应仲裁
		RemainingIncomingGauge = ArbitrateReaction(InstigatorActor, IncomingElement, RemainingIncomingGauge, Aura);

		// 底元素耗尽则移出附着池
		if (Aura.CurrentGauge <= 0.0f)
		{
			ActiveAuras.RemoveAt(i);
		}

		if (RemainingIncomingGauge <= 0.0f)
		{
			break;
		}
	}

	// 3. 反应后若仍有残留，且不是风/岩，则将剩余量作为新底元素挂上（扣除 0.8x 税）
	if (RemainingIncomingGauge > 0.0f && !bIsAnemo && !bIsGeo)
	{
		AttachNewAura(IncomingElement, RemainingIncomingGauge, DecayRate);
	}

	NotifyAuraChanged();
}

float ULSElementComponent::ArbitrateReaction(AActor* InstigatorActor, const FGameplayTag& Incoming, float IncomingGauge, FLSElementAura& TargetAura)
{
	FGameplayTag ReactionTag;
	float ConsumptionRatio = 1.0f; // 触发者 1 单位消耗底元素多少单位

	// ─── 增幅反应 (Vaporize / Melt) ───
	if (Incoming == LSTags::TAG_Element_Hydro && TargetAura.ElementTag == LSTags::TAG_Element_Pyro)
	{
		ReactionTag = LSTags::TAG_Reaction_Vaporize;
		ConsumptionRatio = 2.0f; // 顺向水打火 (1:2)
	}
	else if (Incoming == LSTags::TAG_Element_Pyro && TargetAura.ElementTag == LSTags::TAG_Element_Hydro)
	{
		ReactionTag = LSTags::TAG_Reaction_Vaporize;
		ConsumptionRatio = 0.5f; // 逆向火打水 (2:1)
	}
	else if (Incoming == LSTags::TAG_Element_Pyro && TargetAura.ElementTag == LSTags::TAG_Element_Cryo)
	{
		ReactionTag = LSTags::TAG_Reaction_Melt;
		ConsumptionRatio = 2.0f; // 顺向火打冰 (1:2)
	}
	else if (Incoming == LSTags::TAG_Element_Cryo && TargetAura.ElementTag == LSTags::TAG_Element_Pyro)
	{
		ReactionTag = LSTags::TAG_Reaction_Melt;
		ConsumptionRatio = 0.5f; // 逆向冰打火 (2:1)
	}
	// ─── 2. 感电共存特化 (水 + 雷) ───
	else if ((Incoming == LSTags::TAG_Element_Hydro && TargetAura.ElementTag == LSTags::TAG_Element_Electro) ||
			 (Incoming == LSTags::TAG_Element_Electro && TargetAura.ElementTag == LSTags::TAG_Element_Hydro))
	{
		// 水与雷共存！不直接冲抵底元素，将触发元素也作为底元素保留在池中
		AttachNewAura(Incoming, IncomingGauge, 0.1053f);
		
		ReactionTag = LSTags::TAG_Reaction_ElectroCharged;
		// 立即触发首次感电电击
		const float ReactionDamage = ULSDamageCalculator::CalculateTransformativeReactionDamage(ReactionTag, 90, 100.f, 0.1f, 0.0f);
		if (ULSEventBus* EventBus = ULSEventBus::Get(this))
		{
			EventBus->OnElementReactionTriggered.Broadcast(GetOwner(), ReactionTag, ReactionDamage, InstigatorActor);
		}
		OnReactionTriggered.Broadcast(ReactionTag, ReactionDamage, InstigatorActor, GetOwner());
		return 0.0f; // 已将触发元素纳管为共存底，剩余量归零
	}
	// ─── 3. 超载 (火 + 雷) ───
	else if ((Incoming == LSTags::TAG_Element_Pyro && TargetAura.ElementTag == LSTags::TAG_Element_Electro) ||
			 (Incoming == LSTags::TAG_Element_Electro && TargetAura.ElementTag == LSTags::TAG_Element_Pyro))
	{
		ReactionTag = LSTags::TAG_Reaction_Overload;
		ConsumptionRatio = 1.0f;
		TriggerOverloadExplosion(InstigatorActor);
	}
	// ─── 4. 超导 (冰 + 雷) ───
	else if ((Incoming == LSTags::TAG_Element_Cryo && TargetAura.ElementTag == LSTags::TAG_Element_Electro) ||
			 (Incoming == LSTags::TAG_Element_Electro && TargetAura.ElementTag == LSTags::TAG_Element_Cryo))
	{
		ReactionTag = LSTags::TAG_Reaction_Superconduct;
		ConsumptionRatio = 1.0f;
	}
	// ─── 5. 冻结 (水 + 冰) ───
	else if ((Incoming == LSTags::TAG_Element_Hydro && TargetAura.ElementTag == LSTags::TAG_Element_Cryo) ||
			 (Incoming == LSTags::TAG_Element_Cryo && TargetAura.ElementTag == LSTags::TAG_Element_Hydro))
	{
		ReactionTag = LSTags::TAG_Reaction_Freeze;
		ConsumptionRatio = 1.0f;
	}
	// ─── 6. 风系扩散 (Anemo) ───
	else if (Incoming == LSTags::TAG_Element_Anemo)
	{
		ReactionTag = LSTags::TAG_Reaction_Swirl;
		ConsumptionRatio = 0.5f;
		// 范围溅射传染当前的底元素
		TriggerSwirlSpread(InstigatorActor, TargetAura.ElementTag);
	}
	// ─── 7. 岩系结晶 (Geo) ───
	else if (Incoming == LSTags::TAG_Element_Geo)
	{
		ReactionTag = LSTags::TAG_Reaction_Crystallize;
		ConsumptionRatio = 0.5f;
	}

	if (!ReactionTag.IsValid())
	{
		return IncomingGauge;
	}

	// 扣除底元素
	const float DepletedAura = IncomingGauge * ConsumptionRatio;
	TargetAura.CurrentGauge -= DepletedAura;

	const float RemainderIncoming = (TargetAura.CurrentGauge < 0.0f) ? (-TargetAura.CurrentGauge / ConsumptionRatio) : 0.0f;
	
	// 剧变反应伤害广播
	const float ReactionDamage = ULSDamageCalculator::CalculateTransformativeReactionDamage(ReactionTag, 90, 100.f, 0.1f, 0.0f);
	
	if (ULSEventBus* EventBus = ULSEventBus::Get(this))
	{
		EventBus->OnElementReactionTriggered.Broadcast(GetOwner(), ReactionTag, ReactionDamage, InstigatorActor);
	}
	OnReactionTriggered.Broadcast(ReactionTag, ReactionDamage, InstigatorActor, GetOwner());
	
	return RemainderIncoming;
}

void ULSElementComponent::AttachNewAura(const FGameplayTag& ElementTag, float InitialGauge, float DecayRate)
{
	FLSElementAura NewAura;
	NewAura.ElementTag = ElementTag;
	NewAura.InitialGauge = InitialGauge;
	NewAura.CurrentGauge = InitialGauge * 0.8f; // 扣除 20% 附着税
	NewAura.DecayRate = DecayRate;

	ActiveAuras.Add(NewAura);
	SetComponentTickEnabled(true); // 身上有元素，开启 Tick
}

void ULSElementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	//1,水雷共存感电周期跳电检查
	ProcessELectroChargedTick(DeltaTime);

	// 2,线性自然衰减
	for (int32 i = ActiveAuras.Num() - 1; i >= 0; --i)
	{
		ActiveAuras[i].CurrentGauge -= ActiveAuras[i].DecayRate * DeltaTime;
		if (ActiveAuras[i].CurrentGauge <= 0.0f)
		{
			ActiveAuras.RemoveAt(i);
		}
	}

	// 附着清空后自动关闭 Tick，彻底消除无元素时的 CPU 消耗
	if (ActiveAuras.Num() == 0)
	{
		SetComponentTickEnabled(false);
	}

	NotifyAuraChanged();
}

bool ULSElementComponent::CheckAndUpdateICD(AActor* InstigatorActor)
{
	const uint32 InstigatorID = InstigatorActor ? InstigatorActor->GetUniqueID() : 0;
	const float CurrentTime = GetWorld()->GetTimeSeconds();

	FLSICDEntry& Entry = ICDMap.FindOrAdd(InstigatorID);

	// 超过 2.5 秒，强制重置计数
	if (CurrentTime - Entry.LastApplyTime >= 2.5f)
	{
		Entry.LastApplyTime = CurrentTime;
		Entry.HitCount = 1;
		return true; // 允许附着
	}

	Entry.HitCount++;
	// 第 1 次与第 4 次命中允许附着（3次命中重置计数）
	if (Entry.HitCount % 3 == 1)
	{
		Entry.LastApplyTime = CurrentTime;
		return true;
	}

	return false; // 处于 ICD 内，只造成物理/纯伤害，不挂元素
}

void ULSElementComponent::ConvertGaugeData(ELSElementGauge Gauge, float& OutInitialGauge, float& OutDecayRate)
{
	switch (Gauge)
	{
	case ELSElementGauge::Light:      OutInitialGauge = 1.0f; OutDecayRate = 0.1053f; break; // 1U, 9.5s
	case ELSElementGauge::Heavy:      OutInitialGauge = 2.0f; OutDecayRate = 0.1667f; break; // 2U, 12s
	case ELSElementGauge::SuperHeavy: OutInitialGauge = 4.0f; OutDecayRate = 0.2353f; break; // 4U, 17s
	default:                          OutInitialGauge = 1.0f; OutDecayRate = 0.1053f; break;
	}
}

TArray<FGameplayTag> ULSElementComponent::GetActiveElementTags() const
{
	TArray<FGameplayTag> Tags;
	for (const FLSElementAura& Aura : ActiveAuras)
	{
		Tags.Add(Aura.ElementTag);
	}
	return Tags;
}

bool ULSElementComponent::HasElementAura(const FGameplayTag& ElementTag) const
{
	for (const FLSElementAura& Aura : ActiveAuras)
	{
		if (Aura.ElementTag == ElementTag) return true;
	}
	return false;
}

FGameplayTag ULSElementComponent::GetPrimaryAuraTag() const
{
	return (ActiveAuras.Num() > 0) ? ActiveAuras[0].ElementTag : FGameplayTag();
}

void ULSElementComponent::OnRep_ActiveAuras()
{
	NotifyAuraChanged();
}

void ULSElementComponent::NotifyAuraChanged()
{
	OnAuraChanged.Broadcast(GetActiveElementTags());
}

void ULSElementComponent::ProcessElectroChargedTick(float DeltaTime)
{
	const bool bHasHydro = HasElementAura(LSTags::TAG_Element_Hydro);
	const bool bHasElectro = HasElementAura(LSTags::TAG_Element_Electro);

	if (bHasHydro && bHasElectro)
	{
		ElectroChargedTimer += DeltaTime;
		if (ElectroChargedTimer >= 1.0f)
		{
			ElectroChargedTimer = 0.0f;

			// 每秒跳电：造成感电伤害
			const float ReactionDamage = ULSDamageCalculator::CalculateTransformativeReactionDamage(
				LSTags::TAG_Reaction_ElectroCharged, 90, 100.f, 0.1f, 0.0f
			);

			// 水雷双方各扣除 0.4U
			for (int32 i = ActiveAuras.Num() - 1; i >= 0; --i)
			{
				if (ActiveAuras[i].ElementTag == LSTags::TAG_Element_Hydro ||
					ActiveAuras[i].ElementTag == LSTags::TAG_Element_Electro)
				{
					ActiveAuras[i].CurrentGauge -= 0.4f;
					if (ActiveAuras[i].CurrentGauge <= 0.0f)
					{
						ActiveAuras.RemoveAt(i);
					}
				}
			}

			if (ULSEventBus* EventBus = ULSEventBus::Get(this))
			{
				EventBus->OnElementReactionTriggered.Broadcast(GetOwner(), LSTags::TAG_Reaction_ElectroCharged, ReactionDamage, nullptr);
			}
			OnReactionTriggered.Broadcast(LSTags::TAG_Reaction_ElectroCharged, ReactionDamage, nullptr, GetOwner());
		}
	}
	else
	{
		ElectroChargedTimer = 0.0f;
	}
}

void ULSElementComponent::TriggerSwirlSpread(AActor* InstigatorActor, const FGameplayTag& AuraToSpread)
{
	if (!GetOwner() || !GetWorld()) return;

	const FVector Origin = GetOwner()->GetActorLocation();
	const float SwirlRadius = 500.0f; // 5米扩散半径

	TArray<FOverlapResult> Overlaps;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(SwirlRadius);
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetOwner());

	const bool bHit = GetWorld()->OverlapMultiByChannel(Overlaps, Origin, FQuat::Identity, ECC_Pawn, Sphere, Params);
	if (bHit)
	{
		for (const FOverlapResult& Overlap : Overlaps)
		{
			AActor* NearbyActor = Overlap.GetActor();
			if (NearbyActor && NearbyActor != GetOwner())
			{
				if (ULSElementComponent* NearbyComp = NearbyActor->FindComponentByClass<ULSElementComponent>())
				{
					// 向周围敌人附着被扩散的属性 (1U 弱元素)
					NearbyComp->ApplyElement(InstigatorActor, AuraToSpread, ELSElementGauge::Light);
				}
			}
		}
	}
}

void ULSElementComponent::TriggerOverloadExplosion(AActor* InstigatorActor)
{
	if (!GetOwner() || !GetWorld()) return;

	const FVector Origin = GetOwner()->GetActorLocation();
	const float Radius = 400.0f;

	// 对角色施加击退冲量 (LaunchCharacter)
	if (ACharacter* Char = Cast<ACharacter>(GetOwner()))
	{
		FVector KnockbackDir = (Origin - (InstigatorActor ? InstigatorActor->GetActorLocation() : Origin)).GetSafeNormal();
		KnockbackDir.Z = 0.5f; // 略微向上抛起
		Char->LaunchCharacter(KnockbackDir * 600.0f, true, true);
	}
}