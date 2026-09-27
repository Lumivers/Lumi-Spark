#include "Extraction/ULSBackpackComponent.h"

ULSBackpackComponent::ULSBackpackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	BackpackCapacity = InitialCapacity;
}

void ULSBackpackComponent::BeginPlay()
{
	Super::BeginPlay();

	// 预分配槽位（保持一维定长数组，方便与 UI 网格映射）
	if (BackpackSlots.Num() < BackpackCapacity)
	{
		BackpackSlots.SetNum(BackpackCapacity);
	}
	if (SecureBoxSlots.Num() < SecureBoxCapacity)
	{
		SecureBoxSlots.SetNum(SecureBoxCapacity);
	}
}

int32 ULSBackpackComponent::UpgradeBackpackTier()
{
	switch (CurrentTier)
	{
	case ELSBackpackTier::Default:
		// 初始 15 格 -> 升绿 +15 = 30 格
		CurrentTier = ELSBackpackTier::StandardTier;
		BackpackCapacity = 30;
		break;
	case ELSBackpackTier::StandardTier:
		// 绿 30 格 -> 升蓝 +10 = 40 格
		CurrentTier = ELSBackpackTier::SpecializedTier;
		BackpackCapacity = 40;
		break;
	case ELSBackpackTier::SpecializedTier:
		// 蓝 40 格 -> 升金 +10 = 50 格
		CurrentTier = ELSBackpackTier::PrecisionTier;
		BackpackCapacity = 50;
		break;
	case ELSBackpackTier::PrecisionTier:
		// 金 50 格 -> 升红 +10 = 60 格
		CurrentTier = ELSBackpackTier::ClassifiedTier;
		BackpackCapacity = 60;
		break;
	case ELSBackpackTier::ClassifiedTier:
		// 已满级
		return BackpackCapacity;
	}

	BackpackSlots.SetNum(BackpackCapacity);
	OnInventoryChanged.Broadcast();

	UE_LOG(LogTemp, Display, TEXT("[ULSBackpackComponent] 战备背包升级成功！当前阶位: %d, 新容量: %d 格"), static_cast<int32>(CurrentTier), BackpackCapacity);
	return BackpackCapacity;
}

void ULSBackpackComponent::ExpandSecureBox(int32 AdditionalSlots)
{
	SecureBoxCapacity = FMath::Clamp(SecureBoxCapacity + AdditionalSlots, 4, 6);
	SecureBoxSlots.SetNum(SecureBoxCapacity);
	OnInventoryChanged.Broadcast();
}

bool ULSBackpackComponent::AddItem(const FLSInventoryItem& NewItem, bool bAutoSecureRedItems)
{
	if (!NewItem.IsValid()) return false;

	// 大红物品优先塞进安全箱
	const bool bIsClassified = (NewItem.Rarity == ELSExtractionRarity::Classified);
	if (bAutoSecureRedItems && bIsClassified)
	{
		if (InternalAddToSlotArray(SecureBoxSlots, SecureBoxCapacity, NewItem, true))
		{
			OnItemSecured.Broadcast(NewItem);
			OnInventoryChanged.Broadcast();
			return true;
		}
		UE_LOG(LogTemp, Warning, TEXT("[ULSBackpackComponent] ⚠️ 安全箱已满，顶级物品 [%s] 被移入普通背包！"), *NewItem.DisplayName.ToString());
	}

	// 存入普通背包
	const bool bSuccess = InternalAddToSlotArray(BackpackSlots, BackpackCapacity, NewItem, false);
	if (bSuccess)
	{
		OnInventoryChanged.Broadcast();
	}
	return bSuccess;
}

bool ULSBackpackComponent::InternalAddToSlotArray(TArray<FLSInventoryItem>& SlotArray, int32 MaxCap, const FLSInventoryItem& ItemToAdd, bool bMarkSecured)
{
	if (SlotArray.Num() < MaxCap)
	{
		SlotArray.SetNum(MaxCap);
	}

	int32 RemainingQty = ItemToAdd.Quantity;

	// 1. 如果物品允许堆叠（MaxStack > 1），先找同类未满格合并
	if (ItemToAdd.MaxStack > 1)
	{
		for (int32 i = 0; i < SlotArray.Num(); ++i)
		{
			FLSInventoryItem& Slot = SlotArray[i];
			if (Slot.IsValid() && Slot.ItemID == ItemToAdd.ItemID && Slot.Quantity < Slot.MaxStack)
			{
				const int32 Space = Slot.MaxStack - Slot.Quantity;
				const int32 MergeCount = FMath::Min(Space, RemainingQty);
				Slot.Quantity += MergeCount;
				RemainingQty -= MergeCount;

				if (RemainingQty <= 0) return true;
			}
		}
	}

	// 2. 将剩余数量放入第一个空槽
	for (int32 i = 0; i < SlotArray.Num(); ++i)
	{
		FLSInventoryItem& Slot = SlotArray[i];
		if (!Slot.IsValid())
		{
			Slot = ItemToAdd;
			Slot.Quantity = RemainingQty;
			Slot.bIsSecured = bMarkSecured;
			return true;
		}
	}

	return false;
}

bool ULSBackpackComponent::MoveToSecureBox(int32 BackpackSlotIndex, int32 SecureSlotIndex)
{
	if (!BackpackSlots.IsValidIndex(BackpackSlotIndex) || !BackpackSlots[BackpackSlotIndex].IsValid()) return false;

	FLSInventoryItem ItemToMove = BackpackSlots[BackpackSlotIndex];

	if (SecureSlotIndex >= 0 && SecureSlotIndex < SecureBoxSlots.Num())
	{
		if (!SecureBoxSlots[SecureSlotIndex].IsValid())
		{
			ItemToMove.bIsSecured = true;
			SecureBoxSlots[SecureSlotIndex] = ItemToMove;
			BackpackSlots[BackpackSlotIndex] = FLSInventoryItem();
			OnInventoryChanged.Broadcast();
			return true;
		}
		return false;
	}

	if (InternalAddToSlotArray(SecureBoxSlots, SecureBoxCapacity, ItemToMove, true))
	{
		BackpackSlots[BackpackSlotIndex] = FLSInventoryItem();
		OnInventoryChanged.Broadcast();
		return true;
	}

	return false;
}

bool ULSBackpackComponent::MoveToBackpack(int32 SecureSlotIndex, int32 BackpackSlotIndex)
{
	if (!SecureBoxSlots.IsValidIndex(SecureSlotIndex) || !SecureBoxSlots[SecureSlotIndex].IsValid()) return false;

	FLSInventoryItem ItemToMove = SecureBoxSlots[SecureSlotIndex];

	if (BackpackSlotIndex >= 0 && BackpackSlotIndex < BackpackSlots.Num())
	{
		if (!BackpackSlots[BackpackSlotIndex].IsValid())
		{
			ItemToMove.bIsSecured = false;
			BackpackSlots[BackpackSlotIndex] = ItemToMove;
			SecureBoxSlots[SecureSlotIndex] = FLSInventoryItem();
			OnInventoryChanged.Broadcast();
			return true;
		}
		return false;
	}

	if (InternalAddToSlotArray(BackpackSlots, BackpackCapacity, ItemToMove, false))
	{
		SecureBoxSlots[SecureSlotIndex] = FLSInventoryItem();
		OnInventoryChanged.Broadcast();
		return true;
	}

	return false;
}

bool ULSBackpackComponent::RemoveFromBackpack(int32 SlotIndex, int32 Quantity)
{
	if (!BackpackSlots.IsValidIndex(SlotIndex) || !BackpackSlots[SlotIndex].IsValid()) return false;
	FLSInventoryItem& Slot = BackpackSlots[SlotIndex];
	Slot.Quantity -= Quantity;
	if (Slot.Quantity <= 0) Slot = FLSInventoryItem();
	OnInventoryChanged.Broadcast();
	return true;
}

bool ULSBackpackComponent::RemoveFromSecureBox(int32 SlotIndex, int32 Quantity)
{
	if (!SecureBoxSlots.IsValidIndex(SlotIndex) || !SecureBoxSlots[SlotIndex].IsValid()) return false;
	FLSInventoryItem& Slot = SecureBoxSlots[SlotIndex];
	Slot.Quantity -= Quantity;
	if (Slot.Quantity <= 0) Slot = FLSInventoryItem();
	OnInventoryChanged.Broadcast();
	return true;
}

int32 ULSBackpackComponent::GetTotalMaterialCount() const
{
	int32 Total = 0;
	auto CountFn = [&Total](const TArray<FLSInventoryItem>& Slots)
	{
		for (const FLSInventoryItem& Item : Slots)
		{
			if (Item.IsValid() && Item.ItemType == ELSExtractionItemType::Material)
			{
				Total += Item.Quantity;
			}
		}
	};
	CountFn(BackpackSlots);
	CountFn(SecureBoxSlots);
	return Total;
}

float ULSBackpackComponent::CalculateTotalLootValue() const
{
	float Total = 0.0f;
	auto SumFn = [&Total](const TArray<FLSInventoryItem>& Slots)
	{
		for (const FLSInventoryItem& Item : Slots)
		{
			if (Item.IsValid()) Total += (Item.UnitValue * Item.Quantity);
		}
	};
	SumFn(BackpackSlots);
	SumFn(SecureBoxSlots);
	return Total;
}

void ULSBackpackComponent::ProcessDeathDrop(float RetentionRate, TArray<FLSInventoryItem>& OutDroppedLoot, TArray<FLSInventoryItem>& OutRetainedLoot)
{
	OutDroppedLoot.Reset();
	OutRetainedLoot.Reset();

	// 1. 安全箱物品
	for (const FLSInventoryItem& Item : SecureBoxSlots)
	{
		if (Item.IsValid()) OutRetainedLoot.Add(Item);
	}

	// 2. 普通背包物品：按保留率掉落
	RetentionRate = FMath::Clamp(RetentionRate, 0.0f, 1.0f);
	for (int32 i = 0; i < BackpackSlots.Num(); ++i)
	{
		FLSInventoryItem& Slot = BackpackSlots[i];
		if (!Slot.IsValid()) continue;

		if (RetentionRate <= 0.0f)
		{
			OutDroppedLoot.Add(Slot);
			Slot = FLSInventoryItem();
		}
		else
		{
			const int32 KeepQty = FMath::RoundToInt(Slot.Quantity * RetentionRate);
			const int32 DropQty = Slot.Quantity - KeepQty;

			if (DropQty > 0)
			{
				FLSInventoryItem Dropped = Slot;
				Dropped.Quantity = DropQty;
				OutDroppedLoot.Add(Dropped);
			}

			if (KeepQty > 0)
			{
				FLSInventoryItem Kept = Slot;
				Kept.Quantity = KeepQty;
				OutRetainedLoot.Add(Kept);
				Slot.Quantity = KeepQty;
			}
			else
			{
				Slot = FLSInventoryItem();
			}
		}
	}

	OnInventoryChanged.Broadcast();
}

void ULSBackpackComponent::ClearBackpack()
{
	for (FLSInventoryItem& Slot : BackpackSlots)
	{
		Slot = FLSInventoryItem();
	}
	OnInventoryChanged.Broadcast();
}

bool ULSBackpackComponent::LearnBlueprint(FName BlueprintItemID)
{
	// 在普通背包中查找该图纸
	for (int32 i = 0; i < BackpackSlots.Num(); ++i)
	{
		const FLSInventoryItem& Item = BackpackSlots[i];
		if (Item.bIsBlueprint && Item.ItemID == BlueprintItemID)
		{
			LearnedBlueprints.Add(BlueprintItemID);
			// 学习后图纸作为道具消耗掉
			RemoveFromBackpack(i, 1);
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, FString::Printf(TEXT("📘 【图纸学习成功】已掌握配方：%s！可在工厂随时制造。"), *BlueprintItemID.ToString()));
			return true;
		}
	}
	GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("❌ 背包中未找到该图纸！"));
	return false;
}

bool ULSBackpackComponent::CraftItemFromRecipe(const FLSCraftingRecipe& Recipe)
{
	// 1. 图纸解锁校验
	if (Recipe.bRequiresBlueprint && !LearnedBlueprints.Contains(Recipe.RequiredBlueprintID))
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Red, FString::Printf(TEXT("❌ 制造失败：尚未学会图纸【%s】！"), *Recipe.RequiredBlueprintID.ToString()));
		return false;
	}

	// 2. 原材料数量预检
	for (const FLSItemIngredient& Ingredient : Recipe.Ingredients)
	{
		int32 FoundCount = 0;
		for (const FLSInventoryItem& Item : BackpackSlots)
		{
			if (Item.ItemID == Ingredient.MaterialID)
			{
				FoundCount += Item.Quantity;
			}
		}
		if (FoundCount < Ingredient.Count)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Red, FString::Printf(TEXT("❌ 制造失败：缺少材料 %s (%d / %d)"), *Ingredient.MaterialID.ToString(), FoundCount, Ingredient.Count));
			return false;
		}
	}

	// 3. 扣除材料（逆序遍历安全扣除）
	for (const FLSItemIngredient& Ingredient : Recipe.Ingredients)
	{
		int32 RemainingToRemove = Ingredient.Count;
		for (int32 i = BackpackSlots.Num() - 1; i >= 0 && RemainingToRemove > 0; --i)
		{
			if (BackpackSlots[i].ItemID == Ingredient.MaterialID)
			{
				const int32 Deducted = FMath::Min(BackpackSlots[i].Quantity, RemainingToRemove);
				BackpackSlots[i].Quantity -= Deducted;
				RemainingToRemove -= Deducted;
				if (BackpackSlots[i].Quantity <= 0)
				{
					BackpackSlots.RemoveAt(i);
				}
			}
		}
	}

	// 4. 产出全新满耐久装备并入包
	FLSInventoryItem NewItem = Recipe.ResultItem;
	NewItem.CurrentDurability = NewItem.MaxDurability;
	AddItem(NewItem, true);

	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, FString::Printf(TEXT("🛠️ 【工厂产出】成功制造：%s (耐久: %d/%d)！"), *NewItem.DisplayName.ToString(), NewItem.CurrentDurability, NewItem.MaxDurability));
	return true;
}

void ULSBackpackComponent::ProcessFailedExtraction(TArray<FLSInventoryItem>& OutDroppedLoot)
{
	OutDroppedLoot.Reset();

	// 1. 普通背包内全部物资彻底清空、遗留在死亡现场
	for (int32 i = 0; i < BackpackSlots.Num(); ++i)
	{
		if (BackpackSlots[i].IsValid())
		{
			OutDroppedLoot.Add(BackpackSlots[i]);
		}
	}
	BackpackSlots.Empty();

	// 2. 安全箱内的物品 100% 绝对保留（不作任何清空操作）
	OnInventoryChanged.Broadcast();

	GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Red, FString::Printf(TEXT("💀 【非正常撤离/战死】除安全箱外所有物资已全部遗失！掉落件数: %d"), OutDroppedLoot.Num()));
}