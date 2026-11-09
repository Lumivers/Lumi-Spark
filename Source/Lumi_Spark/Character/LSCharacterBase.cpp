#include "LSCharacterBase.h"
#include "Character/LSCameraComponent.h"
#include "Character/LSMovementComponent.h"
#include "Character/LSSkillComponent.h"
#include "Character/LSHealthComponent.h"
#include "Character/LSStaminaComponent.h"
#include "Character/LSEnergyComponent.h"
#include "Core/LSPlayerController.h"
#include "Weapon/LSWeaponComponent.h"
#include "Element/LSElementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/SpringArmComponent.h"
#include "TimerManager.h"
#include "Weapon/LSThrowableComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"

#define LS_DLL_BUILD_TIMESTAMP TEXT(__DATE__ " " __TIME__)

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

	static void SetALSPropertyInt(UObject* Target, FName PropName, int32 Value)
	{
		if (!Target) return;
		if (FProperty* Prop = Target->GetClass()->FindPropertyByName(PropName))
		{
			if (FIntProperty* IntProp = CastField<FIntProperty>(Prop))
			{
				IntProp->SetPropertyValue_InContainer(Target, Value);
				UE_LOG(LogTemp, Log, TEXT("[SetALSPropertyInt] SUCCESS: %s.%s = %d (FIntProperty)"), *Target->GetName(), *PropName.ToString(), Value);
				return;
			}
			else if (FNumericProperty* NumProp = CastField<FNumericProperty>(Prop))
			{
				void* ValuePtr = NumProp->ContainerPtrToValuePtr<void>(Target);
				NumProp->SetIntPropertyValue(ValuePtr, static_cast<int64>(Value));
				UE_LOG(LogTemp, Log, TEXT("[SetALSPropertyInt] SUCCESS: %s.%s = %d (FNumericProperty)"), *Target->GetName(), *PropName.ToString(), Value);
				return;
			}
		}
	}

	static int32 GetALSPropertyInt(const UObject* Target, FName PropName)
	{
		if (!Target) return -1;
		if (FProperty* Prop = Target->GetClass()->FindPropertyByName(PropName))
		{
			if (FIntProperty* IntProp = CastField<FIntProperty>(Prop))
			{
				return IntProp->GetPropertyValue_InContainer(Target);
			}
			else if (FNumericProperty* NumProp = CastField<FNumericProperty>(Prop))
			{
				const void* ValuePtr = NumProp->ContainerPtrToValuePtr<void>(Target);
				return static_cast<int32>(NumProp->GetSignedIntPropertyValue(ValuePtr));
			}
		}
		return -1;
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

	// 战术射击模式默认启用
	bAlwaysOrientToControlRotation = true;
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
		// 战术射击模式：瞄准/开火进入模式 2 (Aiming 端枪抵肩)；平时走动保持模式 1 (LookingDirection 战术巡逻横移，正对准星上半身稳定)
		SetALSRotationMode(bEnable ? 2 : 1);
		return;
	}

	bUseControllerRotationYaw = false;
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		// 自由冒险模式：战斗时锁定准星横移，非战斗时允许角色面向移动方向自由走跑
		MoveComp->bOrientRotationToMovement = !bEnable;
		MoveComp->bUseControllerDesiredRotation = false;
		MoveComp->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
	}

	// 自由冒险模式下：战斗为 2 (Aiming)，平时恢复 0 (VelocityDirection)
	SetALSRotationMode(bEnable ? 2 : 0);
}

void ALSCharacterBase::SetALSRotationMode(uint8 InRotationMode)
{
	// 1. 直接同步反射属性，确保 ALS 蓝图内部变量立即响应
	SetALSPropertyByteOrEnum(this, FName("RotationMode"), InRotationMode);
	SetALSPropertyByteOrEnum(this, FName("DesiredRotationMode"), InRotationMode);

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

void ALSCharacterBase::SetALSOverlayOverrideState(int32 InOverrideState)
{
	// 1. 同步 Character 自身的反射属性
	SetALSPropertyInt(this, FName("OverlayOverrideState"), InOverrideState);

	static const FName FuncName(TEXT("BPI_SetOverlayOverrideState"));
	if (UFunction* CharFunc = FindFunction(FuncName))
	{
		struct FParams
		{
			int32 State;
		} Params;
		Params.State = InOverrideState;
		ProcessEvent(CharFunc, &Params);
	}

	// 2. 同步 AnimInstance (ALS_AnimBP) 上的 OverlayOverrideState 并触发动画接口事件
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		if (UAnimInstance* AnimInst = MeshComp->GetAnimInstance())
		{
			SetALSPropertyInt(AnimInst, FName("OverlayOverrideState"), InOverrideState);

			if (UFunction* AnimFunc = AnimInst->FindFunction(FuncName))
			{
				struct FParams
				{
					int32 State;
				} Params;
				Params.State = InOverrideState;
				AnimInst->ProcessEvent(AnimFunc, &Params);
				UE_LOG(LogTemp, Log, TEXT("🎯 [SetALSOverlayOverrideState] 成功派发 BPI_SetOverlayOverrideState(%d) 至 %s"),
					InOverrideState, *AnimInst->GetName());
			}
		}
	}
}

void ALSCharacterBase::AddSpineRecoilKick(float Multiplier)
{
	if (Multiplier <= 0.0f)
	{
		return;
	}

	// 1. 真实枪械后坐力解算：
	// Pitch (向上仰角受力) + 微量 Yaw (微弱随机左右偏摆) + Roll (微弱侧倾)
	const float PitchKick = RecoilSpinePitch * Multiplier;
	const float YawKick = FMath::FRandRange(-RecoilSpineYaw, RecoilSpineYaw) * Multiplier;
	const float RollKick = FMath::FRandRange(-RecoilSpineRoll, RecoilSpineRoll) * Multiplier;
	const float BackKick = RecoilSpineBack * Multiplier;

	SpineRecoilRotation.Pitch = FMath::Clamp(SpineRecoilRotation.Pitch + PitchKick, -6.0f, 6.0f);
	SpineRecoilRotation.Yaw = FMath::Clamp(SpineRecoilRotation.Yaw + YawKick, -2.5f, 2.5f);
	SpineRecoilRotation.Roll = FMath::Clamp(SpineRecoilRotation.Roll + RollKick, -1.5f, 1.5f);

	// 2. 躯干向后抵肩受力位移 (Location.X 微退)
	SpineRecoilLocation.X = FMath::Clamp(SpineRecoilLocation.X - BackKick, -2.5f, 0.0f);

	// 屏幕调试提示：实时监测开火冲量触发
	if (GEngine && IsLocallyControlled())
	{
		GEngine->AddOnScreenDebugMessage(8877, 0.4f, FColor::Yellow,
			FString::Printf(TEXT("💥 [后坐冲量爆发] 仰角=%.1f° | 偏摆=%.1f°"), SpineRecoilRotation.Pitch, SpineRecoilRotation.Yaw));
	}
}

void ALSCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	// 战术射击游戏：初始化即进入战术朝向模式（平时 LookingDirection(1) 巡逻横移，开火/瞄准 Aiming(2) 抵肩瞄准）
	bAlwaysOrientToControlRotation = true;
	SetCombatFacing(false);
	SetALSDesiredGait(0); // 核心修复：出生直接锁定为 Gait 0（战术持枪慢步），彻底消除 Gait 1 奔跑甩枪动画！

	// 🔍 [Debug 监测] 打印出生时蓝图读取到的初始状态
	const uint8 InitOverlay = GetALSPropertyByteOrEnum(this, FName("OverlayState"));
	const uint8 InitRotMode = GetALSPropertyByteOrEnum(this, FName("RotationMode"));
	const uint8 InitGait = GetALSPropertyByteOrEnum(this, FName("DesiredGait"));
	UAnimInstance* AnimInst = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	const uint8 AnimOverlay = AnimInst ? GetALSPropertyByteOrEnum(AnimInst, FName("OverlayState")) : 255;
	const int32 AnimOverride = AnimInst ? GetALSPropertyInt(AnimInst, FName("OverlayOverrideState")) : -1;
	const FString AnimClassName = AnimInst ? AnimInst->GetClass()->GetName() : TEXT("None");

	// 记录会话日志与 DLL 构建版本
	const FString FullLogPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectLogDir() / TEXT("Lumi_Spark.log"));
	const FString SessionDir = FPaths::ProjectSavedDir() / TEXT("SessionLogs");
	IFileManager::Get().MakeDirectory(*SessionDir, true);
	const FString TimeStr = FDateTime::Now().ToString(TEXT("%Y-%m-%d %H:%M:%S"));
	const FString FileTimeStr = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
	const FString SessionInfo = FString::Printf(TEXT("========================================\n"
		"会话启动时间: %s\n"
		"DLL 构建时间: %s\n"
		"引擎日志路径: %s\n"
		"角色: %s | 初始步态: %d | 旋转模式: %d\n"
		"========================================\n"),
		*TimeStr, LS_DLL_BUILD_TIMESTAMP, *FullLogPath, *GetName(), InitGait, InitRotMode);
	FFileHelper::SaveStringToFile(SessionInfo, *(SessionDir / TEXT("Latest_Session.txt")));
	FFileHelper::SaveStringToFile(SessionInfo, *(SessionDir / FString::Printf(TEXT("Session_%s.txt"), *FileTimeStr)));

	UE_LOG(LogTemp, Warning, TEXT("================ [ALS 出生雷达] ================"));
	UE_LOG(LogTemp, Warning, TEXT("📦 [DLL 构建版本] %s"), LS_DLL_BUILD_TIMESTAMP);
	UE_LOG(LogTemp, Warning, TEXT("📁 [日志文件路径] %s"), *FullLogPath);
	UE_LOG(LogTemp, Warning, TEXT("角色: %s | 初始 OverlayState: %d | RotationMode: %d | DesiredGait: %d"), *GetName(), InitOverlay, InitRotMode, InitGait);
	UE_LOG(LogTemp, Warning, TEXT("动画蓝图: %s | 初始 Anim_OverlayState: %d | OverrideState: %d"), *AnimClassName, AnimOverlay, AnimOverride);
	UE_LOG(LogTemp, Warning, TEXT("================================================"));

	if (GEngine && IsLocallyControlled())
	{
		GEngine->AddOnScreenDebugMessage(9997, 15.0f, FColor::Emerald,
			FString::Printf(TEXT("📦 [DLL 构建版本] %s"), LS_DLL_BUILD_TIMESTAMP));
		GEngine->AddOnScreenDebugMessage(9996, 12.0f, FColor::Green,
			FString::Printf(TEXT("📁 [当前日志] %s"), *FullLogPath));
		GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Yellow,
			FString::Printf(TEXT("🌟 [出生状态] 角色Overlay: %d | 动画Overlay: %d | 手臂锁定Override: %d | 旋转模式: %d"),
				InitOverlay, AnimOverlay, AnimOverride, InitRotMode));
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

	// 平滑弹性回正躯干脊椎开火后坐力抖动
	const float EffectiveRecovery = 14.0f;
	SpineRecoilRotation = FMath::RInterpTo(SpineRecoilRotation, FRotator::ZeroRotator, DeltaTime, EffectiveRecovery);
	SpineRecoilLocation = FMath::VInterpTo(SpineRecoilLocation, FVector::ZeroVector, DeltaTime, EffectiveRecovery);

	// 实时在屏幕左上角打印 ALS 状态监测雷达（覆盖刷新，不刷屏）
	if (GEngine && IsLocallyControlled())
	{
		const uint8 CharOverlay = GetALSPropertyByteOrEnum(this, FName("OverlayState"));
		const uint8 CharRotMode = GetALSPropertyByteOrEnum(this, FName("RotationMode"));
		const uint8 CharGait = GetALSPropertyByteOrEnum(this, FName("DesiredGait"));
		
		UAnimInstance* AnimInst = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
		const uint8 AnimOverlay = AnimInst ? GetALSPropertyByteOrEnum(AnimInst, FName("OverlayState")) : 255;
		const int32 AnimOverride = AnimInst ? GetALSPropertyInt(AnimInst, FName("OverlayOverrideState")) : -1;
		const FString AnimClassName = AnimInst ? AnimInst->GetClass()->GetName() : TEXT("None");

		GEngine->AddOnScreenDebugMessage(9999, 0.0f, FColor::Cyan,
			FString::Printf(TEXT("📊 [雷达|DLL:%s] Overlay=%d | 锁臂=%d | 旋转=%d | 步态Gait=%d"),
				LS_DLL_BUILD_TIMESTAMP, CharOverlay, AnimOverride, CharRotMode, CharGait));
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
	Die(nullptr);
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