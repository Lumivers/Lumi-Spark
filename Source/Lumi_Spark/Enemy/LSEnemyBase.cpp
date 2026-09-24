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