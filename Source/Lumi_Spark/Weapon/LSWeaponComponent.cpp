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
	if (DefaultPrimaryClass)
	{
		CurrentWeapon = SpawnWeapon(DefaultPrimaryClass);
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

	// 优先寻找专门调整过抵肩旋转与握把偏移的 WeaponSocket，如果未配置或不存在则尝试保底
	FName TargetSocket = SocketName;
	if (TargetSocket.IsNone() || TargetSocket == FName("hand_r"))
	{
		if (Char->GetMesh()->DoesSocketExist(FName("WeaponSocket")))
		{
			TargetSocket = FName("WeaponSocket");
		}
	}

	// 吸附到骨骼插槽，保留原始缩放
	Weapon->AttachToComponent(Char->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, TargetSocket);
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