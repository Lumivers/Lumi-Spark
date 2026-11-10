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

	BaseCorrosionRate = 0.4f;       // 每秒 0.4%，250秒（约4分钟）满溢
	MaxCorrosion = 100.0f;
	CurrentCorrosion = 0.0f;
	OverloadDamageInterval = 1.0f;  // 每秒扣血
	OverloadDamagePercent = 0.08f;  // 8% 最大生命真实伤害
	MaxHealthCapReduction = 0.70f;  // 满溢时生命上限压制到 30%
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

	// 1. 战区内侵蚀度线性持续累积（0.4% / 秒）
	if (bIsInCorrosionZone)
	{
		CurrentCorrosion = FMath::Clamp(CurrentCorrosion + (BaseCorrosionRate * DeltaTime), 0.0f, MaxCorrosion);

		// 触发 100% 满溢过载
		if (CurrentCorrosion >= MaxCorrosion && !bIsOverloaded)
		{
			bIsOverloaded = true;
			OverloadDamageTimer = 0.0f;
			OnCorrosionOverloadChanged.Broadcast(true);
			GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Red, TEXT("🚨 【地脉侵蚀过载】侵蚀度达到 100%！生命回复上限压制至 30%，进入软狂暴持续扣血！"));
		}
	}

	// 2. 侵蚀实时压制当前出战角色的生命恢复上限
	if (ALSCharacterBase* ActiveChar = GetActiveCharacter())
	{
		if (ULSHealthComponent* HealthComp = ActiveChar->GetHealthComponent())
		{
			HealthComp->SetHealthCapRatio(GetHealthRecoveryCapPercent());
		}
	}

	// 3. 100% 软狂暴：每秒扣除 8% 最大生命值
	if (bIsOverloaded)
	{
		OverloadDamageTimer += DeltaTime;
		if (OverloadDamageTimer >= OverloadDamageInterval)
		{
			OverloadDamageTimer = 0.0f;
			ApplyOverloadDamage();
		}
	}

	OnCorrosionUpdated.Broadcast(CurrentCorrosion, MaxCorrosion);
}

void ULSCorrosionComponent::AddCorrosion(float Amount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || Amount <= 0.0f) return;

	CurrentCorrosion = FMath::Clamp(CurrentCorrosion + Amount, 0.0f, MaxCorrosion);

	if (CurrentCorrosion >= MaxCorrosion && !bIsOverloaded)
	{
		bIsOverloaded = true;
		OverloadDamageTimer = 0.0f;
		OnCorrosionOverloadChanged.Broadcast(true);
	}

	OnCorrosionUpdated.Broadcast(CurrentCorrosion, MaxCorrosion);
}

void ULSCorrosionComponent::ReduceCorrosion(float Amount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || Amount <= 0.0f) return;

	CurrentCorrosion = FMath::Clamp(CurrentCorrosion - Amount, 0.0f, MaxCorrosion);

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

	OnCorrosionUpdated.Broadcast(CurrentCorrosion, MaxCorrosion);
}

bool ULSCorrosionComponent::UsePurificationInjector(float CleanseAmount)
{
	if (CurrentCorrosion <= 0.0f && !bIsOverloaded) return false;

	ReduceCorrosion(CleanseAmount);
	GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Cyan, FString::Printf(TEXT("💉 【战术净化】清除 %.1f 侵蚀度（当前: %.1f%%）"), CleanseAmount, CurrentCorrosion));
	return true;
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

	GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Red, FString::Printf(TEXT("☣️ 【侵蚀软狂暴】机体承受侵蚀流失: -%.0f HP"), DamageToApply));
}

float ULSCorrosionComponent::GetVignetteIntensity() const
{
	// 80% 以下无警告
	if (CurrentCorrosion < 80.0f)
	{
		return 0.0f;
	}

	// 80% ~ 100%：线性升高的腐蚀脉冲音画警告
	if (!bIsOverloaded && CurrentCorrosion < MaxCorrosion)
	{
		const float Alpha = (CurrentCorrosion - 80.0f) / 20.0f;
		return FMath::Clamp(Alpha * 0.5f, 0.0f, 0.5f);
	}

	// 满 100% 过载持续强呼吸脉冲
	if (UWorld* World = GetWorld())
	{
		const float Pulse = (FMath::Sin(World->GetTimeSeconds() * 5.0f) + 1.0f) * 0.5f;
		return 0.65f + (Pulse * 0.30f);
	}

	return 0.75f;
}

void ULSCorrosionComponent::OnRep_CurrentCorrosion()
{
	OnCorrosionUpdated.Broadcast(CurrentCorrosion, MaxCorrosion);
}

void ULSCorrosionComponent::OnRep_IsOverloaded()
{
	OnCorrosionOverloadChanged.Broadcast(bIsOverloaded);
}
