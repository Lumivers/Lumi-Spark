#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Extraction/LSExtractionTypes.h"
#include "ULSBackpackComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLSOnInventoryChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLSOnItemSecured, const FLSInventoryItem&, SecuredItem);

// 背包与安全箱组件 (ULSBackpackComponent)
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class LUMI_SPARK_API ULSBackpackComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:
	ULSBackpackComponent();
	
	// 容量与等阶配置
	
	//初始默认15格背包
	static constexpr int32 InitialCapacity = 15;
	
	// 当前背包阶位
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Backpack|Capacity")
	ELSBackpackTier CurrentTier = ELSBackpackTier::Default;
	
	// 当前背包容量（格数）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Backpack|Capacity")
	int32 BackpackCapacity = InitialCapacity;
	
	// 安全箱容量（默认1格，随游戏进展最高可5格）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Backpack|Capacity")
	int32 SecureBoxCapacity = 1;
	
	// 槽位容器
	
	// 普通背包槽位列表（动态扩容，按 CurrentTier 决定最大容量）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Backpack|Slots")
	TArray<FLSInventoryItem> BackpackSlots;
	
	// 安全箱槽位列表（固定容量，按 SecureBoxCapacity 决定最大容量）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Backpack|Slots")
	TArray<FLSInventoryItem> SecureBoxSlots;
	
	// 委托广播
	UPROPERTY(BlueprintAssignable, Category = "Backpack|Events")
	FLSOnInventoryChanged OnInventoryChanged;
	
	UPROPERTY(BlueprintAssignable, Category = "Backpack|Events")
	FLSOnItemSecured OnItemSecured;
	
	// 物品存取与管理

	// 添加物品至背包
	UFUNCTION(BlueprintCallable, Category = "Backpack|Action")
	bool AddItem(const FLSInventoryItem& NewItem, bool bAutoSecureRedItems = true);
	
	// 从背包移入安全箱
	UFUNCTION(BlueprintCallable, Category = "Backpack|Action")
	bool MoveToSecureBox(int32 BackpackIndex, int32 SecureSlotIndex = -1);
	
	// 从安全箱移回背包
	UFUNCTION(BlueprintCallable, Category = "Backpack|Action")
	bool MoveToBackpack(int32 SecureSlotIndex, int32 BackpackIndex = -1);
	
	// 移除或消耗物品
	UFUNCTION(BlueprintCallable, Category = "Backpack|Action")
	bool RemoveFromBackpack(int32 BackpackIndex, int32 Quantity = 1);
	
	UFUNCTION(BlueprintCallable, Category = "Backpack|Action")
	bool RemoveFromSecureBox(int32 SecureSlotIndex, int32 Quantity = 1);
	
	// 阶位升级扩容
	
	// 升级背包阶位，扩展容量
	UFUNCTION(BlueprintCallable, Category = "Backpack|Upgrade")
	int32 UpgradeBackpackTier();
	
	// 安全箱扩容
	UFUNCTION(BlueprintCallable, Category = "Backpack|Upgrade")
	void ExpandSecureBox(int32 AdditionalSlots);
	
	// 查询与结算
	UFUNCTION(BlueprintPure, Category = "Backpack|Query")
	int32 GetTotalMaterialCount() const;
	
	UFUNCTION(BlueprintPure, Category = "Backpack|Query")
	float CalculateTotalLootValue() const;
	
	// 死亡结算
	UFUNCTION(BlueprintCallable, Category = "Backpack|Extraction")
	void ProcessDeathDrop(float RetentionRate, TArray<FLSInventoryItem>& OutDroppedLoots, TArray<FLSInventoryItem>& OutRetainedLoots);
	
	UFUNCTION(BlueprintCallable, Category = "Backpack|Extraction")
	void ClearBackpack();
	
protected:
	virtual void BeginPlay() override;
	
private:
	bool InternalAddToSlotArray(TArray<FLSInventoryItem>& SlotArray, int32 MaxCapacity, const FLSInventoryItem& ItemToAdd, bool bMarkSecured);
};
