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
#include "GameFramework/SpringArmComponent.h"
#include "TimerManager.h"
#include "Weapon/LSThrowableComponent.h"

ALSCharacterBase::ALSCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<ULSMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	// 解绑角色与镜头朝向：平时镜头自由旋转，不强行随鼠标转向
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// 弹簧臂组件装配（驱动第三人称与过肩瞄准臂长及偏移）
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetCapsuleComponent());
	CameraBoom->TargetArmLength = 300.0f;
	CameraBoom->bUsePawnControlRotation = true; // 随 Controller 旋转
	CameraBoom->bDoCollisionTest = true;
	CameraBoom->SocketOffset = FVector(0.0f, 50.0f, 65.0f); // 默认右肩偏移

	// 摄像机挂载至弹簧臂末端 Socket
	CameraComponent = CreateDefaultSubobject<ULSCameraComponent>(TEXT("LSCameraComp"));
	CameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	CameraComponent->bUsePawnControlRotation = false; // 自身不自转，随弹簧臂
	CameraComponent->SpringArm = CameraBoom;
	

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

void ALSCharacterBase::SetCombatFacing(bool bEnable)
{
	if (bAlwaysOrientToControlRotation)
	{
		bUseControllerRotationYaw = true;
		if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
		{
			MoveComp->bOrientRotationToMovement = false;
			MoveComp->bUseControllerDesiredRotation = true;
		}
		SetALSRotationMode(2); // 模式 2 = Aiming (端枪抵肩瞄准)
		return;
	}

	bUseControllerRotationYaw = bEnable;
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement = !bEnable;
		MoveComp->bUseControllerDesiredRotation = bEnable;
		MoveComp->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
	}

	// 核心修复：战斗开火/瞄准时必须切换为模式 2 (Aiming，进入 Rifle Aiming 状态端枪抵肩对准准星)；退出后恢复模式 0 (VelocityDirection 放松提枪跑动)
	SetALSRotationMode(bEnable ? 2 : 0);
}

void ALSCharacterBase::SetALSRotationMode(uint8 InRotationMode)
{
	// 1. 直接同步反射属性，确保 ALS 蓝图内部变量立即响应
	if (FByteProperty* ModeProp = CastField<FByteProperty>(GetClass()->FindPropertyByName(FName("RotationMode"))))
	{
		ModeProp->SetPropertyValue_InContainer(this, InRotationMode);
	}
	if (FByteProperty* DesiredProp = CastField<FByteProperty>(GetClass()->FindPropertyByName(FName("DesiredRotationMode"))))
	{
		DesiredProp->SetPropertyValue_InContainer(this, InRotationMode);
	}

	// 2. 派发 ALS 原生蓝图接口事件驱动状态机与动画过渡
	static const FName FuncName(TEXT("BPI_Set_RotationMode"));
	if (UFunction* Func = FindFunction(FuncName))
	{
		struct FParams
		{
			uint8 NewRotationMode;
		} Params;
		Params.NewRotationMode = InRotationMode;
		ProcessEvent(Func, &Params);
	}
}

void ALSCharacterBase::SetALSDesiredGait(uint8 InGait)
{
	// 1. 同步反射属性 DesiredGait 与 SprintHeld (InGait == 2 代表冲刺)
	if (FByteProperty* GaitProp = CastField<FByteProperty>(GetClass()->FindPropertyByName(FName("DesiredGait"))))
	{
		GaitProp->SetPropertyValue_InContainer(this, InGait);
	}
	if (FBoolProperty* SprintProp = CastField<FBoolProperty>(GetClass()->FindPropertyByName(FName("SprintHeld"))))
	{
		SprintProp->SetPropertyValue_InContainer(this, InGait == 2);
	}

	// 2. 派发 ALS 原生蓝图接口 BPI_Set_Gait 事件驱动状态机与动画过渡
	static const FName FuncName(TEXT("BPI_Set_Gait"));
	if (UFunction* Func = FindFunction(FuncName))
	{
		struct FParams
		{
			uint8 NewGait;
		} Params;
		Params.NewGait = InGait;
		ProcessEvent(Func, &Params);
	}
}

void ALSCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	// 初始化战斗朝向
	SetCombatFacing(bAlwaysOrientToControlRotation);

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