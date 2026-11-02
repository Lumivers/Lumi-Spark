#include "LSWeaponComponent.h"
#include "Weapon/LSWeaponBase.h"
#include "GameFramework/Character.h"
#include "Character/LSCharacterBase.h"
#include "Net/UnrealNetwork.h"

ULSWeaponComponent::ULSWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void ULSWeaponComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// 客户端严禁自行 Spawn 武器，必须由服务端生成并通过引擎复制给客户端
	if (DefaultPrimaryClass)
	{
		CurrentWeapon = SpawnWeapon(DefaultPrimaryClass);
		UE_LOG(LogTemp, Log, TEXT("🔫 [LSWeaponComponent] 默认武器生成: %s"), *GetNameSafe(CurrentWeapon));
	}
	if (CurrentWeapon)
	{
		AttachWeaponToSocket(CurrentWeapon, HandSocketName);
		OnWeaponChanged.Broadcast(CurrentWeapon);
	}
}

ALSWeaponBase* ULSWeaponComponent::SpawnWeapon(TSubclassOf<ALSWeaponBase> WeaponClass)
{
	if (!WeaponClass || !GetWorld()) return nullptr;
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = Cast<APawn>(GetOwner());
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	return GetWorld()->SpawnActor<ALSWeaponBase>(WeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
}

void ULSWeaponComponent::AttachWeaponToSocket(ALSWeaponBase* Weapon, FName SocketName)
{
	ACharacter* Char = Cast<ACharacter>(GetOwner());
	if (!Weapon || !Char || !Char->GetMesh()) return;
	
	// 彻底防止手持武器的物理碰撞体反推、挤压角色自身胶囊体
	Char->MoveIgnoreActorAdd(Weapon);

	// 确立武器的所有权与施加者归属
	Weapon->SetOwner(Char);
	Weapon->SetInstigator(Char);

	// 1. 优先检测是否具备 ALS 原生预先精确校正好的持枪挂点（VB RHS_ik_hand_gun）
	// 免去人工在骨骼编辑器里反复肉眼旋转微调插槽角度的折磨
	FName TargetSocket = SocketName;
	if (Char->GetMesh()->DoesSocketExist(FName("VB RHS_ik_hand_gun")))
	{
		TargetSocket = FName("VB RHS_ik_hand_gun");
	}
	else if (TargetSocket.IsNone() || TargetSocket == FName("hand_r"))
	{
		if (Char->GetMesh()->DoesSocketExist(FName("WeaponSocket")))
		{
			TargetSocket = FName("WeaponSocket");
		}
	}

	// 2. 健壮性检查：若指定的插槽在当前网格体上不存在（例如换了新骨骼还没建 WeaponSocket），自动降级回退
	// 防止 UE 因找不到插槽而默认吸附到 Mesh 原点 (0,0,0)，导致武器掉在角色脚底
	if (!Char->GetMesh()->DoesSocketExist(TargetSocket))
	{
		if (Char->GetMesh()->DoesSocketExist(FName("hand_r")))
		{
			TargetSocket = FName("hand_r");
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 6.0f, FColor::Yellow,
					TEXT("⚠️ [LSWeaponComponent] 骨骼未找到 'WeaponSocket' 插槽，已保底吸附至 'hand_r'！请在骨骼编辑器中为 hand_r 添加 WeaponSocket 并对齐握姿。"));
			}
		}
		else if (Char->GetMesh()->DoesSocketExist(FName("hand_attach_r")))
		{
			TargetSocket = FName("hand_attach_r");
		}
		else
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 6.0f, FColor::Red,
					TEXT("❌ [LSWeaponComponent] 角色骨骼网格体上未找到任何可用的手部骨骼或插槽！"));
			}
		}
	}

	// 吸附到骨骼插槽，保留原始缩放
	Weapon->AttachToComponent(Char->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, TargetSocket);
	UE_LOG(LogTemp, Warning, TEXT("🔫 [LSWeaponComponent] 武器 %s 已吸附至插槽: %s"),
		*Weapon->GetName(), *TargetSocket.ToString());
}

void ULSWeaponComponent::StartFire()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StartFire();
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Red, 
			FString::Printf(TEXT("❌ CurrentWeapon 为空！Owner=%s | Comp=%p | DefaultClass=%s"),
				GetOwner() ? *GetOwner()->GetName() : TEXT("None"),
				this,
				DefaultPrimaryClass ? *DefaultPrimaryClass->GetName() : TEXT("None")));
	}
}

void ULSWeaponComponent::StopFire()
{
	if (CurrentWeapon) CurrentWeapon->StopFire();
}

void ULSWeaponComponent::Reload()
{
	if (CurrentWeapon) CurrentWeapon->Reload();
}

// 客户端接收服务端同步：将手上的枪换到手上
void ULSWeaponComponent::OnRep_CurrentWeapon(ALSWeaponBase* OldWeapon)
{
	if (CurrentWeapon)
	{
		AttachWeaponToSocket(CurrentWeapon, HandSocketName);
	}
	OnWeaponChanged.Broadcast(CurrentWeapon);
}

//注册同步属性
void ULSWeaponComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ULSWeaponComponent, CurrentWeapon);
}
