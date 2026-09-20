#include "Combat/LSTargetDummy.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Element/LSElementComponent.h"
#include "Core/LSEventBus.h"
#include "TimerManager.h"
#include "Combat/LSShieldComponent.h"
#include "Core/LSTypes.h"

ALSTargetDummy::ALSTargetDummy()
{
	PrimaryActorTick.bCanEverTick = false;

	// 1. 身体胶囊体碰撞（根组件）
	CapsuleComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComp"));
	CapsuleComp->InitCapsuleSize(45.0f, 95.0f);
	CapsuleComp->SetCollisionProfileName(TEXT("Pawn"));
	CapsuleComp->SetCanEverAffectNavigation(false);
	RootComponent = CapsuleComp;

	// 2. 头部弱点碰撞球（爆头判定盒）
	HeadWeakspotComp = CreateDefaultSubobject<USphereComponent>(TEXT("HeadWeakspotComp"));
	HeadWeakspotComp->SetupAttachment(CapsuleComp);
	HeadWeakspotComp->InitSphereRadius(22.0f);
	HeadWeakspotComp->SetRelativeLocation(FVector(0.0f, 0.0f, 68.0f));
	HeadWeakspotComp->SetCollisionProfileName(TEXT("Pawn"));
	// 添加 head 标签供武器 Hitscan 精准识别爆头弱点
	HeadWeakspotComp->ComponentTags.Add(TEXT("head"));

	// 3. 显示网格体
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(CapsuleComp);
	MeshComp->SetRelativeLocation(FVector(0.0f, 0.0f, -95.0f));
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 碰撞统一由 Capsule 和 Weakspot 接管

	// 4. 高等元素论中枢组件（挂载后天然支持 16 种元素反应与 ICD 衰减）
	ElementComp = CreateDefaultSubobject<ULSElementComponent>(TEXT("ElementComp"));
	
	// 5. 可选护盾组件（可在蓝图中添加，支持元素反应与伤害吸收）
	ShieldComp = CreateDefaultSubobject<ULSShieldComponent>(TEXT("LSShieldComp"));
	ShieldComp->AddShieldLayer(LSTags::TAG_Element_Cryo, 2000.0f);     // 外层冰盾（弱火 2.0x）
	ShieldComp->AddShieldLayer(LSTags::TAG_Element_Electro, 2000.0f);  // 核心雷盾（弱草 2.0x）
}

void ALSTargetDummy::BeginPlay()
{
	Super::BeginPlay();
	CurrentHealth = MaxHealth;
}

float ALSTargetDummy::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	if (!HasAuthority() || CurrentHealth <= 0.0f)
	{
		return 0.0f;
	}

	// 1. 提取攻击方携带的元素标签（若无则默认为物理）
	FGameplayTag AttackElement = LSTags::TAG_Element_Physical;
	if (DamageCauser)
	{
		// 优先从造成的伤害上下文中提取，或从手雷/枪械提取
		if (DamageCauser->ActorHasTag(TEXT("Element.Pyro"))) AttackElement = LSTags::TAG_Element_Pyro;
		else if (DamageCauser->ActorHasTag(TEXT("Element.Dendro"))) AttackElement = LSTags::TAG_Element_Dendro;
		else if (DamageCauser->ActorHasTag(TEXT("Element.Hydro"))) AttackElement = LSTags::TAG_Element_Hydro;
		else if (DamageCauser->ActorHasTag(TEXT("Element.Electro"))) AttackElement = LSTags::TAG_Element_Electro;
	}

	// 2. 复合元素护盾优先吸收伤害
	float RemainingDamage = DamageAmount;
	if (ShieldComp && ShieldComp->HasActiveShield())
	{
		RemainingDamage = ShieldComp->AbsorbDamage(DamageAmount, AttackElement, DamageCauser);
	}

	if (RemainingDamage <= 0.0f)
	{
		return 0.0f; // 完全被护盾抵消，肉身不扣血
	}

	// 3. 剩余溢出穿透伤害扣减本体
	const float ActualDamage = Super::TakeDamage(RemainingDamage, DamageEvent, EventInstigator, DamageCauser);
	if (!bInfiniteHealth)
	{
		CurrentHealth = FMath::Clamp(CurrentHealth - ActualDamage, 0.0f, MaxHealth);
	}

	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);

	if (CurrentHealth <= 0.0f)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(ResetTimerHandle, this, &ALSTargetDummy::ResetDummy, AutoResetDelay, false);
		}
	}

	return ActualDamage;
}

void ALSTargetDummy::ResetDummy()
{
	GetWorldTimerManager().ClearTimer(ResetTimerHandle);
	CurrentHealth = MaxHealth;
	if (ShieldComp)
	{
		ShieldComp->RestoreAllShields(); // 回满所有层护盾
	}
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
	OnDummyReset.Broadcast();
}