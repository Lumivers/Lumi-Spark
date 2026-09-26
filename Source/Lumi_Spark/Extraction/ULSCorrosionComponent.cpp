#include "Extraction/ULSCorrosionComponent.h"
#include "Net/UnrealNetwork.h"
#include "Character/LSCharacterBase.h"
#include "Character/LSHealthComponent.h"
#include "Extraction/ULSBackpackComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

ULSCorrosionComponent::ULSCorrosionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	SetIsReplicatedByDefault(true);
}

void ULSCorrosionComponent::BeginPlay()
{
	Super::BeginPlay();
	OverloadDamageTimer = 0.0f;
}

void ULSCorrosionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ULSCorrosionComponent, CurrentCorrosion);
	DOREPLIFETIME(ULSCorrosionComponent, CurrentFilterDurability);
	DOREPLIFETIME(ULSCorrosionComponent, bIsOverloaded);
}

void ULSCorrosionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 仅服务端执行权威状态推演
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	// 1. 滤芯耐久消耗与侵蚀阻隔判定
	if (CurrentFilterDurability > 0.0f)
	{
		CurrentFilterDurability = FMath::Max(0.0f, CurrentFilterDurability - (FilterConsumptionRate * DeltaTime));
		if (CurrentFilterDurability <= 0.0f)
		{
			OnFilterDepleted.Broadcast();
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Orange, TEXT("⚠️ 【滤芯耗尽】抗侵蚀过滤器耐久已归零，地脉侵蚀加速扩散！"));
		}
		// 滤芯完好时：100% 阻隔地脉侵蚀（侵蚀度不增长）
	}
	else
	{
		// 滤芯耗尽：承受加速侵蚀扩散
		const float EffectiveGainRate = BaseCorrosionRate * ZoneCorrosionMultiplier * DepletedCorrosionMultiplier;
		CurrentCorrosion = FMath::Clamp(CurrentCorrosion + (EffectiveGainRate * DeltaTime), 0.0f, MaxCorrosion);

		// 检查是否达到 100% 进入过载
		if (CurrentCorrosion >= MaxCorrosion && !bIsOverloaded)
		{
			bIsOverloaded = true;
			OverloadDamageTimer = 0.0f;
			OnCorrosionOverloadChanged.Broadcast(true);
			GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Red, TEXT("🚨 【地脉过载】侵蚀度达到 100%！机体减速 30%，持续承受周期百分比生命流失！"));
		}
	}

	// 2. 过载惩罚：周期性百分比真伤扣血
	if (bIsOverloaded)
	{
		OverloadDamageTimer += DeltaTime;
		if (OverloadDamageTimer >= OverloadDamageInterval)
		{
			OverloadDamageTimer = 0.0f;
			ApplyOverloadDamage();
		}
	}

	// 广播本地委托（服务端本地刷新）
	OnCorrosionUpdated.Broadcast(CurrentCorrosion, MaxCorrosion, CurrentFilterDurability);
}

bool ULSCorrosionComponent::UsePurificationInjector(float CleanseAmount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;

	if (CurrentCorrosion <= 0.0f && !bIsOverloaded)
	{
		return false; // 处于健康状态无需注射
	}

	const float OldCorrosion = CurrentCorrosion;
	CurrentCorrosion = FMath::Clamp(CurrentCorrosion - CleanseAmount, 0.0f, MaxCorrosion);

	// 若清空了满溢过载
	if (bIsOverloaded && CurrentCorrosion < MaxCorrosion)
	{
		bIsOverloaded = false;
		OverloadDamageTimer = 0.0f;
		OnCorrosionOverloadChanged.Broadcast(false);
	}

	OnCorrosionUpdated.Broadcast(CurrentCorrosion, MaxCorrosion, CurrentFilterDurability);
	GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Cyan, FString::Printf(TEXT("💉 【战术净化】注射完成，清除 %.1f 侵蚀度（当前: %.1f%%）"), OldCorrosion - CurrentCorrosion, CurrentCorrosion));
	return true;
}

bool ULSCorrosionComponent::InstallFilter(float DurabilityAmount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || DurabilityAmount <= 0.0f) return false;

	CurrentFilterDurability = FMath::Clamp(CurrentFilterDurability + DurabilityAmount, 0.0f, MaxFilterDurability);
	OnFilterInstalled.Broadcast(DurabilityAmount);
	OnCorrosionUpdated.Broadcast(CurrentCorrosion, MaxCorrosion, CurrentFilterDurability);

	GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Green, FString::Printf(TEXT("🛡️ 【滤芯更换】已装填新滤芯，剩余防护耐久: %.1f 秒"), CurrentFilterDurability));
	return true;
}

void ULSCorrosionComponent::SetZoneCorrosionMultiplier(float NewMultiplier)
{
	ZoneCorrosionMultiplier = FMath::Max(0.0f, NewMultiplier);
}

bool ULSCorrosionComponent::ConsumeInjectorFromBackpack()
{
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC) return false;

	ULSBackpackComponent* Backpack = PC->FindComponentByClass<ULSBackpackComponent>();
	if (!Backpack) return false;

	// 遍历普通背包槽位查找战术消耗品 "Item_Injector"
	for (int32 i = 0; i < Backpack->BackpackSlots.Num(); ++i)
	{
		const FLSInventoryItem& Item = Backpack->BackpackSlots[i];
		if (Item.ItemType == ELSExtractionItemType::Consumable && Item.ItemID.ToString().Contains(TEXT("Injector")))
		{
			if (UsePurificationInjector(100.0f))
			{
				Backpack->RemoveFromBackpack(i, 1);
				return true;
			}
		}
	}

	GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("❌ 背包中无可用【地脉净化针】！"));
	return false;
}

bool ULSCorrosionComponent::ConsumeFilterFromBackpack()
{
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC) return false;

	ULSBackpackComponent* Backpack = PC->FindComponentByClass<ULSBackpackComponent>();
	if (!Backpack) return false;

	for (int32 i = 0; i < Backpack->BackpackSlots.Num(); ++i)
	{
		const FLSInventoryItem& Item = Backpack->BackpackSlots[i];
		if (Item.ItemType == ELSExtractionItemType::Consumable && Item.ItemID.ToString().Contains(TEXT("Filter")))
		{
			if (InstallFilter(120.0f))
			{
				Backpack->RemoveFromBackpack(i, 1);
				return true;
			}
		}
	}

	GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("❌ 背包中无可用【抗侵蚀滤芯】！"));
	return false;
}

ALSCharacterBase* ULSCorrosionComponent::GetActiveCharacter() const
{
	if (APlayerController* PC = Cast<APlayerController>(GetOwner()))
	{
		return Cast<ALSCharacterBase>(PC->GetPawn());
	}
	return Cast<ALSCharacterBase>(GetOwner());
}

void ULSCorrosionComponent::ApplyOverloadDamage()
{
	ALSCharacterBase* ActiveChar = GetActiveCharacter();
	if (!ActiveChar || ActiveChar->IsDead()) return;

	ULSHealthComponent* HealthComp = ActiveChar->GetHealthComponent();
	if (!HealthComp || HealthComp->IsDead()) return;

	// 真实伤害：按出战角色最大生命的百分比扣除
	const float DamageToApply = HealthComp->GetMaxHealth() * OverloadDamagePercent;
	HealthComp->TakeDamage(DamageToApply, LSTags::TAG_Damage_Type_DoT, ActiveChar, ActiveChar->GetController());

	GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Red, FString::Printf(TEXT("☣️ 【地脉侵蚀伤害】机体承受侵蚀真实伤害: -%.0f HP"), DamageToApply));
}

float ULSCorrosionComponent::GetVignetteIntensity() const
{
	// 侵蚀度 < 60% 无视效
	if (CurrentCorrosion < 60.0f)
	{
		return 0.0f;
	}

	// 60% ~ 100%：平滑抬升暗角 (0.0 ~ 0.5)
	if (!bIsOverloaded && CurrentCorrosion < MaxCorrosion)
	{
		const float Alpha = (CurrentCorrosion - 60.0f) / 40.0f;
		return FMath::Clamp(Alpha * 0.5f, 0.0f, 0.5f);
	}

	// 过载 100%：正弦心跳脉冲警告 (0.6 ~ 0.95)
	if (UWorld* World = GetWorld())
	{
		const float Pulse = (FMath::Sin(World->GetTimeSeconds() * 5.0f) + 1.0f) * 0.5f; // [0, 1]
		return 0.65f + (Pulse * 0.30f);
	}

	return 0.75f;
}

void ULSCorrosionComponent::OnRep_CurrentCorrosion()
{
	OnCorrosionUpdated.Broadcast(CurrentCorrosion, MaxCorrosion, CurrentFilterDurability);
}

void ULSCorrosionComponent::OnRep_FilterDurability()
{
	OnCorrosionUpdated.Broadcast(CurrentCorrosion, MaxCorrosion, CurrentFilterDurability);
}

void ULSCorrosionComponent::OnRep_IsOverloaded()
{
	OnCorrosionOverloadChanged.Broadcast(bIsOverloaded);
}