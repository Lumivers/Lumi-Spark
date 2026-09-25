#include "Equipment/ULSDriveCoreComponent.h"
#include "Character/LSCharacterBase.h"
#include "Character/LSCharacterDataAsset.h"
#include "Weapon/LSWeaponBase.h"
#include "Weapon/LSWeaponComponent.h"
#include "Core/LSTypes.h"
#include "Character/LSHealthComponent.h"

ULSDriveCoreComponent::ULSDriveCoreComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULSDriveCoreComponent::BeginPlay()
{
	Super::BeginPlay();
	EvaluateSetBonuses();
}

bool ULSDriveCoreComponent::EquipDisc(ELSDriveDiscSlot Slot, const FLSDriveDisc& Disc)
{
	if (!Disc.IsValid()) return false;

	// 保证装入槽位与驱动盘本身归属槽位一致
	FLSDriveDisc NewDisc = Disc;
	NewDisc.Slot = Slot;

	Loadout.SetDiscBySlot(Slot, NewDisc);
	EvaluateSetBonuses();

	OnDriveCoreChanged.Broadcast(Loadout);
	return true;
}

bool ULSDriveCoreComponent::UnequipDisc(ELSDriveDiscSlot Slot)
{
	Loadout.ClearSlot(Slot);
	EvaluateSetBonuses();

	OnDriveCoreChanged.Broadcast(Loadout);
	return true;
}

FLSDriveDisc ULSDriveCoreComponent::GetEquippedDisc(ELSDriveDiscSlot Slot) const
{
	if (const FLSDriveDisc* Disc = Loadout.GetDiscBySlot(Slot))
	{
		return *Disc;
	}
	return FLSDriveDisc();
}

int32 ULSDriveCoreComponent::GetSetPieceCount(FGameplayTag SetTag) const
{
	if (const int32* FoundCount = ActiveSetCounts.Find(SetTag))
	{
		return *FoundCount;
	}
	return 0;
}

bool ULSDriveCoreComponent::IsTwoPieceActive(FGameplayTag SetTag) const
{
	return GetSetPieceCount(SetTag) >= 2;
}

bool ULSDriveCoreComponent::IsFourPieceActive(FGameplayTag SetTag) const
{
	return GetSetPieceCount(SetTag) >= 4;
}

void ULSDriveCoreComponent::EvaluateSetBonuses()
{
	ActiveSetCounts.Empty();

	const TArray<FLSDriveDisc> Discs = Loadout.GetAllValidDiscs();
	for (const FLSDriveDisc& Disc : Discs)
	{
		if (Disc.SetTag.IsValid())
		{
			ActiveSetCounts.FindOrAdd(Disc.SetTag)++;
		}
	}
}

void ULSDriveCoreComponent::AccumulateDiscStats(const FLSDriveDisc& Disc, FLSCombatAttributes& InOutStats) const
{
	if (!Disc.IsValid()) return;

	auto ApplyStat = [&InOutStats](const FLSDriveDiscStat& Stat)
	{
		switch (Stat.StatType)
		{
		case ELSDriveStatType::FlatShield:
			InOutStats.TotalShield += Stat.Value;
			break;
		case ELSDriveStatType::MaxHPPercent:
			// 暂存至 TotalMaxHealth 比例计算
			InOutStats.TotalMaxHealth += Stat.Value;
			break;
		case ELSDriveStatType::AttackPercent:
			InOutStats.TotalAttack += Stat.Value;
			break;
		case ELSDriveStatType::DefensePercent:
			InOutStats.TotalDefense += Stat.Value;
			break;
		case ELSDriveStatType::PenetrationRate:
			InOutStats.PenetrationRate += Stat.Value;
			break;
		case ELSDriveStatType::CritRate:
			InOutStats.TotalCritRate += Stat.Value;
			break;
		case ELSDriveStatType::CritDamage:
			InOutStats.TotalCritDamage += Stat.Value;
			break;
		case ELSDriveStatType::ElementalMastery:
			InOutStats.TotalElementalMastery += Stat.Value;
			break;
		case ELSDriveStatType::AllElementalDamageBonus:
			InOutStats.AllElementalDamageBonus += Stat.Value;
			break;
		case ELSDriveStatType::ReactionDamageBonus:
			InOutStats.ReactionDamageBonus += Stat.Value;
			break;
		case ELSDriveStatType::ElementalResist:
			InOutStats.ElementalResistance += Stat.Value;
			break;
		case ELSDriveStatType::EnergyRecharge:
			InOutStats.EnergyRecharge += Stat.Value;
			break;
		case ELSDriveStatType::ReloadSpeed:
			InOutStats.ReloadSpeedBonus += Stat.Value;
			break;
		case ELSDriveStatType::ShieldStrength:
			InOutStats.ShieldStrength += Stat.Value;
			break;
		default:
			break;
		}
	};

	// 累加主词条
	ApplyStat(Disc.MainStat);

	// 累加副词条
	for (const FLSDriveDiscStat& Sub : Disc.SubStats)
	{
		ApplyStat(Sub);
	}
}

void ULSDriveCoreComponent::ApplySetBonusStats(FLSCombatAttributes& InOutStats) const
{
	// 遍历所有已激活的套装被动属性
	for (const auto& Pair : ActiveSetCounts)
	{
		const FGameplayTag SetTag = Pair.Key;
		const int32 Count = Pair.Value;

		// 2 件套效果
		if (Count >= 2)
		{
			if (SetTag == LSTags::TAG_DriveSet_TacticalSwap)
			{
				InOutStats.SkillCooldownReduction += 0.12f; // 战技CDR +12%
			}
			else if (SetTag == LSTags::TAG_DriveSet_PrecisionMarksman)
			{
				InOutStats.TotalCritDamage += 0.20f; // 暴伤 +20%
			}
			else if (SetTag == LSTags::TAG_DriveSet_ElementalResonance)
			{
				InOutStats.TotalElementalMastery += 80.0f; // 精通 +80
			}
			else if (SetTag == LSTags::TAG_DriveSet_HeavyBastion)
			{
				InOutStats.TotalShield += 400.0f; // 基础护盾 +400
				InOutStats.ShieldStrength += 0.20f; // 护盾强效 +20%
			}
		}

		// 4 件套效果（数值常驻部分）
		if (Count >= 4)
		{
			if (SetTag == LSTags::TAG_DriveSet_ElementalResonance)
			{
				InOutStats.ReactionDamageBonus += 0.30f; // 反应增伤 +30%（非独立乘区）
			}
			else if (SetTag == LSTags::TAG_DriveSet_TacticalSwap)
			{
				InOutStats.ReloadSpeedBonus += 0.20f; // 战术突入换弹提速
			}
		}
	}
}

FLSCombatAttributes ULSDriveCoreComponent::CalculateCombatAttributes(ALSCharacterBase* TargetCharacter) const
{
	FLSCombatAttributes Result;

	// 1. 基础白值（Base Stats）
	float CharBaseHP = 1000.0f;
	float CharBaseDEF = 100.0f;
	float BaseATK = 150.0f; // 默认基础攻击

	if (TargetCharacter)
	{
		// 直接从角色已有的独立生命组件读取基础血量上限
		if (const ULSHealthComponent* HealthComp = TargetCharacter->GetHealthComponent())
		{
			CharBaseHP = HealthComp->GetMaxHealth();
		}
		// 加上当前手持武器的基础伤害白值
		if (const ULSWeaponComponent* WeaponComp = TargetCharacter->GetWeaponComponent())
		{
			if (const ALSWeaponBase* CurWeapon = WeaponComp->GetCurrentWeapon())
			{
				BaseATK += CurWeapon->GetBaseDamage();
			}
		}
	}

	// 2. 统计驱动盘上的百分比加成与固定加成
	FLSCombatAttributes AggregatedBonuses;
	AggregatedBonuses.TotalAttack = 0.0f;      // 用作大攻击百分比累加
	AggregatedBonuses.TotalMaxHealth = 0.0f;   // 用作大生命百分比累加
	AggregatedBonuses.TotalDefense = 0.0f;     // 用作大防御百分比累加
	AggregatedBonuses.TotalShield = 0.0f;      // 固定白盾
	AggregatedBonuses.TotalCritRate = 0.05f;   // 基础暴击 5%
	AggregatedBonuses.TotalCritDamage = 0.50f; // 基础暴伤 50%
	AggregatedBonuses.TotalElementalMastery = 0.0f;
	AggregatedBonuses.PenetrationRate = 0.0f;
	AggregatedBonuses.AllElementalDamageBonus = 0.0f;
	AggregatedBonuses.ReactionDamageBonus = 0.0f;
	AggregatedBonuses.ElementalResistance = 0.10f; // 基础全抗性 10%
	AggregatedBonuses.EnergyRecharge = 1.0f;
	AggregatedBonuses.ReloadSpeedBonus = 0.0f;
	AggregatedBonuses.SkillCooldownReduction = 0.0f;
	AggregatedBonuses.ShieldStrength = 0.0f;

	const TArray<FLSDriveDisc> Discs = Loadout.GetAllValidDiscs();
	for (const FLSDriveDisc& Disc : Discs)
	{
		AccumulateDiscStats(Disc, AggregatedBonuses);
	}

	// 3. 计入套装被动数值
	ApplySetBonusStats(AggregatedBonuses);

	// 4. 正式结算最终全乘区属性（严格先百分比后小属性）
	Result.TotalAttack = BaseATK * (1.0f + AggregatedBonuses.TotalAttack);
	Result.TotalMaxHealth = CharBaseHP * (1.0f + AggregatedBonuses.TotalMaxHealth);
	Result.TotalDefense = CharBaseDEF * (1.0f + AggregatedBonuses.TotalDefense);
	Result.TotalShield = AggregatedBonuses.TotalShield;

	Result.TotalCritRate = FMath::Clamp(AggregatedBonuses.TotalCritRate, 0.0f, 1.0f);
	Result.TotalCritDamage = FMath::Max(0.50f, AggregatedBonuses.TotalCritDamage);
	Result.TotalElementalMastery = FMath::Max(0.0f, AggregatedBonuses.TotalElementalMastery);
	Result.PenetrationRate = FMath::Clamp(AggregatedBonuses.PenetrationRate, 0.0f, 1.0f);
	Result.AllElementalDamageBonus = AggregatedBonuses.AllElementalDamageBonus;
	Result.ReactionDamageBonus = AggregatedBonuses.ReactionDamageBonus;
	Result.ElementalResistance = FMath::Clamp(AggregatedBonuses.ElementalResistance, 0.0f, 0.90f);
	Result.EnergyRecharge = FMath::Max(1.0f, AggregatedBonuses.EnergyRecharge);
	Result.ReloadSpeedBonus = FMath::Clamp(AggregatedBonuses.ReloadSpeedBonus, 0.0f, 0.80f);
	Result.SkillCooldownReduction = FMath::Clamp(AggregatedBonuses.SkillCooldownReduction, 0.0f, 0.50f);
	Result.ShieldStrength = AggregatedBonuses.ShieldStrength;

	return Result;
}

FLSAttackerStats ULSDriveCoreComponent::BuildAttackerStats(ALSCharacterBase* TargetCharacter) const
{
	const FLSCombatAttributes Attr = CalculateCombatAttributes(TargetCharacter);

	FLSAttackerStats Stats;
	Stats.Level = 90; // 默认等级
	Stats.Attack = Attr.TotalAttack;
	Stats.CritRate = Attr.TotalCritRate;
	Stats.CritDamage = Attr.TotalCritDamage;
	Stats.ElementalMastery = Attr.TotalElementalMastery;
	Stats.DamageBonus = Attr.AllElementalDamageBonus;
	Stats.DefIgnoreRate = Attr.PenetrationRate;

	return Stats;
}

void ULSDriveCoreComponent::EquipPresetLoadout(FGameplayTag FourPieceSetTag, FGameplayTag TwoPieceSetTag, ELSDriveDiscRarity Rarity)
{
	const ELSDriveDiscSlot Slots[] = {
		ELSDriveDiscSlot::Slot1_Foundation,
		ELSDriveDiscSlot::Slot2_Power,
		ELSDriveDiscSlot::Slot3_Armor,
		ELSDriveDiscSlot::Slot4_Critical,
		ELSDriveDiscSlot::Slot5_Elemental,
		ELSDriveDiscSlot::Slot6_Efficiency
	};

	// 1~4 号位装 4 件套
	for (int32 i = 0; i < 4; ++i)
	{
		FLSDriveDisc Disc;
		Disc.DiscID = FGuid::NewGuid();
		Disc.Slot = Slots[i];
		Disc.Rarity = Rarity;
		Disc.Level = GetMaxLevelForRarity(Rarity);
		Disc.SetTag = FourPieceSetTag;

		// 默认主词条配置
		switch (Slots[i])
		{
		case ELSDriveDiscSlot::Slot1_Foundation:
			Disc.MainStat = FLSDriveDiscStat(ELSDriveStatType::FlatShield, 1000.0f);
			break;
		case ELSDriveDiscSlot::Slot2_Power:
			Disc.MainStat = FLSDriveDiscStat(ELSDriveStatType::AttackPercent, 0.466f);
			break;
		case ELSDriveDiscSlot::Slot3_Armor:
			Disc.MainStat = FLSDriveDiscStat(ELSDriveStatType::DefensePercent, 0.583f);
			break;
		case ELSDriveDiscSlot::Slot4_Critical:
			Disc.MainStat = FLSDriveDiscStat(ELSDriveStatType::CritRate, 0.311f);
			break;
		default: break;
		}

		// 附赠两条极品副词条
		Disc.SubStats.Add(FLSDriveDiscStat(ELSDriveStatType::CritDamage, 0.155f));
		Disc.SubStats.Add(FLSDriveDiscStat(ELSDriveStatType::AttackPercent, 0.082f));

		EquipDisc(Slots[i], Disc);
	}

	// 5~6 号位装 2 件套
	for (int32 i = 4; i < 6; ++i)
	{
		FLSDriveDisc Disc;
		Disc.DiscID = FGuid::NewGuid();
		Disc.Slot = Slots[i];
		Disc.Rarity = Rarity;
		Disc.Level = GetMaxLevelForRarity(Rarity);
		Disc.SetTag = TwoPieceSetTag;

		if (Slots[i] == ELSDriveDiscSlot::Slot5_Elemental)
		{
			Disc.MainStat = FLSDriveDiscStat(ELSDriveStatType::AllElementalDamageBonus, 0.30f);
		}
		else
		{
			Disc.MainStat = FLSDriveDiscStat(ELSDriveStatType::EnergyRecharge, 0.518f);
		}

		Disc.SubStats.Add(FLSDriveDiscStat(ELSDriveStatType::CritRate, 0.078f));
		Disc.SubStats.Add(FLSDriveDiscStat(ELSDriveStatType::ElementalMastery, 42.0f));

		EquipDisc(Slots[i], Disc);
	}
}