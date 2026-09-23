#include "LSCharacterBase.h"
#include "Character/LSCameraComponent.h"
#include "Character/LSMovementComponent.h"
#include "Character/LSSkillComponent.h"
#include "Character/LSHealthComponent.h"
#include "Character/LSStaminaComponent.h"
#include "Character/LSEnergyComponent.h"
#include "Character/LSTeamSwitchComponent.h"
#include "Core/LSPlayerController.h"
#include "Weapon/LSWeaponComponent.h"
#include "Element/LSElementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Weapon/LSThrowableComponent.h"

ALSCharacterBase::ALSCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<ULSMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	// 摄像机与手臂装配
	CameraComponent = CreateDefaultSubobject<ULSCameraComponent>(TEXT("LSCameraComp"));
	CameraComponent->SetupAttachment(GetCapsuleComponent());
	
	FPArmsMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FPArmsMesh"));
	FPArmsMesh->SetupAttachment(CameraComponent);
	FPArmsMesh->SetOnlyOwnerSee(true);
	FPArmsMesh->SetCastShadow(false);
	FPArmsMesh->SetCollisionProfileName(TEXT("NoCollision"));

	// 玩法业务组件
	WeaponComponent = CreateDefaultSubobject<ULSWeaponComponent>(TEXT("LSWeaponComp"));
	ElementComponent = CreateDefaultSubobject<ULSElementComponent>(TEXT("LSElementComp"));
	SkillComponent = CreateDefaultSubobject<ULSSkillComponent>(TEXT("LSSkillComp"));

	// 独立资源组件装配
	HealthComponent = CreateDefaultSubobject<ULSHealthComponent>(TEXT("LSHealthComp"));
	StaminaComponent = CreateDefaultSubobject<ULSStaminaComponent>(TEXT("LSStaminaComp"));
	EnergyComponent = CreateDefaultSubobject<ULSEnergyComponent>(TEXT("LSEnergyComp"));
	ThrowableComponent = CreateDefaultSubobject<ULSThrowableComponent>(TEXT("ThrowableComponent"));
}

void ALSCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	// 监听生命组件的死亡事件驱动小队顺切或倒地
	if (HealthComponent)
	{
		HealthComponent->OnDeath.AddDynamic(this, &ALSCharacterBase::HandleHealthComponentDeath);
	}
}

void ALSCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ALSCharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALSCharacterBase, bIsDowned);
}

bool ALSCharacterBase::IsDead() const
{
	return HealthComponent ? HealthComponent->IsDead() : false;
}

float ALSCharacterBase::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	if (!HasAuthority() || IsDead())
	{
		return 0.0f;
	}

	// 1. 无敌帧拦截（闪避时彻底免伤，不打扰生命与护盾）
	if (ULSMovementComponent* MoveComp = GetLSMovementComponent())
	{
		if (MoveComp->IsInvincible())
		{
			return 0.0f;
		}
	}

	// 2. 权威扣血交由生命组件独占计算（内部联动复合护盾）
	if (HealthComponent)
	{
		return HealthComponent->TakeDamage(DamageAmount, FGameplayTag(), DamageCauser, EventInstigator);
	}

	return 0.0f;
}

void ALSCharacterBase::HandleHealthComponentDeath()
{
	// 当血量扣尽触发死亡时：
	if (bIsDowned)
	{
		// 倒地状态下受到致命重创 -> 彻底死亡！
		Die(nullptr);
	}
	else
	{
		// 检查三人小队是否有存活备用队友顺切救场
		bool bCanSwitchStandby = false;
		if (ALSPlayerController* PC = Cast<ALSPlayerController>(GetController()))
		{
			if (ULSTeamSwitchComponent* TeamComp = PC->GetTeamSwitchComponent())
			{
				for (int32 i = 0; i < 3; ++i)
				{
					if (TeamComp->CanSwitchToIndex(i))
					{
						TeamComp->SwitchTo(i);
						bCanSwitchStandby = true;
						break;
					}
				}
			}
		}

		// 无备用队友（或联机中） -> 进入倒地匍匐状态等待救援
		if (!bCanSwitchStandby)
		{
			EnterDownedState(nullptr);
		}
	}
}

void ALSCharacterBase::Die(AActor* Killer)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BleedoutTimerHandle);
	}

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (WeaponComponent)
	{
		WeaponComponent->StopFire();
	}
}

void ALSCharacterBase::Revive(AActor* Reviver, float RestoredHealthPercent)
{
	bIsDowned = false;
	BleedoutRemainingTimer = 0.0f;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BleedoutTimerHandle);
	}

	if (HealthComponent)
	{
		HealthComponent->SetDead(false);
		HealthComponent->Heal(HealthComponent->GetMaxHealth() * RestoredHealthPercent);
	}

	if (ULSMovementComponent* MoveComp = GetLSMovementComponent())
	{
		MoveComp->MaxWalkSpeed = MoveComp->WalkSpeed;
	}
}

void ALSCharacterBase::EnterDownedState(AActor* Killer)
{
	bIsDowned = true;
	BleedoutRemainingTimer = DownedBleedoutMaxTime;

	if (WeaponComponent)
	{
		WeaponComponent->StopFire();
	}

	if (ULSMovementComponent* MoveComp = GetLSMovementComponent())
	{
		MoveComp->MaxWalkSpeed = DownedWalkSpeed;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(BleedoutTimerHandle, this, &ALSCharacterBase::HandleBleedoutTick, 1.0f, true);
	}
}

void ALSCharacterBase::HandleBleedoutTick()
{
	BleedoutRemainingTimer -= 1.0f;
	if (BleedoutRemainingTimer <= 0.0f)
	{
		Die(nullptr);
	}
}

void ALSCharacterBase::OnRep_IsDowned()
{
	if (ULSMovementComponent* MoveComp = GetLSMovementComponent())
	{
		MoveComp->MaxWalkSpeed = bIsDowned ? DownedWalkSpeed : MoveComp->WalkSpeed;
	}
}

void ALSCharacterBase::EnterBackgroundMode()
{
	SetActorHiddenInGame(true);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (WeaponComponent)
	{
		WeaponComponent->StopFire();
	}
}

void ALSCharacterBase::ExitBackgroundMode()
{
	SetActorHiddenInGame(false);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

bool ALSCharacterBase::CanInteract(AActor* Interactor) const
{
	return bIsDowned && !IsDead();
}

FText ALSCharacterBase::GetInteractPrompt(AActor* Interactor) const
{
	return FText::FromString(TEXT("长按 [F] 救起队友"));
}

float ALSCharacterBase::GetInteractDuration(AActor* Interactor) const
{
	return 3.0f;
}

void ALSCharacterBase::OnInteractComplete(AActor* Interactor)
{
	Revive(Interactor, 0.5f);
}