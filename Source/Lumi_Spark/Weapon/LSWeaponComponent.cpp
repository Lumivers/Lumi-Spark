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
	
	// 服务端或单机环境下统一实例化默认主副武器
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		if (DefaultPrimaryClass)
		{
			PrimaryWeapon = SpawnWeapon(DefaultPrimaryClass);
			UE_LOG(LogTemp, Log, TEXT("🔫 [LSWeaponComponent] 默认主武器生成: %s"), *GetNameSafe(PrimaryWeapon));
		}
		if (DefaultSecondaryClass)
		{
			SecondaryWeapon = SpawnWeapon(DefaultSecondaryClass);
			UE_LOG(LogTemp, Log, TEXT("🔫 [LSWeaponComponent] 默认副武器生成: %s"), *GetNameSafe(SecondaryWeapon));
		}

		if (PrimaryWeapon)
		{
			CurrentWeapon = PrimaryWeapon;
			CurrentWeaponIndex = 0;
			AttachWeaponToSocket(PrimaryWeapon, HandSocketName);
		}
		if (SecondaryWeapon)
		{
			AttachWeaponToSocket(SecondaryWeapon, BackSocketName);
		}

		if (CurrentWeapon)
		{
			OnWeaponChanged.Broadcast(CurrentWeapon);
		}
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

	// 1. 严格遵循数据驱动：完全使用在 UE 蓝图细节面板配置的 HandSocketName
	// 无论你在蓝图里配 WeaponSocket、PistolSocket 还是任何自定义插槽，C++ 绝不强行越权覆盖！
	FName TargetSocket = SocketName;
	if (!Char->GetMesh()->DoesSocketExist(TargetSocket))
	{
		// 如果是背部插槽不存在，按优先级寻找后背骨骼保底挂载
		if (SocketName == BackSocketName)
		{
			if (Char->GetMesh()->DoesSocketExist(FName("Weapon_Back")))
			{
				TargetSocket = FName("Weapon_Back");
			}
			else if (Char->GetMesh()->DoesSocketExist(FName("spine_05")))
			{
				TargetSocket = FName("spine_05");
			}
			else if (Char->GetMesh()->DoesSocketExist(FName("spine_03")))
			{
				TargetSocket = FName("spine_03");
			}
			else if (Char->GetMesh()->DoesSocketExist(FName("spine_02")))
			{
				TargetSocket = FName("spine_02");
			}
		}
		// 手部插槽保底
		else
		{
			if (Char->GetMesh()->DoesSocketExist(FName("WeaponSocket")))
			{
				TargetSocket = FName("WeaponSocket");
			}
			else if (Char->GetMesh()->DoesSocketExist(FName("hand_r")))
			{
				TargetSocket = FName("hand_r");
			}
			else if (Char->GetMesh()->DoesSocketExist(FName("hand_attach_r")))
			{
				TargetSocket = FName("hand_attach_r");
			}
		}
	}

	// 吸附到骨骼插槽，保留原始缩放
	Weapon->AttachToComponent(Char->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, TargetSocket);
	UE_LOG(LogTemp, Warning, TEXT("🔫 [LSWeaponComponent] 武器 %s 已吸附至插槽: %s"),
		*Weapon->GetName(), *TargetSocket.ToString());
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green,
			FString::Printf(TEXT("🔫 [LSWeaponComponent] 武器已成功吸附至插槽: %s"), *TargetSocket.ToString()));
	}
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

void ULSWeaponComponent::SwitchWeapon(int32 SlotIndex)
{
	if (SlotIndex == CurrentWeaponIndex) return;
	if (SlotIndex == 0)
	{
		EquipPrimaryWeapon();
	}
	else if (SlotIndex == 1)
	{
		EquipSecondaryWeapon();
	}
}

void ULSWeaponComponent::EquipPrimaryWeapon()
{
	if (!PrimaryWeapon || CurrentWeapon == PrimaryWeapon) return;
	StopFire();

	// 将副武器挂回背部
	if (SecondaryWeapon)
	{
		AttachWeaponToSocket(SecondaryWeapon, BackSocketName);
	}

	// 将主武器持于手中
	AttachWeaponToSocket(PrimaryWeapon, HandSocketName);

	CurrentWeapon = PrimaryWeapon;
	CurrentWeaponIndex = 0;
	OnWeaponChanged.Broadcast(CurrentWeapon);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(5512, 2.0f, FColor::Orange, TEXT("🔫 已切换至主武器 [1 号位]"));
	}
}

void ULSWeaponComponent::EquipSecondaryWeapon()
{
	if (!SecondaryWeapon || CurrentWeapon == SecondaryWeapon) return;
	StopFire();

	// 将主武器挂回背部
	if (PrimaryWeapon)
	{
		AttachWeaponToSocket(PrimaryWeapon, BackSocketName);
	}

	// 将副武器持于手中
	AttachWeaponToSocket(SecondaryWeapon, HandSocketName);

	CurrentWeapon = SecondaryWeapon;
	CurrentWeaponIndex = 1;
	OnWeaponChanged.Broadcast(CurrentWeapon);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(5512, 2.0f, FColor::Cyan, TEXT("🔫 已切换至副武器 [2 号位]"));
	}
}

void ULSWeaponComponent::ToggleWeapon()
{
	if (CurrentWeaponIndex == 0)
	{
		EquipSecondaryWeapon();
	}
	else
	{
		EquipPrimaryWeapon();
	}
}

void ULSWeaponComponent::OnRep_CurrentWeaponIndex()
{
	if (CurrentWeaponIndex == 0)
	{
		if (SecondaryWeapon) AttachWeaponToSocket(SecondaryWeapon, BackSocketName);
		if (PrimaryWeapon) AttachWeaponToSocket(PrimaryWeapon, HandSocketName);
	}
	else
	{
		if (PrimaryWeapon) AttachWeaponToSocket(PrimaryWeapon, BackSocketName);
		if (SecondaryWeapon) AttachWeaponToSocket(SecondaryWeapon, HandSocketName);
	}
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

// 注册同步属性
void ULSWeaponComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ULSWeaponComponent, PrimaryWeapon);
	DOREPLIFETIME(ULSWeaponComponent, SecondaryWeapon);
	DOREPLIFETIME(ULSWeaponComponent, CurrentWeapon);
	DOREPLIFETIME(ULSWeaponComponent, CurrentWeaponIndex);
}
