#include "Equipment/ULSDriveDiscDataAsset.h"
#include "Core/LSTypes.h"

ULSDriveDiscDataAsset::ULSDriveDiscDataAsset()
{
}

FPrimaryAssetId ULSDriveDiscDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("DriveSet"), SetTag.GetTagName());
}

namespace
{
	// 随机从低档、中档、高档、满档中抽取数值
	float RollSubStatValue(ELSDriveStatType StatType)
	{
		// 随机抽取 0~3 档位
		const int32 Tier = FMath::RandRange(0, 3);

		switch (StatType)
		{
			// 暴击率：2.7% / 3.1% / 3.5% / 3.9%
		case ELSDriveStatType::CritRate:
			{
				static const float Tiers[] = { 0.027f, 0.031f, 0.035f, 0.039f };
				return Tiers[Tier];
			}
		// 暴击伤害：5.4% / 6.2% / 7.0% / 7.8%
		case ELSDriveStatType::CritDamage:
			{
				static const float Tiers[] = { 0.054f, 0.062f, 0.070f, 0.078f };
				return Tiers[Tier];
			}
		// 百分比大攻击：4.1% / 4.7% / 5.3% / 5.8%
		case ELSDriveStatType::AttackPercent:
			{
				static const float Tiers[] = { 0.041f, 0.047f, 0.053f, 0.058f };
				return Tiers[Tier];
			}
		// 百分比大生命：4.1% / 4.7% / 5.3% / 5.8%
		case ELSDriveStatType::MaxHPPercent:
			{
				static const float Tiers[] = { 0.041f, 0.047f, 0.053f, 0.058f };
				return Tiers[Tier];
			}
		// 百分比大防御：5.1% / 5.8% / 6.6% / 7.3%
		case ELSDriveStatType::DefensePercent:
			{
				static const float Tiers[] = { 0.051f, 0.058f, 0.066f, 0.073f };
				return Tiers[Tier];
			}
		// 元素精通：16 / 19 / 21 / 23
		case ELSDriveStatType::ElementalMastery:
			{
				static const float Tiers[] = { 16.0f, 19.0f, 21.0f, 23.0f };
				return Tiers[Tier];
			}
		// 充能效率：4.5% / 5.2% / 5.8% / 6.5%
		case ELSDriveStatType::EnergyRecharge:
			{
				static const float Tiers[] = { 0.045f, 0.052f, 0.058f, 0.065f };
				return Tiers[Tier];
			}
		// 固定小攻击：14 / 16 / 18 / 19
		case ELSDriveStatType::FlatAttack:
			{
				static const float Tiers[] = { 14.0f, 16.0f, 18.0f, 19.0f };
				return Tiers[Tier];
			}
		// 固定小生命：209 / 239 / 269 / 299
		case ELSDriveStatType::FlatHP:
			{
				static const float Tiers[] = { 209.0f, 239.0f, 269.0f, 299.0f };
				return Tiers[Tier];
			}
		// 固定小防御：16 / 19 / 21 / 23
		case ELSDriveStatType::FlatDefense:
			{
				static const float Tiers[] = { 16.0f, 19.0f, 21.0f, 23.0f };
				return Tiers[Tier];
			}
		default:
			return 0.03f;
		}
	}
}

FLSDriveDisc ULSDriveDiscDataAsset::GenerateDisc(ULSDriveDiscDataAsset* SetAsset, ELSDriveDiscSlot Slot, ELSDriveDiscRarity Rarity)
{
	FLSDriveDisc NewDisc;
	NewDisc.DiscID = FGuid::NewGuid();
	NewDisc.Slot = Slot;
	NewDisc.Rarity = Rarity;
	NewDisc.Level = 0;

	if (SetAsset)
	{
		NewDisc.SetTag = SetAsset->SetTag;
		NewDisc.DiscName = FText::Format(FText::FromString(TEXT("{0} · [{1}号位]")), SetAsset->SetName, FText::AsNumber(static_cast<int32>(Slot) + 1));
	}
	else
	{
		NewDisc.DiscName = FText::FromString(TEXT("标准驱动盘"));
	}

	// 1. 生成槽位主词条
	ELSDriveStatType SelectedMainStat = ELSDriveStatType::AttackPercent;
	float InitialMainValue = 0.0583f; // 默认大攻击 5.83%

	switch (Slot)
	{
	case ELSDriveDiscSlot::Slot1_Foundation:
		// 1 号位默认给大护盾或大生命
		SelectedMainStat = ELSDriveStatType::FlatShield;
		InitialMainValue = 200.0f; // 0级200护盾，满级+20到1000
		break;
	case ELSDriveDiscSlot::Slot2_Power:
		SelectedMainStat = ELSDriveStatType::AttackPercent;
		InitialMainValue = 0.0583f;
		break;
	case ELSDriveDiscSlot::Slot3_Armor:
		SelectedMainStat = ELSDriveStatType::DefensePercent;
		InitialMainValue = 0.0729f;
		break;
	case ELSDriveDiscSlot::Slot4_Critical:
		SelectedMainStat = ELSDriveStatType::CritRate;
		InitialMainValue = 0.0389f;
		break;
	case ELSDriveDiscSlot::Slot5_Elemental:
		SelectedMainStat = ELSDriveStatType::AllElementalDamageBonus;
		InitialMainValue = 0.0583f;
		break;
	case ELSDriveDiscSlot::Slot6_Efficiency:
		SelectedMainStat = ELSDriveStatType::EnergyRecharge;
		InitialMainValue = 0.0648f;
		break;
	}

	NewDisc.MainStat = FLSDriveDiscStat(SelectedMainStat, InitialMainValue);

	// 2. 根据品质决定初始副词条条数（标准1条、特化1~2条、精密2~3条、绝密3~4条）
	int32 InitialSubCount = 1;
	switch (Rarity)
	{
	case ELSDriveDiscRarity::Standard:    // 绿 T1 标准: 初始固定 1 条
		InitialSubCount = 1;
		break;
	case ELSDriveDiscRarity::Specialized: // 蓝 T2 特化: 初始 1 ~ 2 条
		InitialSubCount = FMath::RandRange(1, 2);
		break;
	case ELSDriveDiscRarity::Precision:   // 金 T3 精密: 初始 2 ~ 3 条
		InitialSubCount = FMath::RandRange(2, 3);
		break;
	case ELSDriveDiscRarity::Classified:  // 红 T4 绝密: 初始 3 ~ 4 条
		InitialSubCount = FMath::RandRange(3, 4);
		break;
	}

	// 经典 10 种副词条备选池
	const TArray<ELSDriveStatType> CandidateStats = {
		ELSDriveStatType::CritRate,         // 暴击率
		ELSDriveStatType::CritDamage,       // 暴击伤害
		ELSDriveStatType::AttackPercent,    // 百分比大攻击
		ELSDriveStatType::MaxHPPercent,     // 百分比大生命
		ELSDriveStatType::DefensePercent,   // 百分比大防御
		ELSDriveStatType::ElementalMastery, // 元素精通
		ELSDriveStatType::EnergyRecharge,   // 元素充能
		ELSDriveStatType::FlatAttack,       // 固定小攻击
		ELSDriveStatType::FlatHP,           // 固定小生命
		ELSDriveStatType::FlatDefense       // 固定小防御
	};

	TArray<ELSDriveStatType> AvailableStats;
	for (ELSDriveStatType St : CandidateStats)
	{
		// 互斥原则：副词条绝不出与主词条完全相同的属性
		if (St != SelectedMainStat)
		{
			AvailableStats.Add(St);
		}
	}

	// 随机抽取并赋予四档浮动初值
	for (int32 i = 0; i < InitialSubCount && AvailableStats.Num() > 0; ++i)
	{
		const int32 PickIndex = FMath::RandRange(0, AvailableStats.Num() - 1);
		const ELSDriveStatType PickedStat = AvailableStats[PickIndex];
		AvailableStats.RemoveAt(PickIndex);

		// 使用四档浮动抽取数值
		const float SubVal = RollSubStatValue(PickedStat);
		NewDisc.SubStats.Add(FLSDriveDiscStat(PickedStat, SubVal));
	}
	
	return NewDisc;
}