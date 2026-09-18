#include "LSCharacterBase.h"
#include "LSCameraComponent.h"
#include "LSMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "weapon/LSWeaponComponent.h"
#include "Weapon/LSWeaponBase.h"
#include "Element/LSElementComponent.h"
#include "Core/LSEventBus.h"
#include "Net/UnrealNetwork.h"
#include "Character/LSSkillComponent.h"
#include "Character/LSTeamSwitchComponent.h"
#include "Core/LSPlayerController.h"

// 构造函数：用自定义的ULSMovementComponent 替换默认的CharacterMovementComponent
ALSCharacterBase::ALSCharacterBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer.SetDefaultSubobjectClass<ULSMovementComponent>(ACharacter::CharacterMovementComponentName))
{
 	// 开启Tick
	PrimaryActorTick.bCanEverTick = true;
	
	//1,创建弹簧臂并插在角色眼部高度（0.0.65）
	USpringArmComponent* SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	SpringArm->SetupAttachment(GetCapsuleComponent());
	SpringArm->SetRelativeLocation(FVector(0.f, 0.f, 65.f)); //角色眼部高度
	SpringArm->TargetArmLength = 300.f; //默认第一人称，长度0
	SpringArm->SocketOffset = FVector(0.f, 50.f, 15.f); //右肩偏移
	SpringArm->bUsePawnControlRotation = true; //弹簧臂跟随控制器旋转
	SpringArm->bDoCollisionTest = true; //禁用弹簧臂碰撞检测，避免摄像机被遮挡
	
	//1. 创建摄像机组件并附加到根碰撞胶囊体
	CameraComponent = CreateDefaultSubobject<ULSCameraComponent>(TEXT("LSCameraComponent"));
	CameraComponent->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	CameraComponent->bUsePawnControlRotation = false; //摄像机交由弹簧臂控制旋转
	CameraComponent->SpringArm = SpringArm; //将弹簧臂引用传递给摄像机组件，以便在切换视角时调整位置和FOV
	
	//2. 创建第一人称手臂Mesh组件并附加到摄像机组件
	FPArmsMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FPArmsMesh"));
	FPArmsMesh->SetupAttachment(CameraComponent);
	FPArmsMesh->SetOnlyOwnerSee(true); //仅本地玩家可见
	FPArmsMesh->bCastDynamicShadow = false; //不投射动态阴影
	FPArmsMesh->CastShadow = false; //不投射阴影
	FPArmsMesh->SetCollisionProfileName(TEXT("NoCollision")); //不参与碰撞
	
	//基础角色Mesh设置（第三人称全身模型）
	GetMesh()->SetupAttachment(GetCapsuleComponent());
	GetMesh()->bCastHiddenShadow = true; //隐藏时仍投射阴影
	
	WeaponComponent = CreateDefaultSubobject<ULSWeaponComponent>(TEXT("LSWeaponComp"));

	//挂载元素附着中枢
	ElementComponent = CreateDefaultSubobject<ULSElementComponent>(TEXT("LSElementComp"));

	// 实例化技能与大招充能组件
	SkillComponent = CreateDefaultSubobject<ULSSkillComponent>(TEXT("LSSkillComp"));
}

// Called every frame
void ALSCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	//后坐力恢复现已由 WeaponBase 上的 LSRecoilComponent 自动处理
}

void ALSCharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALSCharacterBase, CurrentHealth);
}

void ALSCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	CurrentHealth = MaxHealth;
}

float ALSCharacterBase::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	if (!HasAuthority() || CurrentHealth <= 0.0f)
	{
		return 0.0f;
	}

	// 1. 无敌帧拦截（移动组件在闪避期间广播的 Invincible 状态）
	if (ULSMovementComponent* MoveComp = GetLSMovementComponent())
	{
		if (MoveComp->IsInvincible())
		{
			return 0.0f; // 处于闪避无敌帧，彻底免伤！
		}
	}

	// 2. 权威扣除生命值
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	CurrentHealth = FMath::Clamp(CurrentHealth - ActualDamage, 0.0f, MaxHealth);

	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);

	// 3. 判定死亡
	if (CurrentHealth <= 0.0f)
	{
		Die(DamageCauser);
	}

	return ActualDamage;
}

void ALSCharacterBase::Die(AActor* Killer)
{
	// 关闭碰撞，防止死后继续挡子弹
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	// 检查小队是否有存活队友可以换入
	if (ALSPlayerController* PC = Cast<ALSPlayerController>(GetController()))
	{
		if (ULSTeamSwitchComponent* TeamComp = PC->GetTeamSwitchComponent())
		{
			bool bSwitched = false;
			for (int32 i = 0; i < 3; ++i)
			{
				if (TeamComp->CanSwitchToIndex(i))
				{
					TeamComp->SwitchTo(i);
					bSwitched = true;
					break;
				}
			}
			
			if (!bSwitched)
			{
				// 没有存活队友可切换，触发游戏失败逻辑（可在蓝图中绑定事件）
				PC->SetIgnoreMoveInput(true);
				PC->SetIgnoreLookInput(true);
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("☠️ 【全队覆灭】小队所有角色均已阵亡！"));
			}
		}
	}
	else if (AController* GenericPC = GetController())
	{
		GenericPC->SetIgnoreMoveInput(true);
		GenericPC->SetIgnoreLookInput(true);
	}
	
	// 通过总线向全关卡广播死亡事件（驱动结算、任务计数）
	if (ULSEventBus* EventBus = ULSEventBus::Get(this))
	{
		EventBus->OnCharacterDied.Broadcast(this);
		if (Killer)
		{
			EventBus->OnEnemyKilled.Broadcast(this, Killer);
		}
	}
}

void ALSCharacterBase::OnRep_CurrentHealth()
{
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

void ALSCharacterBase::EnterBackgroundMode()
{
	// 1. 隐藏全身与第一人称手臂
	SetActorHiddenInGame(true);

	// 2, 隐藏当前持有的手部/收纳武器
	if (WeaponComponent)
	{
		WeaponComponent->StopFire();
		//隐藏武器Actor
		if (ALSWeaponBase* CurrentWeapon = WeaponComponent->GetCurrentWeapon())
		{
			CurrentWeapon->SetActorHiddenInGame(true);
		}
	}

	// 3. 关闭物理胶囊体碰撞（防止在后台时挡住子弹或被怪物打中）
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 4. 停止移动组件
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->DisableMovement();
	}
}

void ALSCharacterBase::ExitBackgroundMode()
{
	// 1. 重新显形
	SetActorHiddenInGame(false);

	// 2, 恢复武器显型
	if (WeaponComponent)
	{
		//恢复武器Actor显型
		if (ALSWeaponBase* CurrentWeapon = WeaponComponent->GetCurrentWeapon())
		{
			CurrentWeapon->SetActorHiddenInGame(false);
		}
	}

	// 3. 恢复碰撞
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	// 4. 恢复移动模式为行走
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->SetMovementMode(MOVE_Walking);
	}

	// 5. 重新让摄像机对准当前手臂显隐状态
	if (CameraComponent)
	{
		CameraComponent->UpdateMeshVisibility();
	}
}