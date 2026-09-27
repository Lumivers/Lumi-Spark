#include "Extraction/ULSCorrosionComponent.h"
#include "Net/UnrealNetwork.h"
#include "Character/LSCharacterBase.h"
#include "Character/LSHealthComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

ULSCorrosionComponent::ULSCorrosionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	SetIsReplicatedByDefault(true);

	// 默认空装/跑刀状态（耐久 0，无抗侵蚀与承伤减免）
	CurrentMaskDurability = 0;
	EquippedAntiCorrosionGear = FLSAntiCorrosionGearData();
	EquippedAntiCorrosionGear.MaxDurability = 0;
	EquippedAntiCorrosionGear.CorrosionMitigation = 0.0f;
	EquippedAntiCorrosionGear.DamageMitigation = 0.0f;
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
	DOREPLIFETIME(ULSCorrosionComponent, CurrentMaskDurability);
	DOREPLIFETIME(ULSCorrosionComponent, EquippedAntiCorrosionGear);
	DOREPLIFETIME(ULSCorrosionComponent, bIsInCorrosionZone);
	DOREPLIFETIME(ULSCorrosionComponent, bIsOverloaded);
}

void ULSCorrosionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 仅服务端权威执行侵蚀累积与过载扣血
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	// 1. 侵蚀度累积计算（局内绝不扣抗蚀器耐久点数）
	if (bIsInCorrosionZone)
	{
		float EffectiveRate = BaseCorrosionRate;

		// 若抗蚀器完好（耐久点数 > 0），根据品质减免积累速度（绿10% / 蓝25% / 紫40% / 金55%）
		if (CurrentMaskDurability > 0)
		{
			EffectiveRate *= (1.0f - EquippedAntiCorrosionGear.CorrosionMitigation);
		}

		CurrentCorrosion = FMath::Clamp(CurrentCorrosion + (EffectiveRate * DeltaTime), 0.0f, MaxCorrosion);

		// 触发 100% 过载
		if (CurrentCorrosion >= MaxCorrosion && !bIsOverloaded)
		{
			bIsOverloaded = true;
			OverloadDamageTimer = 0.0f;
			OnCorrosionOverloadChanged.Broadcast(true);
			GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Red, TEXT("🚨 【地脉过载】侵蚀度达到 100%！生命回复上限压制至 30%，承受持续生命流失！"));
		}
	}

	// 2. 侵蚀实时压制当前出战角色的生命恢复上限（锁血条上限，不影响 3C 移速）
	if (ALSCharacterBase* ActiveChar = GetActiveCharacter())
	{
		if (ULSHealthComponent* HealthComp = ActiveChar->GetHealthComponent())
		{
			HealthComp->SetHealthCapRatio(GetHealthRecoveryCapPercent());
		}
	}

	// 3. 过载惩罚：周期性真实伤害
	if (bIsOverloaded)
	{
		OverloadDamageTimer += DeltaTime;
		if (OverloadDamageTimer >= OverloadDamageInterval)
		{
			OverloadDamageTimer = 0.0f;
			ApplyOverloadDamage();
		}
	}

	OnCorrosionUpdated.Broadcast(CurrentCorrosion, MaxCorrosion, CurrentMaskDurability);
}

bool ULSCorrosionComponent::UsePurificationInjector(float CleanseAmount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
	if (CurrentCorrosion <= 0.0f && !bIsOverloaded) return false;

	const float OldCorrosion = CurrentCorrosion;
	CurrentCorrosion = FMath::Clamp(CurrentCorrosion - CleanseAmount, 0.0f, MaxCorrosion);

	// 清除过载状态
	if (bIsOverloaded && CurrentCorrosion < MaxCorrosion)
	{
		bIsOverloaded = false;
		OverloadDamageTimer = 0.0f;
		OnCorrosionOverloadChanged.Broadcast(false);
	}

	// 立即恢复被压制的生命上限
	if (ALSCharacterBase* ActiveChar = GetActiveCharacter())
	{
		if (ULSHealthComponent* HealthComp = ActiveChar->GetHealthComponent())
		{
			HealthComp->SetHealthCapRatio(GetHealthRecoveryCapPercent());
		}
	}

	OnCorrosionUpdated.Broadcast(CurrentCorrosion, MaxCorrosion, CurrentMaskDurability);
	GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Cyan, FString::Printf(TEXT("💉 【战术净化】已注射！清除 %.1f 侵蚀度（当前: %.1f%%，生命回复上限恢复至 %.0f%%）"), OldCorrosion - CurrentCorrosion, CurrentCorrosion, GetHealthRecoveryCapPercent() * 100.0f));
	return true;
}

void ULSCorrosionComponent::ApplyRaidDurabilityDeduction(ELSHazardDifficulty RaidHazard)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	// 消耗规则：简单1、中等1、困难2、高危3
	int32 Deduction = 1;
	switch (RaidHazard)
	{
	case ELSHazardDifficulty::Low:     Deduction = 1; break;
	case ELSHazardDifficulty::Medium:  Deduction = 1; break;
	case ELSHazardDifficulty::Hard:    Deduction = 2; break;
	case ELSHazardDifficulty::Extreme: Deduction = 3; break;
	}

	CurrentMaskDurability = FMath::Max(0, CurrentMaskDurability - Deduction);
	OnCorrosionUpdated.Broadcast(CurrentCorrosion, MaxCorrosion, CurrentMaskDurability);

	GEngine->AddOnScreenDebugMessage(-1, 3.5f, FColor::Yellow, FString::Printf(TEXT("📦 【撤离磨损结算】本局难度消耗 %d 点抗蚀器耐久，剩余: %d / %d"), Deduction, CurrentMaskDurability, EquippedAntiCorrosionGear.MaxDurability));

	if (CurrentMaskDurability <= 0)
	{
		OnMaskDepleted.Broadcast();
		GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Orange, TEXT("⚠️ 【抗蚀器已报废】抗蚀器耐久归零，无法在后续战斗中阻隔侵蚀与减伤，请前往工坊制造新装备！"));
	}
}

void ULSCorrosionComponent::EquipAntiCorrosionGear(ELSExtractionRarity InRarity)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	EquippedAntiCorrosionGear = FLSAntiCorrosionGearData::GetStandardConfig(InRarity);
	CurrentMaskDurability = EquippedAntiCorrosionGear.MaxDurability;
	OnCorrosionUpdated.Broadcast(CurrentCorrosion, MaxCorrosion, CurrentMaskDurability);

	GEngine->AddOnScreenDebugMessage(-1, 3.5f, FColor::Green, FString::Printf(TEXT("🛡️ 【抗蚀器已装配】品级: %d | 耐久: %d/%d | 侵蚀减免: %.0f%% | 伤害减免: %.0f%%"),
		static_cast<int32>(InRarity), CurrentMaskDurability, EquippedAntiCorrosionGear.MaxDurability,
		EquippedAntiCorrosionGear.CorrosionMitigation * 100.0f, EquippedAntiCorrosionGear.DamageMitigation * 100.0f));
}

void ULSCorrosionComponent::EquipNewMask(int32 InDurability)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || InDurability <= 0) return;

	EquippedAntiCorrosionGear.MaxDurability = InDurability;
	CurrentMaskDurability = InDurability;
	OnCorrosionUpdated.Broadcast(CurrentCorrosion, MaxCorrosion, CurrentMaskDurability);

	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, FString::Printf(TEXT("🛡️ 【面罩就绪】已装配面罩，耐久点数: %d / %d"), CurrentMaskDurability, InDurability));
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

	const float DamageToApply = HealthComp->GetMaxHealth() * OverloadDamagePercent;
	HealthComp->TakeDamage(DamageToApply, LSTags::TAG_Damage_Type_DoT, ActiveChar, ActiveChar->GetController());

	GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Red, FString::Printf(TEXT("☣️ 【侵蚀真实伤害】机体承受侵蚀流失: -%.0f HP"), DamageToApply));
}

float ULSCorrosionComponent::GetVignetteIntensity() const
{
	if (CurrentCorrosion < 60.0f)
	{
		return 0.0f;
	}

	if (!bIsOverloaded && CurrentCorrosion < MaxCorrosion)
	{
		const float Alpha = (CurrentCorrosion - 60.0f) / 40.0f;
		return FMath::Clamp(Alpha * 0.5f, 0.0f, 0.5f);
	}

	if (UWorld* World = GetWorld())
	{
		const float Pulse = (FMath::Sin(World->GetTimeSeconds() * 5.0f) + 1.0f) * 0.5f;
		return 0.65f + (Pulse * 0.30f);
	}

	return 0.75f;
}

void ULSCorrosionComponent::OnRep_CurrentCorrosion()
{
	OnCorrosionUpdated.Broadcast(CurrentCorrosion, MaxCorrosion, CurrentMaskDurability);
}

void ULSCorrosionComponent::OnRep_MaskDurability()
{
	OnCorrosionUpdated.Broadcast(CurrentCorrosion, MaxCorrosion, CurrentMaskDurability);
}

void ULSCorrosionComponent::OnRep_IsOverloaded()
{
	OnCorrosionOverloadChanged.Broadcast(bIsOverloaded);
}