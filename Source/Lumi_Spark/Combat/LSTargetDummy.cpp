#include "Combat/LSTargetDummy.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Element/LSElementComponent.h"
#include "Core/LSEventBus.h"
#include "TimerManager.h"

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
}

void ALSTargetDummy::BeginPlay()
{
	Super::BeginPlay();
	CurrentHealth = MaxHealth;
}

float ALSTargetDummy::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	if (DamageAmount <= 0.0f) return 0.0f;

	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (bInfiniteHealth)
	{
		CurrentHealth = MaxHealth;
		OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
		return ActualDamage;
	}

	CurrentHealth = FMath::Clamp(CurrentHealth - ActualDamage, 0.0f, MaxHealth);
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);

	// 血量打空时触发击破反馈与延迟自动回血
	if (CurrentHealth <= 0.0f)
	{
		// 广播全局击杀事件
		if (ULSEventBus* EventBus = ULSEventBus::Get(this))
		{
			EventBus->OnEnemyKilled.Broadcast(this, DamageCauser);
		}

		// 启动自动复活重置计时器
		GetWorldTimerManager().ClearTimer(ResetTimerHandle);
		GetWorldTimerManager().SetTimer(ResetTimerHandle, this, &ALSTargetDummy::ResetDummy, AutoResetDelay, false);
	}

	return ActualDamage;
}

void ALSTargetDummy::ResetDummy()
{
	GetWorldTimerManager().ClearTimer(ResetTimerHandle);
	CurrentHealth = MaxHealth;
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
	OnDummyReset.Broadcast();
}