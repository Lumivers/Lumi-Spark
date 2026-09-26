#include "Combat/LSShieldComponent.h"
#include "Net/UnrealNetwork.h"
#include "Core/LSEventBus.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

ULSShieldComponent::ULSShieldComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void ULSShieldComponent::BeginPlay()
{
	Super::BeginPlay();
}

void ULSShieldComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ULSShieldComponent, ShieldLayers);
}

bool ULSShieldComponent::HasActiveShield() const
{
	for (const FLSShieldLayer& Layer : ShieldLayers)
	{
		if (Layer.CurrentShield > 0.0f)
		{
			return true;
		}
	}
	return false;
}

FGameplayTag ULSShieldComponent::GetActiveShieldElement() const
{
	for (const FLSShieldLayer& Layer : ShieldLayers)
	{
		if (Layer.CurrentShield > 0.0f)
		{
			return Layer.ShieldElement;
		}
	}
	return FGameplayTag();
}

float ULSShieldComponent::GetActiveShieldRatio() const
{
	for (const FLSShieldLayer& Layer : ShieldLayers)
	{
		if (Layer.CurrentShield > 0.0f && Layer.MaxShield > 0.0f)
		{
			return FMath::Clamp(Layer.CurrentShield / Layer.MaxShield, 0.0f, 1.0f);
		}
	}
	return 0.0f;
}

float ULSShieldComponent::CalculateElementalBreakMultiplier(const FGameplayTag& AttackElement, const FGameplayTag& ShieldElement) const
{
	// 同属性免疫（例如：冰枪打冰盾 0 伤害，彻底免疫）
	if (AttackElement.IsValid() && AttackElement == ShieldElement)
	{
		return 0.0f;
	}

	// 无元素物理攻击（子弹白字）：对元素盾只有 0.4x 的微量磨盾效果
	if (!AttackElement.IsValid() || AttackElement == LSTags::TAG_Element_Physical)
	{
		// 物理对岩盾正常生效（1.0x），对其他元素盾严重削减
		if (ShieldElement == LSTags::TAG_Element_Geo) return 1.0f;
		return 0.4f;
	}

	// 高等元素克制矩阵（2.0x 强克制）
	if (ShieldElement == LSTags::TAG_Element_Cryo && AttackElement == LSTags::TAG_Element_Pyro) return 2.0f;  // 火克冰（融化破盾）
	if (ShieldElement == LSTags::TAG_Element_Pyro && AttackElement == LSTags::TAG_Element_Hydro) return 2.0f; // 水克火（蒸发破盾）
	if (ShieldElement == LSTags::TAG_Element_Electro && AttackElement == LSTags::TAG_Element_Dendro) return 2.0f; // 草克雷（激化破盾）
	if (ShieldElement == LSTags::TAG_Element_Hydro && AttackElement == LSTags::TAG_Element_Electro) return 1.5f;  // 雷克水（感电破盾）
	if (ShieldElement == LSTags::TAG_Element_Geo && (AttackElement == LSTags::TAG_Element_Geo || AttackElement == LSTags::TAG_Damage_Type_Explosion)) return 3.0f; // 钝击/爆炸克岩盾

	// 其余普通元素消耗（1.0x 基准消耗）
	return 1.0f;
}

float ULSShieldComponent::AbsorbDamage(float InDamage, const FGameplayTag& DamageElement, AActor* DamageCauser)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || InDamage <= 0.0f)
	{
		return InDamage;
	}

	float RemainingDamage = InDamage;

	// 从最外层（数组首部）开始依次破盾
	for (int32 i = 0; i < ShieldLayers.Num(); ++i)
	{
		FLSShieldLayer& Layer = ShieldLayers[i];
		if (Layer.CurrentShield <= 0.0f) continue;

		const float Multiplier = CalculateElementalBreakMultiplier(DamageElement, Layer.ShieldElement);

		// 如果同属性完全免疫，直接吞噬所有伤害，护盾不掉血，本体也不掉血
		if (Multiplier <= 0.0f)
		{
			return 0.0f;
		}

		// 计算经过元素克制倍率放大后的破盾当量
		const float EffectiveShieldDamage = RemainingDamage * Multiplier;

		if (Layer.CurrentShield >= EffectiveShieldDamage)
		{
			// 本层护盾足以完全吸收
			Layer.CurrentShield -= EffectiveShieldDamage;
			RemainingDamage = 0.0f;

			OnShieldDamaged.Broadcast(Layer.ShieldElement, Layer.CurrentShield, Layer.MaxShield);
			break;
		}
		else
		{
			// 本层护盾被击破，计算穿透的剩余伤害折算回未放大的伤害基数
			const float AbsorbedRaw = Layer.CurrentShield / Multiplier;
			Layer.CurrentShield = 0.0f;
			RemainingDamage = FMath::Max(0.0f, RemainingDamage - AbsorbedRaw);

			// 广播单层破盾事件与全局事件总线
			OnShieldLayerBroken.Broadcast(Layer.ShieldElement);
			if (ULSEventBus* EventBus = ULSEventBus::Get(this))
			{
				EventBus->OnShieldBroken.Broadcast(GetOwner(), Layer.ShieldElement);
			}

			// 检查是否全盾已破
			if (!HasActiveShield())
			{
				OnAllShieldsDepleted.Broadcast();
				ApplyShieldBreakStun();
				break;
			}
		}
	}

	return RemainingDamage;
}

void ULSShieldComponent::ApplyShieldBreakStun()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority()) return;

	// 给 Actor 贴上破盾虚弱标签
	OwnerActor->Tags.AddUnique(TEXT("State.Stunned"));

	// 若是 Character，短暂中断移动进入硬直虚弱
	if (ACharacter* Char = Cast<ACharacter>(OwnerActor))
	{
		if (UCharacterMovementComponent* MoveComp = Char->GetCharacterMovement())
		{
			MoveComp->DisableMovement();
		}
	}

	// 启动恢复定时器
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(StunRecoveryTimerHandle, this, &ULSShieldComponent::RecoverFromStun, ShieldBreakStunDuration, false);
	}

	GEngine->AddOnScreenDebugMessage(-1, 3.5f, FColor::Purple, FString::Printf(TEXT("💥 【破盾虚弱】%s 所有护盾已破裂，进入瘫痪虚弱状态！"), *OwnerActor->GetName()));
}

void ULSShieldComponent::RecoverFromStun()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority()) return;

	OwnerActor->Tags.Remove(TEXT("State.Stunned"));

	if (ACharacter* Char = Cast<ACharacter>(OwnerActor))
	{
		if (UCharacterMovementComponent* MoveComp = Char->GetCharacterMovement())
		{
			MoveComp->SetMovementMode(MOVE_Walking);
		}
	}
}

void ULSShieldComponent::AddShieldLayer(const FGameplayTag& ShieldElement, float ShieldAmount, float AbsorptionRatio)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	FLSShieldLayer NewLayer;
	NewLayer.ShieldElement = ShieldElement;
	NewLayer.MaxShield = ShieldAmount;
	NewLayer.CurrentShield = ShieldAmount;
	NewLayer.AbsorptionRatio = AbsorptionRatio;

	ShieldLayers.Add(NewLayer);
}

void ULSShieldComponent::RestoreAllShields()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	for (FLSShieldLayer& Layer : ShieldLayers)
	{
		Layer.CurrentShield = Layer.MaxShield;
	}
}

void ULSShieldComponent::OnRep_ShieldLayers()
{
	// 客户端接收到同步时向本地 UI 触发最新生效护盾数据
	for (const FLSShieldLayer& Layer : ShieldLayers)
	{
		if (Layer.CurrentShield > 0.0f)
		{
			OnShieldDamaged.Broadcast(Layer.ShieldElement, Layer.CurrentShield, Layer.MaxShield);
			return;
		}
	}
}

void ULSShieldComponent::ShatterAllShields()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	bool bHadAnyShield = false;

	// 遍历并将所有仍有护盾量的层数清零，同时触发单层破损事件
	for (FLSShieldLayer& Layer : ShieldLayers)
	{
		if (Layer.CurrentShield > 0.0f)
		{
			bHadAnyShield = true;
			Layer.CurrentShield = 0.0f;
			OnShieldLayerBroken.Broadcast(Layer.ShieldElement);
		}
	}

	// 若此前确实存在护盾，触发全盾破损与全局事件总线
	if (bHadAnyShield)
	{
		OnAllShieldsDepleted.Broadcast();
		if (ULSEventBus* EventBus = ULSEventBus::Get(this))
		{
			EventBus->OnShieldBroken.Broadcast(GetOwner(), FGameplayTag());
		}
	}

	// 处决一击强制定身瘫痪
	ApplyShieldBreakStun();
}