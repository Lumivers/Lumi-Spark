#include "LSEnemyBase.h"
#include "LSEnemyDataAsset.h"
#include "Character/LSHealthComponent.h"
#include "Element/LSElementComponent.h"
#include "Combat/LSShieldComponent.h"
#include "Combat/LSThreatComponent.h"
#include "Character/LSEnergyComponent.h"
#include "Character/LSCharacterBase.h"
#include "Core/LSEventBus.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/DamageEvents.h"

ALSEnemyBase::ALSEnemyBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	// 装配专职战斗组件
	HealthComponent = CreateDefaultSubobject<ULSHealthComponent>(TEXT("HealthComp"));
	ElementComponent = CreateDefaultSubobject<ULSElementComponent>(TEXT("ElementComp"));
	ShieldComponent = CreateDefaultSubobject<ULSShieldComponent>(TEXT("ShieldComp"));
	ThreatComponent = CreateDefaultSubobject<ULSThreatComponent>(TEXT("ThreatComp"));

	// 敌人碰撞通道配置
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
}

// ILSInteractableInterface 交互接口实现
bool ALSEnemyBase::CanInteract(AActor* Interactor) const
{
	if (!bAllowBackstabExecution || bIsDead || !Interactor)
	{
		return false;
	}
	
	// 目标已死亡判定
	if (HealthComponent && HealthComponent->IsDead())
	{
		return false;
	}
	
	const FVector EnemyLoc = GetActorLocation();
	const FVector InteractorLoc = Interactor->GetActorLocation();
	
	// 1. 高度差校验（防止跳在头顶或上下楼梯误触）
	if (FMath::Abs(EnemyLoc.Z - InteractorLoc.Z) > 100.0f)
	{
		return false;
	}
	
	// 2. 水平距离校验（默认 2.3 米内）
	const float DistSq2D = FVector::DistSquared2D(EnemyLoc, InteractorLoc);
	if (DistSq2D > FMath::Square(BackstabMaxDistance))
	{
		return false;
	}
	
	// 3. 背后空间点积校验（核心几何计算）
	const FVector EnemyForward = GetActorForwardVector().GetSafeNormal2D();
	const FVector ToInteractor = (InteractorLoc - EnemyLoc).GetSafeNormal2D();
	
	// 当玩家处于怪的正背后时，ToInteractor 与 EnemyForward 反向，点积趋近于 -1.0
	// 这里取 Dot < -0.4f，相当于怪物背后约 132° 的后向扇形夹角
	const float BehindDot = FVector::DotProduct(EnemyForward, ToInteractor);
	if (BehindDot > -0.4f)
	{
		return false; // 未在背后扇形区域
	}
	
	// 4. 玩家朝向校验（玩家必须面朝怪物背部）
	const FVector InteractorForward = Interactor->GetActorForwardVector().GetSafeNormal2D();
	const float FacingDot = FVector::DotProduct(InteractorForward, EnemyForward);
	if (FacingDot < 0.2f)
	{
		return false; // 玩家未朝向怪物背部（如背对背）
	}
	
	return true;
}

FText ALSEnemyBase::GetInteractPrompt(AActor* Interactor) const
{
	return NSLOCTEXT("LumiSpark", "XenoBladePrompt", "按 [F] 异体刃背后处决 (-75% HP / 破盾)");
}

void ALSEnemyBase::OnInteractComplete(AActor* Interactor)
{
	// 按 F 瞬发处决
	ExecuteXenoBlade(Interactor);
}

bool ALSEnemyBase::ExecuteXenoBlade(AActor* Interactor)
{
	if (!HasAuthority()) return false;
	if (bIsDead || !HealthComponent || HealthComponent->IsDead()) return false;
	
	// 1. 瞬碎所有元素护盾
	if (ShieldComponent)
	{
		ShieldComponent->ShatterAllShields();
	}
	
	// 2. 扣除 75% 最大生命值
	const float MaxHP = HealthComponent->GetMaxHealth();
	const float ExecutionDamage = MaxHP * BackstabDamageRatio;
	
	// 处决伤害划归为技能真伤类型（透穿肉身）
	HealthComponent->TakeDamage(ExecutionDamage, LSTags::TAG_Damage_Type_Skill, Interactor, Interactor ? Interactor->GetInstigatorController() : nullptr);
	
	// 3. 广播处决事件
	OnXenoBladeExecuted.Broadcast(this, Interactor, ExecutionDamage);
	
	// 4. 调试反馈
	GEngine->AddOnScreenDebugMessage(-1, 3.5f, FColor::Red, FString::Printf(TEXT("🗡️ 【异体刃处决】成功！对 %s 造成 %.0f 点真实伤害（75%% 最大生命），全层元素护盾瞬间粉碎！"), *GetName(), ExecutionDamage));
	
	return true;
}

void ALSEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	// 1. 绑定生命组件的死亡广播
	if (HealthComponent)
	{
		HealthComponent->OnDeath.AddDynamic(this, &ALSEnemyBase::HandleHealthComponentDeath);
	}

	// 2. 从 EnemyDataAsset 资产初始化属性与抗性
	if (EnemyDataAsset)
	{
		CachedResistances = EnemyDataAsset->Resistances;

		if (HealthComponent)
		{
			HealthComponent->InitializeHealth(EnemyDataAsset->MaxHealth);
		}

		if (EnemyDataAsset->bDefaultSuperArmor)
		{
			ActiveGameplayTags.AddTag(LSTags::TAG_State_SuperArmor);
		}

		SetCombatMovementSpeed(false);
	}
}

float ALSEnemyBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead || DamageAmount <= 0.0f) return 0.0f;

	// 1. 解析伤害中的元素属性并应用元素抗性减免
	float AdjustedDamage = DamageAmount;
	// 默认假定无元素，若有命中上下文可在后续集成
	FGameplayTag DmgElement = FGameplayTag();

	if (CachedResistances.Contains(DmgElement))
	{
		const float ResFactor = FMath::Clamp(CachedResistances[DmgElement], -1.0f, 1.0f);
		AdjustedDamage *= (1.0f - ResFactor);
	}

	// 2. 先走多层元素破盾判定 (ShieldComponent)
	if (ShieldComponent && ShieldComponent->HasActiveShield())
	{
		AdjustedDamage = ShieldComponent->AbsorbDamage(AdjustedDamage, DmgElement, DamageCauser);
	}

	// 3. 记录仇恨值 (ThreatComponent)
	if (ThreatComponent && DamageCauser)
	{
		ThreatComponent->AddThreatFromDamage(DamageCauser, AdjustedDamage, false);
	}

	// 4. 剩余伤害移交生命组件
	if (HealthComponent && AdjustedDamage > 0.0f)
	{
		HealthComponent->TakeDamage(AdjustedDamage, DmgElement, DamageCauser, EventInstigator);
	}

	return AdjustedDamage;
}

void ALSEnemyBase::SetCombatMovementSpeed(bool bInCombat)
{
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		if (EnemyDataAsset)
		{
			MoveComp->MaxWalkSpeed = bInCombat ? EnemyDataAsset->CombatRunSpeed : EnemyDataAsset->PatrolWalkSpeed;
		}
	}
}

void ALSEnemyBase::HandleHealthComponentDeath()
{
	OnDeath(nullptr);
}

void ALSEnemyBase::OnDeath(AActor* Killer)
{
	if (bIsDead) return;
	bIsDead = true;

	ActiveGameplayTags.AddTag(LSTags::TAG_State_Dead);

	// 掉落元素能量微粒
	SpawnEnergyParticles(Killer);

	// 停止移动与碰撞
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 延时销毁或执行布娃娃（Ragdoll）
	SetLifeSpan(3.0f);
}

void ALSEnemyBase::SpawnEnergyParticles(AActor* Killer)
{
	if (!EnemyDataAsset) return;

	// 向全局总线或直接向击杀者小队注入能量微粒
	if (Killer)
	{
		if (ALSCharacterBase* PlayerChar = Cast<ALSCharacterBase>(Killer))
		{
			if (ULSEnergyComponent* EnergyComp = PlayerChar->GetEnergyComponent())
			{
				for (int32 i = 0; i < EnemyDataAsset->DroppedParticleCount; ++i)
				{
					EnergyComp->CollectParticle(EnemyDataAsset->DroppedParticleElement, EnemyDataAsset->EnergyPerParticle);
				}
			}
		}
	}
}