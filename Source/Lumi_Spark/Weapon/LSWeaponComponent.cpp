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

	// 1. 严格遵循数据驱动：完全使用在 UE 蓝图细节面板配置的 HandSocketName
	// 无论你在蓝图里配 WeaponSocket、PistolSocket 还是任何自定义插槽，C++ 绝不强行越权覆盖！
	FName TargetSocket = SocketName;
	if (!Char->GetMesh()->DoesSocketExist(TargetSocket))
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Yellow,
				FString::Printf(TEXT("⚠️ [LSWeaponComponent] 骨骼未找到配置的插槽 '%s'！请检查角色蓝图细节面板的 HandSocketName 配置。"), *TargetSocket.ToString()));
		}

		// 仅在指定插槽完全不存在时，进行健壮性保底回退（防止武器掉在世界原点）
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
		else
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Red,
					TEXT("❌ [LSWeaponComponent] 角色骨骼网格体上未找到任何可用的手部骨骼或插槽！"));
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
