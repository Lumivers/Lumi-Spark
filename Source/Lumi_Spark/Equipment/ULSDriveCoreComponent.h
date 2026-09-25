#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Equipment/LSDriveCoreTypes.h"
#include "GameplayTagContainer.h"
#include "Combat/LSDamageCalculator.h"
#include "ULSDriveCoreComponent.generated.h"

class ALSCharacterBase;
class ULSDriveDiscDataAsset;

// ─── 委托声明 ───
// 驱动核心装配变动广播（供未来 UI 面板与角色属性刷新监听）
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLSDriveCoreChanged, const FLSDriveCoreLoadout&, NewLoadout);

/**
 * 全队共享驱动核心中枢组件 (ULSDriveCoreComponent)
 * 挂载于 ALSPlayerController，管理 6 槽装配、4+2套装仲裁与属性汇总
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class LUMI_SPARK_API ULSDriveCoreComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSDriveCoreComponent();

	virtual void BeginPlay() override;

	// ═══ 装备装卸核心接口 ═══

	//在指定槽位装配驱动盘（自动校验槽位合法性，并重新结算套装与属性）
	UFUNCTION(BlueprintCallable, Category = "DriveCore|Equip")
	bool EquipDisc(ELSDriveDiscSlot Slot, const FLSDriveDisc& Disc);

	//卸下指定槽位的驱动盘
	UFUNCTION(BlueprintCallable, Category = "DriveCore|Equip")
	bool UnequipDisc(ELSDriveDiscSlot Slot);

	//获取指定槽位当前装备的驱动盘
	UFUNCTION(BlueprintPure, Category = "DriveCore|Equip")
	FLSDriveDisc GetEquippedDisc(ELSDriveDiscSlot Slot) const;

	//获取当前完整的 6 槽装载方案
	UFUNCTION(BlueprintPure, Category = "DriveCore|Equip")
	const FLSDriveCoreLoadout& GetLoadout() const { return Loadout; }

	// ═══ 套装状态查询 ═══

	//查询某套装当前已装备的有效件数
	UFUNCTION(BlueprintPure, Category = "DriveCore|Set")
	int32 GetSetPieceCount(FGameplayTag SetTag) const;

	//查询某套装的 2 件套是否激活
	UFUNCTION(BlueprintPure, Category = "DriveCore|Set")
	bool IsTwoPieceActive(FGameplayTag SetTag) const;

	//查询某套装的 4 件套是否激活
	UFUNCTION(BlueprintPure, Category = "DriveCore|Set")
	bool IsFourPieceActive(FGameplayTag SetTag) const;

	// ═══ 属性汇总管线（与战斗系统核心咬合） ═══
	
	//为当前在场角色计算全乘区最终战斗属性
	UFUNCTION(BlueprintCallable, Category = "DriveCore|Stats")
	FLSCombatAttributes CalculateCombatAttributes(ALSCharacterBase* TargetCharacter) const;

	// 生成直接喂给 LSDamageCalculator 的攻击者战斗属性包
	UFUNCTION(BlueprintCallable, Category = "DriveCore|Stats")
	FLSAttackerStats BuildAttackerStats(ALSCharacterBase* TargetCharacter) const;

	// ═══ 调试与快捷预设 ═══
	// 一键为全队装配指定流派顶级 4+2 套装（前4槽装SetA，后2槽装SetB）
	UFUNCTION(BlueprintCallable, Category = "DriveCore|Debug")
	void EquipPresetLoadout(FGameplayTag FourPieceSetTag, FGameplayTag TwoPieceSetTag, ELSDriveDiscRarity Rarity = ELSDriveDiscRarity::Classified);

public:
	UPROPERTY(BlueprintAssignable, Category = "DriveCore|Events")
	FOnLSDriveCoreChanged OnDriveCoreChanged;

protected:
	// 当前 6 槽装配方案
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DriveCore|State")
	FLSDriveCoreLoadout Loadout;

	// 套装件数统计表 (SetTag -> 已装备数量)
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "DriveCore|State")
	TMap<FGameplayTag, int32> ActiveSetCounts;

	// 重新评估并仲裁当前激活的 4 件套与 2 件套
	void EvaluateSetBonuses();

	// 内部累加单张驱动盘的词条属性
	void AccumulateDiscStats(const FLSDriveDisc& Disc, FLSCombatAttributes& InOutStats) const;

	// 内部应用激活的套装被动属性
	void ApplySetBonusStats(FLSCombatAttributes& InOutStats) const;
};