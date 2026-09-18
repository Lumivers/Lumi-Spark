#include "Character/LSTeamSwitchComponent.h"
#include "Character/LSCharacterBase.h"
#include "Character/LSMovementComponent.h"
#include "Weapon/LSWeaponComponent.h"
#include "Core/LSEventBus.h"
#include "GameFramework/PlayerController.h"
#include "Core/LSPlayerController.h"
#include "UI/LSUHDWidget.h"

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

void ULSTeamSwitchComponent::SetupTeam(ALSCharacterBase* PrimaryCharacter, const TArray<TSubclassOf<ALSCharacterBase>>& StandbyClasses)
{
	if (!PrimaryCharacter || !GetWorld()) return;

	TeamMembers.Empty();
	TeamMembers.Add(PrimaryCharacter);
	ActiveIndex = 0;

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.Owner = GetOwner();

		const FVector SpawnLoc = PrimaryCharacter->GetActorLocation();
		const FRotator SpawnRot = PrimaryCharacter->GetActorRotation();

		for (TSubclassOf<ALSCharacterBase> StandbyClass : StandbyClasses)
		{
			if (!StandbyClass) continue;
			if (ALSCharacterBase* StandbyChar = GetWorld()->SpawnActor<ALSCharacterBase>(StandbyClass, SpawnLoc, SpawnRot, SpawnParams))
			{
				TeamMembers.Add(StandbyChar);
				StandbyChar->EnterBackgroundMode(); // 立即进入后台休眠
			}
		}
	}
}

bool ULSTeamSwitchComponent::CanSwitch() const
{
	// 1. 冷却中无法切换
	if (CooldownTimer > 0.0f) return false;

	// 2. 小队成员不足
	if (TeamMembers.Num() < 2) return false;

	// 3. 当前在场角色若处于死亡状态不可普通对调
	ALSCharacterBase* CurrentChar = GetActiveCharacter();
	if (CurrentChar && CurrentChar->IsDead()) return false;

	// 4. 检查是否存在至少一个合法的存活待命队友
	for (int32 i = 0; i < TeamMembers.Num(); ++i)
	{
		if (i != ActiveIndex && CanSwitchToIndex(i))
		{
			return true;
		}
	}

	return false;
}

bool ULSTeamSwitchComponent::ToggleCharacter()
{
	return CycleNextCharacter(true);
}

bool ULSTeamSwitchComponent::SwitchTo(int32 TargetIndex)
{
	if (!CanSwitchToIndex(TargetIndex)) return false;

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
	const int32 OldIndex = ActiveIndex;
	ActiveIndex = NewIndex;
	CooldownTimer = SwitchCooldown;
	SetComponentTickEnabled(true); // 开启 Tick 走 CD 衰减

	// 广播本地委托与跨系统全局事件总线
	OnTeamSwitch.Broadcast(OutChar, InChar, ActiveIndex);

	if (ULSEventBus* EventBus = ULSEventBus::Get(this))
	{
		EventBus->OnCharacterSwitched.Broadcast(OldIndex, ActiveIndex);
	}

	// 通知 playerController拥有的HUD重新绑定新角色
	if (ALSPlayerController* LSPc = Cast<ALSPlayerController>(PC))
	{
		if (ULSHUDWidget* HUDWidget = Cast<ULSHUDWidget>(LSPc->HUDWidgetInstance))
		{
			HUDWidget->BindToCharacter(InChar);
			HUDWidget->OnActiveCharacterSwitched(InChar, ActiveIndex);
		}
	}
}

ALSCharacterBase* ULSTeamSwitchComponent::GetActiveCharacter() const
{
	return TeamMembers.IsValidIndex(ActiveIndex) ? TeamMembers[ActiveIndex].Get() : nullptr;
}

ALSCharacterBase* ULSTeamSwitchComponent::GetInactiveCharacter() const
{
	for (int32 i = 0; i < TeamMembers.Num(); ++i)
	{
		if (i != ActiveIndex && TeamMembers.IsValidIndex(i))
		{
			return TeamMembers[i].Get();
		}
	}
	return nullptr;
}

bool ULSTeamSwitchComponent::CanSwitchToIndex(int32 TargetIndex) const
{
	if (CooldownTimer > 0.0f) return false;
	if (!TeamMembers.IsValidIndex(TargetIndex)) return false;
	if (TargetIndex == ActiveIndex) return false;

	ALSCharacterBase* TargetChar = TeamMembers[TargetIndex];
	if (!TargetChar || TargetChar->IsDead()) return false;

	return true;
}

bool ULSTeamSwitchComponent::CycleNextCharacter(bool bForward)
{
	if (TeamMembers.Num() < 2 || CooldownTimer > 0.0f) return false;

	const int32 Total = TeamMembers.Num();
	const int32 Step = bForward ? 1 : -1;

	// 环形寻址寻找下一个存活队友
	for (int32 i = 1; i < Total; ++i)
	{
		int32 CandidateIndex = (ActiveIndex + i * Step + Total * 10) % Total;
		if (CanSwitchToIndex(CandidateIndex))
		{
			return SwitchTo(CandidateIndex);
		}
	}
	return false;
}

ALSCharacterBase* ULSTeamSwitchComponent::GetCharacterAtIndex(int32 Index) const
{
	return TeamMembers.IsValidIndex(Index) ? TeamMembers[Index] : nullptr;
}