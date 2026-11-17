#include "LSWeaponComponent.h"
#include "Weapon/LSWeaponBase.h"
#include "Weapon/Weapons/LSRifle.h"
#include "Weapon/Weapons/LSShotgun.h"
#include "GameFramework/Character.h"
#include "Character/LSCharacterBase.h"
#include "Net/UnrealNetwork.h"

ULSWeaponComponent::ULSWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	// 默认双枪配置：1 号位脉冲突击步枪（火 Pyro），2 号位寒霜重型霰弹枪（冰 Cryo 2U）
	DefaultPrimaryClass = ALSRifle::StaticClass();
	DefaultSecondaryClass = ALSShotgun::StaticClass();
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
	
	// 彻底防止武器的物理碰撞体反推、挤压角色自身胶囊体
	Char->MoveIgnoreActorAdd(Weapon);

	// 确立武器的所有权与施加者归属
	Weapon->SetOwner(Char);
	Weapon->SetInstigator(Char);

	// 1. 若为副手/背部收纳请求
	if (SocketName == BackSocketName)
	{
		// 仅当骨骼资产中显式创建了对齐好的背部插槽（如 Weapon_Back）时才吸附挂载
		// 严禁直接吸附到 spine 骨骼原点，防止武器横向穿胸刺出
		if (Char->GetMesh()->DoesSocketExist(BackSocketName))
		{
			Weapon->SetActorHiddenInGame(false);
			Weapon->AttachToComponent(Char->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, BackSocketName);
		}
		else
		{
			// 无背部专用插槽时，采用现代战术射击标准做法：安全隐藏未持握武器
			Weapon->SetActorHiddenInGame(true);
			Weapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		}
		return;
	}

	// 2. 若为主手持握请求：必须显式显示
	Weapon->SetActorHiddenInGame(false);

	FName HandSocket = SocketName;
	if (!Char->GetMesh()->DoesSocketExist(HandSocket))
	{
		if (Char->GetMesh()->DoesSocketExist(FName("WeaponSocket")))
		{
			HandSocket = FName("WeaponSocket");
		}
		else if (Char->GetMesh()->DoesSocketExist(FName("hand_rSocket")))
		{
			HandSocket = FName("hand_rSocket");
		}
		else if (Char->GetMesh()->DoesSocketExist(FName("hand_r")))
		{
			HandSocket = FName("hand_r");
		}
	}

	Weapon->AttachToComponent(Char->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, HandSocket);
	Weapon->ResetAttachTransformCache();
	UE_LOG(LogTemp, Log, TEXT("🔫 [LSWeaponComponent] 武器 %s 已手持吸附至插槽: %s 并刷新变换基准"), *Weapon->GetName(), *HandSocket.ToString());
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
