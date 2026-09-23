#include "LSElementalField.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Element/LSElementComponent.h"
#include "Core/LSEventBus.h"
#include "Engine/World.h"
#include "Engine/DamageEvents.h"
#include "DrawDebugHelpers.h"

ALSElementalField::ALSElementalField()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	FieldSphere = CreateDefaultSubobject<USphereComponent>(TEXT("FieldSphere"));
	FieldSphere->InitSphereRadius(FieldRadius);
	FieldSphere->SetCollisionProfileName(TEXT("Trigger"));
	FieldSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	FieldSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	RootComponent = FieldSphere;
}

void ALSElementalField::BeginPlay()
{
	Super::BeginPlay();

	SetLifeSpan(Duration);

	// 仅在权威端运行定时器结算
	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(PeriodicTimerHandle, this, &ALSElementalField::HandlePeriodicTick, TickInterval, true);
	}
}

void ALSElementalField::InitializeField(FGameplayTag InElementTag, AActor* InInstigator, float InDuration, float InRadius)
{
	ElementTag = InElementTag;
	FieldInstigator = InInstigator;
	Duration = InDuration;
	FieldRadius = InRadius;

	if (FieldSphere)
	{
		FieldSphere->SetSphereRadius(FieldRadius);
	}
	SetLifeSpan(Duration);
}

void ALSElementalField::HandlePeriodicTick()
{
	if (!HasAuthority() || !FieldSphere) return;

	TArray<AActor*> OverlappingActors;
	FieldSphere->GetOverlappingActors(OverlappingActors, APawn::StaticClass());

	// 绘制地面可视圈（绿色/橙色/蓝色）
	DrawDebugCircle(GetWorld(), GetActorLocation(), FieldRadius, 32, GetFieldDebugColor(), false, TickInterval + 0.05f, 0, 2.5f, FVector(0, 0, 1), FVector(1, 0, 0));

	ULSEventBus* EventBus = ULSEventBus::Get(this);

	for (AActor* Victim : OverlappingActors)
	{
		// 忽略自身与手雷投掷者
		if (!Victim || Victim == FieldInstigator) continue;

		ULSElementComponent* TargetElemComp = Victim->FindComponentByClass<ULSElementComponent>();

		// 1. 火海领域：持续 15 点 DoT 伤害与 1U 弱火附着
		if (ElementTag == LSTags::TAG_Element_Pyro)
		{
			if (TargetElemComp)
			{
				TargetElemComp->ApplyElement(FieldInstigator ? FieldInstigator : this, ElementTag, ELSElementGauge::Light);
			}

			Victim->TakeDamage(FireDoTDamage, FDamageEvent(), FieldInstigator ? FieldInstigator->GetInstigatorController() : nullptr, this);

			if (EventBus)
			{
				FLSDamageContext Context;
				Context.DamageCauser = FieldInstigator ? FieldInstigator : this;
				Context.TargetActor = Victim;
				Context.BaseDamage = FireDoTDamage;
				Context.FinalDamage = FireDoTDamage;
				Context.ElementTag = ElementTag;
				Context.DamageTypeTag = LSTags::TAG_Damage_Type_DoT;
				EventBus->OnDamageDealt.Broadcast(Context);
			}
		}
		// 2. 水雾领域：持续施加 1U 弱水附着（湿润状态）
		else if (ElementTag == LSTags::TAG_Element_Hydro)
		{
			if (TargetElemComp)
			{
				TargetElemComp->ApplyElement(FieldInstigator ? FieldInstigator : this, ElementTag, ELSElementGauge::Light);
			}
		}
		// 3. 霜原领域：持续 1U 弱冰附着，且降低 40% 移动速度
		else if (ElementTag == LSTags::TAG_Element_Cryo)
		{
			if (TargetElemComp)
			{
				TargetElemComp->ApplyElement(FieldInstigator ? FieldInstigator : this, ElementTag, ELSElementGauge::Light);
			}

			if (ACharacter* VictimChar = Cast<ACharacter>(Victim))
			{
				if (UCharacterMovementComponent* MoveComp = VictimChar->GetCharacterMovement())
				{
					// 降低移速并在此次 Tick 后自动恢复保底
					MoveComp->MaxWalkSpeed = FMath::Max(150.0f, MoveComp->MaxWalkSpeed * IceSlowMultiplier);
				}
			}
		}
		// 4. 雷磁领域：周期性施加 1U 弱雷附着
		else if (ElementTag == LSTags::TAG_Element_Electro)
		{
			if (TargetElemComp)
			{
				TargetElemComp->ApplyElement(FieldInstigator ? FieldInstigator : this, ElementTag, ELSElementGauge::Light);
			}
		}
		// 5. 草雾领域：持续播撒草反应底
		else if (ElementTag == LSTags::TAG_Element_Dendro)
		{
			if (TargetElemComp)
			{
				TargetElemComp->ApplyElement(FieldInstigator ? FieldInstigator : this, ElementTag, ELSElementGauge::Light);
			}
		}
	}
}

FColor ALSElementalField::GetFieldDebugColor() const
{
	if (ElementTag == LSTags::TAG_Element_Pyro) return FColor::Red;
	if (ElementTag == LSTags::TAG_Element_Hydro) return FColor::Blue;
	if (ElementTag == LSTags::TAG_Element_Cryo) return FColor::Cyan;
	if (ElementTag == LSTags::TAG_Element_Electro) return FColor::Purple;
	if (ElementTag == LSTags::TAG_Element_Dendro) return FColor::Green;
	return FColor::Yellow;
}