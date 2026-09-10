#include "UI/LSDamagePopWidget.h"
#include "Core/LSEventBus.h"
#include "GameFramework/PlayerController.h"

void ULSDamagePopWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 监听全局事件总线（纯解耦，绝不持有武器或怪物指针）
	if (ULSEventBus* EventBus = ULSEventBus::Get(this))
	{
		EventBus->OnDamageDealt.AddDynamic(this, &ULSDamagePopWidget::HandleDamageDealt);
		EventBus->OnElementReactionTriggered.AddDynamic(this, &ULSDamagePopWidget::HandleReactionTriggered);
	}
}

void ULSDamagePopWidget::NativeDestruct()
{
	if (ULSEventBus* EventBus = ULSEventBus::Get(this))
	{
		EventBus->OnDamageDealt.RemoveDynamic(this, &ULSDamagePopWidget::HandleDamageDealt);
		EventBus->OnElementReactionTriggered.RemoveDynamic(this, &ULSDamagePopWidget::HandleReactionTriggered);
	}

	Super::NativeDestruct();
}

void ULSDamagePopWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;

	// 逐帧更新活跃飘字：世界位置上浮、投影到屏幕、透明度渐隐与销毁
	for (int32 i = ActivePopItems.Num() - 1; i >= 0; --i)
	{
		FLSDamagePopItem& Item = ActivePopItems[i];
		Item.RemainingLifeTime -= InDeltaTime;

		if (Item.RemainingLifeTime <= 0.0f)
		{
			ActivePopItems.RemoveAt(i);
			continue;
		}

		// 1. 物理位置沿初速度上浮
		Item.WorldLocation += Item.Velocity * InDeltaTime;

		// 2. 3D 世界坐标投影至 2D 视口屏幕坐标
		PC->ProjectWorldLocationToScreen(Item.WorldLocation, Item.ScreenPosition, true);

		// 3. 计算生命周期曲线：前 20% 快速弹性放大，后 80% 平滑淡出
		const float Progress = 1.0f - (Item.RemainingLifeTime / Item.TotalLifeTime);
		if (Progress < 0.2f)
		{
			// 弹性弹出感 (Pop In)
			const float PopAlpha = Progress / 0.2f;
			Item.CurrentScale = FMath::Lerp(0.5f, Item.bIsCritical ? CritScaleMultiplier : 1.0f, PopAlpha);
		}
		else
		{
			// 平滑渐隐 (Fade Out)
			const float FadeAlpha = (Progress - 0.2f) / 0.8f;
			Item.CurrentAlpha = FMath::Clamp(1.0f - FadeAlpha, 0.0f, 1.0f);
		}
	}
}

void ULSDamagePopWidget::HandleDamageDealt(const FLSDamageContext& DamageContext)
{
	// 仅展示本地玩家造成的伤害（若是多端联机，避免其他玩家打的字满屏乱飞）
	APlayerController* PC = GetOwningPlayer();
	if (PC && DamageContext.DamageCauser)
	{
		if (DamageContext.DamageCauser != PC->GetPawn() && DamageContext.DamageCauser->GetOwner() != PC->GetPawn())
		{
			return;
		}
	}

	const FVector HitLoc = DamageContext.HitResult.ImpactPoint.IsNearlyZero()
		? (DamageContext.TargetActor ? DamageContext.TargetActor->GetActorLocation() : FVector::ZeroVector)
		: DamageContext.HitResult.ImpactPoint;

	SpawnPopNumber(
		DamageContext.FinalDamage,
		HitLoc,
		DamageContext.ElementTag,
		DamageContext.bIsCritical,
		DamageContext.bIsReactionDamage ? DamageContext.ElementTag : FGameplayTag()
	);
}

void ULSDamagePopWidget::HandleReactionTriggered(AActor* Target, FGameplayTag ReactionTag, float ReactionDamage, AActor* Instigator)
{
	// 仅展示自己触发的剧变反应飘字
	APlayerController* PC = GetOwningPlayer();
	if (PC && Instigator && Instigator != PC->GetPawn())
	{
		return;
	}

	const FVector HitLoc = Target ? (Target->GetActorLocation() + FVector(0.f, 0.f, 50.f)) : FVector::ZeroVector;

	// 剧变反应飘字
	SpawnPopNumber(ReactionDamage, HitLoc, FGameplayTag(), true, ReactionTag);
}

void ULSDamagePopWidget::SpawnPopNumber(float Damage, const FVector& HitLocation, const FGameplayTag& ElementTag, bool bIsCritical, const FGameplayTag& ReactionTag)
{
	if (Damage <= 0.0f) return;

	FLSDamagePopItem NewItem;
	NewItem.Damage = Damage;
	NewItem.bIsCritical = bIsCritical;
	NewItem.TotalLifeTime = PopLifeTime;
	NewItem.RemainingLifeTime = PopLifeTime;
	NewItem.DisplayColor = GetColorForElement(ElementTag);
	NewItem.ReactionText = GetReactionNameText(ReactionTag);

	// 核心防重叠算法：加入微小的随机径向散布，让高频子弹呈喷泉错落展开
	const float RandomAngle = FMath::FRandRange(-0.8f, 0.8f);
	const float SideImpulse = FMath::FRandRange(-50.0f, 50.0f);
	NewItem.WorldLocation = HitLocation + FVector(SideImpulse * 0.3f, SideImpulse * 0.3f, FMath::FRandRange(0.f, 20.f));
	NewItem.Velocity = FVector(SideImpulse, SideImpulse * 0.5f, FloatSpeed);

	if (bIsCritical)
	{
		// 暴击字赋予醒目的金色泛光
		NewItem.DisplayColor = FLinearColor(1.0f, 0.85f, 0.1f);
	}

	ActivePopItems.Add(NewItem);
	OnPopItemCreated(NewItem);
}

FLinearColor ULSDamagePopWidget::GetColorForElement(const FGameplayTag& ElementTag) const
{
	// 原神标准 7 大元素色相库
	if (ElementTag == LSTags::TAG_Element_Pyro)      return FLinearColor(1.0f, 0.35f, 0.1f);  // 火红
	if (ElementTag == LSTags::TAG_Element_Hydro)     return FLinearColor(0.1f, 0.6f, 1.0f);   // 湛蓝
	if (ElementTag == LSTags::TAG_Element_Electro)   return FLinearColor(0.75f, 0.3f, 1.0f);  // 紫晶
	if (ElementTag == LSTags::TAG_Element_Cryo)      return FLinearColor(0.4f, 0.9f, 1.0f);   // 冰蓝
	if (ElementTag == LSTags::TAG_Element_Dendro)    return FLinearColor(0.35f, 0.95f, 0.15f);// 荧光草绿
	if (ElementTag == LSTags::TAG_Element_Anemo)     return FLinearColor(0.3f, 1.0f, 0.8f);   // 青绿
	if (ElementTag == LSTags::TAG_Element_Geo)       return FLinearColor(1.0f, 0.8f, 0.2f);   // 琥珀金

	return FLinearColor::White; // 默认物理纯白
}

FText ULSDamagePopWidget::GetReactionNameText(const FGameplayTag& ReactionTag) const
{
	if (ReactionTag == LSTags::TAG_Reaction_Vaporize)       return FText::FromString(TEXT("蒸发"));
	if (ReactionTag == LSTags::TAG_Reaction_Melt)           return FText::FromString(TEXT("融化"));
	if (ReactionTag == LSTags::TAG_Reaction_Overload)       return FText::FromString(TEXT("超载"));
	if (ReactionTag == LSTags::TAG_Reaction_Superconduct)   return FText::FromString(TEXT("超导"));
	if (ReactionTag == LSTags::TAG_Reaction_ElectroCharged) return FText::FromString(TEXT("感电"));
	if (ReactionTag == LSTags::TAG_Reaction_Freeze)         return FText::FromString(TEXT("冻结"));
	if (ReactionTag == LSTags::TAG_Reaction_Bloom)          return FText::FromString(TEXT("绽放"));
	if (ReactionTag == LSTags::TAG_Reaction_Burgeon)        return FText::FromString(TEXT("烈绽放"));
	if (ReactionTag == LSTags::TAG_Reaction_Hyperbloom)     return FText::FromString(TEXT("超绽放"));
	if (ReactionTag == LSTags::TAG_Reaction_Swirl)          return FText::FromString(TEXT("扩散"));

	return FText::GetEmpty();
}