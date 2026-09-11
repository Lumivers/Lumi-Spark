#include "Character/LSTeamSwitchComponent.h"
#include "Character/LSCharacterBase.h"
#include "Character/LSMovementComponent.h"
#include "Weapon/LSWeaponComponent.h"
#include "Core/LSEventBus.h"
#include "GameFramework/PlayerController.h"

ULSTeamSwitchComponent::ULSTeamSwitchComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false; // 初始关闭 Tick，切人进入 CD 时才按需使能（极致性能）
}

void ULSTeamSwitchComponent::BeginPlay()
{
	Super::BeginPlay();
	TeamMembers.SetNumZeroed(2);
}

void ULSTeamSwitchComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 冷却计时递减
	if (CooldownTimer > 0.0f)
	{
		CooldownTimer -= DeltaTime;
		OnSwitchCooldownChanged.Broadcast(FMath::Max(0.0f, CooldownTimer), SwitchCooldown);

		if (CooldownTimer <= 0.0f)
		{
			CooldownTimer = 0.0f;
			SetComponentTickEnabled(false); // CD 恢复，关闭 Tick 消除 CPU 消耗
		}
	}
}

void ULSTeamSwitchComponent::SetupTeam(ALSCharacterBase* PrimaryCharacter, TSubclassOf<ALSCharacterBase> SecondaryClass)
{
	if (!PrimaryCharacter || !GetWorld()) return;

	TeamMembers[0] = PrimaryCharacter;
	ActiveIndex = 0;

	// 服务端权威生成副角色（Slot 1），并立刻送入后台休眠
	if (GetOwner() && GetOwner()->HasAuthority() && SecondaryClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.Owner = GetOwner();

		// 生成在主角色附近但隐藏并关闭碰撞
		FVector SpawnLoc = PrimaryCharacter->GetActorLocation();
		FRotator SpawnRot = PrimaryCharacter->GetActorRotation();

		if (ALSCharacterBase* SecondaryChar = GetWorld()->SpawnActor<ALSCharacterBase>(SecondaryClass, SpawnLoc, SpawnRot, SpawnParams))
		{
			TeamMembers[1] = SecondaryChar;
			SecondaryChar->EnterBackgroundMode(); // 立即隐身并进入后台休眠
		}
	}
}

bool ULSTeamSwitchComponent::CanSwitch() const
{
	// 1. 冷却中无法切换
	if (CooldownTimer > 0.0f) return false;

	// 2. 小队成员不齐或未初始化
	if (!TeamMembers.IsValidIndex(0) || !TeamMembers.IsValidIndex(1)) return false;
	if (!TeamMembers[0] || !TeamMembers[1]) return false;

	// 3. 待命角色若已死亡不可切出
	const int32 TargetIndex = (ActiveIndex == 0) ? 1 : 0;
	ALSCharacterBase* TargetChar = TeamMembers[TargetIndex];
	if (!TargetChar || TargetChar->IsDead()) return false;

	// 4. 当前在场角色若处于死亡状态不可普通对调
	ALSCharacterBase* CurrentChar = TeamMembers[ActiveIndex];
	if (CurrentChar && CurrentChar->IsDead()) return false;

	return true;
}

bool ULSTeamSwitchComponent::ToggleCharacter()
{
	const int32 TargetIndex = (ActiveIndex == 0) ? 1 : 0;
	return SwitchTo(TargetIndex);
}

bool ULSTeamSwitchComponent::SwitchTo(int32 TargetIndex)
{
	if (TargetIndex == ActiveIndex) return false;
	if (!CanSwitch()) return false;

	ALSCharacterBase* OutChar = TeamMembers[ActiveIndex];
	ALSCharacterBase* InChar = TeamMembers[TargetIndex];

	if (!OutChar || !InChar) return false;

	PerformSwitch(OutChar, InChar, TargetIndex);
	return true;
}

void ULSTeamSwitchComponent::PerformSwitch(ALSCharacterBase* OutChar, ALSCharacterBase* InChar, int32 NewIndex)
{
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC) return;

	// ─── 1. 锁死并备份当前的竞技核心参数 ───
	const FRotator SavedControlRot = PC->GetControlRotation(); // 准星视角绝对保存
	const FVector SavedVelocity = OutChar->GetVelocity();     // 当前移动动量向量
	const FVector SavedLocation = OutChar->GetActorLocation(); // 空间绝对坐标

	// ─── 2. 退场角色清理 ───
	// 停火并打断开镜
	if (ULSWeaponComponent* OutWeaponComp = OutChar->GetWeaponComponent())
	{
		OutWeaponComp->StopFire();
	}
	OutChar->EnterBackgroundMode(); // 隐身、关碰撞、后台轻量更新

	// ─── 3. 入场角色位置与物理状态移交 ───
	InChar->SetActorLocation(SavedLocation);
	if (bInheritAimDirection)
	{
		InChar->SetActorRotation(FRotator(0.f, SavedControlRot.Yaw, 0.f));
	}

	InChar->ExitBackgroundMode(); // 显形、恢复碰撞、激活第一人称专属手臂

	// 动量无缝注入：如果之前在冲刺/滑铲，新角色直接继承速度，绝对不卡顿！
	if (bInheritVelocity)
	{
		if (UCharacterMovementComponent* InMove = InChar->GetCharacterMovement())
		{
			InMove->Velocity = SavedVelocity;
		}
	}

	// ─── 4. 控制器 Possession 交接 ───
	PC->UnPossess();
	PC->Possess(InChar);

	// 瞬间恢复 ControlRotation，保障第一人称视野零跳动
	PC->SetControlRotation(SavedControlRot);

	// ─── 5. 状态同步与事件广播 ───
	ActiveIndex = NewIndex;
	CooldownTimer = SwitchCooldown;
	SetComponentTickEnabled(true); // 开启 Tick 走 CD 衰减

	// 广播本地委托与跨系统全局事件总线
	OnTeamSwitch.Broadcast(OutChar, InChar, ActiveIndex);

	if (ULSEventBus* EventBus = ULSEventBus::Get(this))
	{
		EventBus->OnCharacterSwitched.Broadcast((ActiveIndex == 0) ? 1 : 0, ActiveIndex);
	}
}

ALSCharacterBase* ULSTeamSwitchComponent::GetActiveCharacter() const
{
	return TeamMembers.IsValidIndex(ActiveIndex) ? TeamMembers[ActiveIndex].Get() : nullptr;
}

ALSCharacterBase* ULSTeamSwitchComponent::GetInactiveCharacter() const
{
	const int32 InactiveIdx = (ActiveIndex == 0) ? 1 : 0;
	return TeamMembers.IsValidIndex(InactiveIdx) ? TeamMembers[InactiveIdx].Get() : nullptr;
}