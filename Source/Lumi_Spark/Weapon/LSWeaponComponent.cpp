#include "LSWeaponComponent.h"
#include "Weapon/LSWeaponBase.h"
#include "GameFramework/Character.h"
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
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	// 1. 生成主副武器
	if (DefaultPrimaryClass) PrimaryWeapon = SpawnWeapon(DefaultPrimaryClass);
	if (DefaultSecondaryClass) SecondaryWeapon = SpawnWeapon(DefaultSecondaryClass);
    
	// 2. 默认装备主武器
	if (PrimaryWeapon)
	{
		CurrentWeapon = PrimaryWeapon;
		CurrentSlot = ELSWeaponSlot::MainWeapon;
		AttachWeaponToSocket(CurrentWeapon, HandSocketName);
		OnWeaponChanged.Broadcast(CurrentWeapon);
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("❌ PrimaryWeapon Spawn 失败或 DefaultPrimaryClass 为空！"));
	}

	if (SecondaryWeapon)
	{
		AttachWeaponToSocket(SecondaryWeapon, HolsterSocketName);
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
	
	//吸附到骨骼插槽，保留原始缩放
	Weapon->AttachToComponent(Char->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
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

void ULSWeaponComponent::EquipWeapon(ELSWeaponSlot NewSlot)
{
	//如果当前槽位就是目标槽位，或者目标槽位无效，直接返回
	if (NewSlot == CurrentSlot || NewSlot == ELSWeaponSlot::None) return;
	
	// 若当前是客户端，向服务端发送 RPC 请求
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		Server_EquipWeapon(NewSlot);
		return;
	}
	
	ALSWeaponBase* PendingWeapon = (NewSlot == ELSWeaponSlot::MainWeapon) ? PrimaryWeapon : SecondaryWeapon;
	if (!PendingWeapon) return;
	
	//收枪前先停止旧武器射击
	if (CurrentWeapon)
	{
		CurrentWeapon->StopFire();
		AttachWeaponToSocket(CurrentWeapon, HolsterSocketName);
	}
	
	//新枪拔出并挂载到右手
	CurrentWeapon = PendingWeapon;
	CurrentSlot = NewSlot;
	AttachWeaponToSocket(CurrentWeapon, HandSocketName);
	
	OnWeaponChanged.Broadcast(CurrentWeapon);
}

void ULSWeaponComponent::QuickSwitchWeapon()
{
	const ELSWeaponSlot TargetSlot = (CurrentSlot == ELSWeaponSlot::MainWeapon) ? ELSWeaponSlot::SubWeapon : ELSWeaponSlot::MainWeapon;
	EquipWeapon(TargetSlot);
}

void ULSWeaponComponent::Server_EquipWeapon_Implementation(ELSWeaponSlot NewSlot)
{
	EquipWeapon(NewSlot);
}

// 客户端接收服务端同步：将手上的枪换到手上，旧枪挂回背部
void ULSWeaponComponent::OnRep_CurrentWeapon(ALSWeaponBase* OldWeapon)
{
	if (OldWeapon)
	{
		AttachWeaponToSocket(OldWeapon, HolsterSocketName);
	}
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
	
	DOREPLIFETIME(ULSWeaponComponent, PrimaryWeapon);
	DOREPLIFETIME(ULSWeaponComponent, SecondaryWeapon);
	DOREPLIFETIME(ULSWeaponComponent, CurrentWeapon);
	DOREPLIFETIME(ULSWeaponComponent, CurrentSlot);
}