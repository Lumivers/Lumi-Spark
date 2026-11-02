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

namespace
{
	static void SetALSPropertyByteOrEnum(UObject* Target, FName PropName, uint8 Value)
	{
		if (!Target) return;
		if (FProperty* Prop = Target->GetClass()->FindPropertyByName(PropName))
		{
			if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Prop))
			{
				if (FNumericProperty* Underlying = EnumProp->GetUnderlyingProperty())
				{
					void* ValuePtr = EnumProp->ContainerPtrToValuePtr<void>(Target);
					Underlying->SetIntPropertyValue(ValuePtr, static_cast<int64>(Value));
					UE_LOG(LogTemp, Log, TEXT("[SetALSProperty] SUCCESS: %s.%s = %d (FEnumProperty)"), *Target->GetName(), *PropName.ToString(), Value);
					return;
				}
			}
			else if (FByteProperty* ByteProp = CastField<FByteProperty>(Prop))
			{
				ByteProp->SetPropertyValue_InContainer(Target, Value);
				UE_LOG(LogTemp, Log, TEXT("[SetALSProperty] SUCCESS: %s.%s = %d (FByteProperty)"), *Target->GetName(), *PropName.ToString(), Value);
				return;
			}
			UE_LOG(LogTemp, Warning, TEXT("[SetALSProperty] FAIL: %s.%s 找到但既非 Enum 也非 Byte!"), *Target->GetName(), *PropName.ToString());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[SetALSProperty] NOT FOUND: %s 上未找到名为 %s 的属性!"), *Target->GetName(), *PropName.ToString());
		}
	}

	static uint8 GetALSPropertyByteOrEnum(const UObject* Target, FName PropName)
	{
		if (!Target) return 255;
		if (FProperty* Prop = Target->GetClass()->FindPropertyByName(PropName))
		{
			if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Prop))
			{
				if (FNumericProperty* Underlying = EnumProp->GetUnderlyingProperty())
				{
					const void* ValuePtr = EnumProp->ContainerPtrToValuePtr<void>(Target);
					return static_cast<uint8>(Underlying->GetSignedIntPropertyValue(ValuePtr));
				}
			}
			else if (FByteProperty* ByteProp = CastField<FByteProperty>(Prop))
			{
				return ByteProp->GetPropertyValue_InContainer(Target);
			}
		}
		return 255;
	}
}

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
		bUseControllerRotationYaw = false;
		if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
		{
			MoveComp->bOrientRotationToMovement = false;
			MoveComp->bUseControllerDesiredRotation = false;
		}
		SetALSRotationMode(2); // 模式 2 = Aiming (端枪抵肩瞄准)
		return;
	}

	bUseControllerRotationYaw = false;
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement = false;
		MoveComp->bUseControllerDesiredRotation = false;
		MoveComp->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
	}

	// 核心修复：战斗开火/瞄准时必须切换为模式 2 (Aiming，进入 Rifle Aiming 状态端枪抵肩对准准星)；退出后恢复模式 0 (VelocityDirection 放松提枪跑动)
	SetALSRotationMode(bEnable ? 2 : 1);
}

void ALSCharacterBase::SetALSRotationMode(uint8 InRotationMode)
{
	// 1. 直接同步反射属性，确保 ALS 蓝图内部变量立即响应
	SetALSPropertyByteOrEnum(this, FName("RotationMode"), InRotationMode);
	// unused
	{

	}
	SetALSPropertyByteOrEnum(this, FName("DesiredRotationMode"), InRotationMode);
	// unused
	{

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
	SetALSPropertyByteOrEnum(this, FName("DesiredGait"), InGait);
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

void ALSCharacterBase::SetALSOverlayState(uint8 InOverlayState)
{
	UE_LOG(LogTemp, Warning, TEXT("🎯 [SetALSOverlayState] 收到调用，修改 OverlayState: %d -> %d"),
		GetALSPropertyByteOrEnum(this, FName("OverlayState")), InOverlayState);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Orange,
			FString::Printf(TEXT("🎯 [SetALSOverlayState] 修改 OverlayState 为 %d"), InOverlayState));
	}
	// 1. 设置 Character 自身的 OverlayState 反射变量
	SetALSPropertyByteOrEnum(this, FName("OverlayState"), InOverlayState);

	// 2. 同步设置 AnimInstance (ALS_AnimBP) 上的 OverlayState 变量
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		if (UAnimInstance* AnimInst = MeshComp->GetAnimInstance())
		{
			SetALSPropertyByteOrEnum(AnimInst, FName("OverlayState"), InOverlayState);
		}
	}

	// 3. 派发 ALS 原生蓝图接口 BPI_Set_OverlayState
	static const FName FuncName(TEXT("BPI_Set_OverlayState"));
	if (UFunction* Func = FindFunction(FuncName))
	{
		struct FParams
		{
			uint8 NewOverlayState;
		} Params;
		Params.NewOverlayState = InOverlayState;
		ProcessEvent(Func, &Params);
	}
}

void ALSCharacterBase::BeginPlay()
{
	Super::BeginPlay();


	// 初始化战斗朝向
	// 仅在明确启用准星锁定时才强制覆盖朝向，否则保留蓝图资产自身配置的默认旋转模式
	if (bAlwaysOrientToControlRotation)
	{
		SetCombatFacing(true);
	}

	// 🔍 [Debug 监测] 打印出生时蓝图读取到的初始状态
	const uint8 InitOverlay = GetALSPropertyByteOrEnum(this, FName("OverlayState"));
	const uint8 InitRotMode = GetALSPropertyByteOrEnum(this, FName("RotationMode"));
	const uint8 InitGait = GetALSPropertyByteOrEnum(this, FName("DesiredGait"));
	UAnimInstance* AnimInst = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	const uint8 AnimOverlay = AnimInst ? GetALSPropertyByteOrEnum(AnimInst, FName("OverlayState")) : 255;
	const FString AnimClassName = AnimInst ? AnimInst->GetClass()->GetName() : TEXT("None");

	UE_LOG(LogTemp, Warning, TEXT("================ [ALS 出生雷达] ================"));
	UE_LOG(LogTemp, Warning, TEXT("角色: %s | 初始 OverlayState: %d | RotationMode: %d | DesiredGait: %d"), *GetName(), InitOverlay, InitRotMode, InitGait);
	UE_LOG(LogTemp, Warning, TEXT("动画蓝图: %s | 初始 Anim_OverlayState: %d"), *AnimClassName, AnimOverlay);
	UE_LOG(LogTemp, Warning, TEXT("================================================"));

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Yellow,
			FString::Printf(TEXT("🌟 [出生状态] 角色Overlay: %d | 动画Overlay: %d | 旋转模式: %d | 动画蓝图: %s"),
				InitOverlay, AnimOverlay, InitRotMode, *AnimClassName));
	}

	// 监听生命组件的死亡事件驱动小队顺切或倒地
	if (HealthComponent)
	{
		HealthComponent->OnDeath.AddDynamic(this, &ALSCharacterBase::HandleHealthComponentDeath);
	}
}

void ALSCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 实时在屏幕左上角打印 ALS 状态监测雷达（覆盖刷新，不刷屏）
	if (GEngine && IsLocallyControlled())
	{
		const uint8 CharOverlay = GetALSPropertyByteOrEnum(this, FName("OverlayState"));
		const uint8 CharRotMode = GetALSPropertyByteOrEnum(this, FName("RotationMode"));
		const uint8 CharGait = GetALSPropertyByteOrEnum(this, FName("DesiredGait"));
		
		UAnimInstance* AnimInst = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
		const uint8 AnimOverlay = AnimInst ? GetALSPropertyByteOrEnum(AnimInst, FName("OverlayState")) : 255;
		const FString AnimClassName = AnimInst ? AnimInst->GetClass()->GetName() : TEXT("None");

		GEngine->AddOnScreenDebugMessage(9999, 0.0f, FColor::Cyan,
			FString::Printf(TEXT("📊 [ALS实时雷达] 角色Overlay=%d | 动画BP_Overlay=%d | 旋转模式=%d | 步态Gait=%d | 动画蓝图=%s"),
				CharOverlay, AnimOverlay, CharRotMode, CharGait, *AnimClassName));
	}
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