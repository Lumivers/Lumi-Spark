#include "LSEnemyBase.h"
#include "LSAIController.h"
#include "LSEnemyDataAsset.h"
#include "Character/LSHealthComponent.h"
#include "Element/LSElementComponent.h"
#include "Combat/LSShieldComponent.h"
#include "Combat/LSThreatComponent.h"
#include "Character/LSEnergyComponent.h"
#include "Character/LSCharacterBase.h"
#include "Core/LSEventBus.h"
#include "Core/LSPlayerController.h"
#include "Extraction/ULSCorrosionComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "UI/LSFloatingHealthBarWidget.h"
#include "Engine/DamageEvents.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

ALSEnemyBase::ALSEnemyBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	// 装配专职战斗组件
	HealthComponent = CreateDefaultSubobject<ULSHealthComponent>(TEXT("HealthComp"));
	ElementComponent = CreateDefaultSubobject<ULSElementComponent>(TEXT("ElementComp"));
	ShieldComponent = CreateDefaultSubobject<ULSShieldComponent>(TEXT("ShieldComp"));
	ThreatComponent = CreateDefaultSubobject<ULSThreatComponent>(TEXT("ThreatComp"));

	// 头顶血条组件装配（面向屏幕摄像机，免受视距与朝向旋转畸变）
	HealthBarComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarComp"));
	if (GetMesh())
	{
		HealthBarComponent->SetupAttachment(GetMesh(), FName("head"));
		HealthBarComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 35.0f));
	}
	else
	{
		HealthBarComponent->SetupAttachment(RootComponent);
		HealthBarComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 110.0f));
	}
	HealthBarComponent->SetWidgetSpace(EWidgetSpace::Screen);
	HealthBarComponent->SetDrawSize(FVector2D(120.0f, 10.0f));
	HealthBarComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 敌人碰撞通道配置：胶囊体与骨骼网格体确保阻挡武器射线 Visibility
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	if (GetMesh())
	{
		GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	}

	// 绑定战术 AI 控制器与生成/放置即附身
	AIControllerClass = ALSAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// ─── 运动与转向平滑配置 ───
	// 禁用控制器强制硬同步 Yaw，交由 CharacterMovementComponent 物理平滑旋转
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement = true; // 随寻路移动速度方向自动转向
		MoveComp->bUseControllerDesiredRotation = false;
		MoveComp->RotationRate = FRotator(0.0f, 600.0f, 0.0f); // 敏捷转向角速度（度/秒）
	}
}

// ILSInteractableInterface 交互接口实现
bool ALSEnemyBase::CanInteract(AActor* Interactor) const
{
	if (!bAllowBackstabExecution || bIsDead || !Interactor)
	{
		return false;
	}
	
	// 目标已死亡判定
	if (HealthComponent && HealthComponent->IsDead())
	{
		return false;
	}
	
	const FVector EnemyLoc = GetActorLocation();
	const FVector InteractorLoc = Interactor->GetActorLocation();
	
	// 1. 高度差校验（防止跳在头顶或上下楼梯误触）
	if (FMath::Abs(EnemyLoc.Z - InteractorLoc.Z) > 100.0f)
	{
		return false;
	}
	
	// 2. 水平距离校验（默认 2.3 米内）
	const float DistSq2D = FVector::DistSquared2D(EnemyLoc, InteractorLoc);
	if (DistSq2D > FMath::Square(BackstabMaxDistance))
	{
		return false;
	}
	
	// 3. 背后空间点积校验（核心几何计算）
	const FVector EnemyForward = GetActorForwardVector().GetSafeNormal2D();
	const FVector ToInteractor = (InteractorLoc - EnemyLoc).GetSafeNormal2D();
	
	// 当玩家处于怪的正背后时，ToInteractor 与 EnemyForward 反向，点积趋近于 -1.0
	// 这里取 Dot < -0.4f，相当于怪物背后约 132° 的后向扇形夹角
	const float BehindDot = FVector::DotProduct(EnemyForward, ToInteractor);
	if (BehindDot > -0.4f)
	{
		return false; // 未在背后扇形区域
	}
	
	// 4. 玩家朝向校验（玩家必须面朝怪物背部）
	const FVector InteractorForward = Interactor->GetActorForwardVector().GetSafeNormal2D();
	const float FacingDot = FVector::DotProduct(InteractorForward, EnemyForward);
	if (FacingDot < 0.2f)
	{
		return false; // 玩家未朝向怪物背部（如背对背）
	}
	
	return true;
}

FText ALSEnemyBase::GetInteractPrompt(AActor* Interactor) const
{
	return NSLOCTEXT("LumiSpark", "XenoBladePrompt", "按 [F] 异体刃背后处决 (-75% HP / 破盾)");
}

void ALSEnemyBase::OnInteractComplete(AActor* Interactor)
{
	// 按 F 瞬发处决
	ExecuteXenoBlade(Interactor);
}

bool ALSEnemyBase::ExecuteXenoBlade(AActor* Interactor)
{
	if (!HasAuthority()) return false;
	if (bIsDead || !HealthComponent || HealthComponent->IsDead()) return false;

	// 1. 瞬碎所有元素护盾
	if (ShieldComponent)
	{
		ShieldComponent->ShatterAllShields();
	}

	// 2. 根据怪物品级确定处决伤害（普通小怪 100% / 精英怪 50% / Boss 25%）
	float DamageRatio = 1.0f; // 默认普通小怪 100% 斩杀
	if (EnemyDataAsset)
	{
		if (EnemyDataAsset->EnemyIDTag == LSTags::TAG_Enemy_Type_Boss || ActiveGameplayTags.HasTag(LSTags::TAG_Enemy_Type_Boss))
		{
			DamageRatio = 0.25f; // 首领 Boss 25%
		}
		else if (EnemyDataAsset->EnemyIDTag == LSTags::TAG_Enemy_Type_Elite || ActiveGameplayTags.HasTag(LSTags::TAG_Enemy_Type_Elite))
		{
			DamageRatio = 0.50f; // 精英怪 50%
		}
	}

	const float MaxHP = HealthComponent->GetMaxHealth();
	const float ExecutionDamage = MaxHP * DamageRatio;

	// 3. 造成技能穿透伤害
	HealthComponent->TakeDamage(ExecutionDamage, LSTags::TAG_Damage_Type_Skill, Interactor, Interactor ? Interactor->GetInstigatorController() : nullptr);

	// 非致死处决（精英怪/Boss）触发受击音效
	if (!bIsDead && !HealthComponent->IsDead())
	{
		TryPlayHitSound();
	}

	OnXenoBladeExecuted.Broadcast(this, Interactor, ExecutionDamage);

	FString RankName = DamageRatio >= 1.0f ? TEXT("普通小怪") : (DamageRatio >= 0.5f ? TEXT("精英怪") : TEXT("首领Boss"));
	GEngine->AddOnScreenDebugMessage(-1, 3.5f, FColor::Red, FString::Printf(TEXT("【战术处决】对【%s】造成 %.0f 真实伤害（%.0f%% 生命），护盾已破除！"), *RankName, ExecutionDamage, DamageRatio * 100.0f));

	return true;
}

void ALSEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	// 运行时强制生效平滑转向与移动朝向（防御既有蓝图资产已序列化旧 CDO 默认值的问题）
	bUseControllerRotationYaw = false;
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement = true;
		MoveComp->RotationRate = FRotator(0.0f, 600.0f, 0.0f);
	}

	// 1. 确保在单机或服务端必定拥有 AI 控制器附身（防御蓝图拖放放置时丢失 AutoPossessAI）
	if (!GetController() && HasAuthority())
	{
		if (AIControllerClass)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AController* NewAIC = GetWorld()->SpawnActor<AController>(AIControllerClass, GetActorTransform(), SpawnParams);
			if (NewAIC)
			{
				NewAIC->Possess(this);
				UE_LOG(LogTemp, Warning, TEXT("🤖 [ALSEnemyBase] BeginPlay 成功兜底生成并附身 AIController: %s"), *NewAIC->GetName());
			}
		}
	}

	// 2. 绑定生命组件的死亡与血量变动广播
	if (HealthComponent)
	{
		HealthComponent->OnDeath.AddDynamic(this, &ALSEnemyBase::HandleHealthComponentDeath);
		HealthComponent->OnHealthChanged.AddDynamic(this, &ALSEnemyBase::HandleHealthChanged);
	}

	// 3. 从 EnemyDataAsset 资产初始化属性与抗性
	if (EnemyDataAsset)
	{
		CachedResistances = EnemyDataAsset->Resistances;

		if (HealthComponent)
		{
			HealthComponent->InitializeHealth(EnemyDataAsset->MaxHealth);
		}

		if (EnemyDataAsset->bDefaultSuperArmor)
		{
			ActiveGameplayTags.AddTag(LSTags::TAG_State_SuperArmor);
		}

		SetCombatMovementSpeed(false);
	}
}

float ALSEnemyBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead || DamageAmount <= 0.0f) return 0.0f;

	LastDamageCauser = DamageCauser;
	LastDamageInstigator = EventInstigator;

	// 记录受击物理信息（若为点伤害提取精确打击点、骨骼和射击方向，供死亡布娃娃继承动量）
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const FPointDamageEvent* PointDmg = static_cast<const FPointDamageEvent*>(&DamageEvent);
		LastHitLocation = PointDmg->HitInfo.ImpactPoint;
		LastHitBoneName = PointDmg->HitInfo.BoneName;
		LastHitDirection = PointDmg->ShotDirection;
	}
	else
	{
		LastHitLocation = GetActorLocation();
		LastHitBoneName = NAME_None;
		LastHitDirection = DamageCauser ? (GetActorLocation() - DamageCauser->GetActorLocation()).GetSafeNormal() : -GetActorForwardVector();
	}

	// 1. 解析伤害中的元素属性并应用元素抗性减免
	float AdjustedDamage = DamageAmount;
	// 默认假定无元素，若有命中上下文可在后续集成
	FGameplayTag DmgElement = FGameplayTag();

	if (CachedResistances.Contains(DmgElement))
	{
		const float ResFactor = FMath::Clamp(CachedResistances[DmgElement], -1.0f, 1.0f);
		AdjustedDamage *= (1.0f - ResFactor);
	}

	const float PostResistanceDamage = AdjustedDamage;

	// 2. 先走多层元素破盾判定 (ShieldComponent)
	if (ShieldComponent && ShieldComponent->HasActiveShield())
	{
		AdjustedDamage = ShieldComponent->AbsorbDamage(AdjustedDamage, DmgElement, DamageCauser);
	}

	// 3. 记录仇恨值 (ThreatComponent)
	if (ThreatComponent && DamageCauser)
	{
		ThreatComponent->AddThreatFromDamage(DamageCauser, AdjustedDamage, false);
	}

	// 4. 剩余伤害移交生命组件
	if (HealthComponent && AdjustedDamage > 0.0f)
	{
		HealthComponent->TakeDamage(AdjustedDamage, DmgElement, DamageCauser, EventInstigator);

		if (GEngine)
		{
			const float CurHP = HealthComponent->GetCurrentHealth();
			const float MaxHP = HealthComponent->GetMaxHealth();
#if WITH_EDITOR
			const FString DisplayName = GetActorLabel();
#else
			const FString DisplayName = GetName();
#endif
			// 使用固定 Key 2002 原地单行刷新，连射受击时动态更新生命值数字
			GEngine->AddOnScreenDebugMessage(2002, 1.5f, FColor::Orange,
				FString::Printf(TEXT("💥 [受击] %s 承受: %.1f | 剩余生命: %.1f / %.1f"),
					*DisplayName, AdjustedDamage, CurHP, MaxHP));
		}
	}

	// 5. 播放受击音效（非致死且非完全免疫时触发；致死一击由 OnDeath 播放死亡音效，避免双重重叠）
	if (PostResistanceDamage > 0.0f && !bIsDead && (!HealthComponent || !HealthComponent->IsDead()))
	{
		TryPlayHitSound();
	}

	// 6. 受击即刻唤醒 AI 索敌与警戒对准（防止被从背后偷袭时发呆）
	if (ALSAIController* AIC = Cast<ALSAIController>(GetController()))
	{
		// 解析真正的攻击者角色（若 DamageCauser 是武器，取其持有人）
		AActor* ActualTarget = DamageCauser;
		if (ActualTarget && ActualTarget->GetOwner())
		{
			ActualTarget = ActualTarget->GetOwner();
		}

		if (ActualTarget)
		{
			AIC->SetFocus(ActualTarget);
			AIC->MoveToActor(ActualTarget, 150.0f);
			SetCombatMovementSpeed(true);
		}
	}

	return AdjustedDamage;
}

void ALSEnemyBase::SetCombatMovementSpeed(bool bInCombat)
{
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		if (EnemyDataAsset)
		{
			MoveComp->MaxWalkSpeed = bInCombat ? EnemyDataAsset->CombatRunSpeed : EnemyDataAsset->PatrolWalkSpeed;
		}
		else
		{
			// 无数据资产时的标准战术移速兜底（交火快跑 460，常态慢巡 200）
			MoveComp->MaxWalkSpeed = bInCombat ? 460.0f : 200.0f;
		}
	}
}

void ALSEnemyBase::HandleHealthComponentDeath()
{
	AActor* Killer = LastDamageCauser.IsValid() ? LastDamageCauser.Get() : nullptr;
	if (!Killer && LastDamageInstigator.IsValid())
	{
		Killer = LastDamageInstigator->GetPawn();
	}
	OnDeath(Killer);
}

void ALSEnemyBase::HandleHealthChanged(float CurrentHealth, float MaxHealth, bool bIsDamage)
{
	if (bIsDead) return;

	// 1. 同步刷新 UWidgetComponent 挂载的头顶血条控件
	if (HealthBarComponent)
	{
		if (ULSFloatingHealthBarWidget* BarWidget = Cast<ULSFloatingHealthBarWidget>(HealthBarComponent->GetUserWidgetObject()))
		{
			BarWidget->UpdateHealth(CurrentHealth, MaxHealth);
		}
	}

	// 2. 3D 即时头顶紧凑血条显示（受击瞬间在头顶清晰显现，免去任何手动蓝图配置）
	if (bIsDamage && CurrentHealth > 0.0f)
	{
		FVector HeadPos = GetActorLocation() + FVector(0.0f, 0.0f, 95.0f);
		if (GetMesh() && GetMesh()->DoesSocketExist(FName("head")))
		{
			HeadPos = GetMesh()->GetSocketLocation(FName("head")) + FVector(0.0f, 0.0f, 25.0f);
		}

		const float Pct = MaxHealth > 0.0f ? FMath::Clamp(CurrentHealth / MaxHealth, 0.0f, 1.0f) : 0.0f;
		const FColor HealthColor = FColor::MakeRedToGreenColorFromScalar(Pct);
		const FString HealthStr = FString::Printf(TEXT("[%.0f / %.0f]"), CurrentHealth, MaxHealth);

		// 在头顶显示清晰小巧的血量数值，存活 1.2 秒（受击刷新）
		DrawDebugString(GetWorld(), HeadPos, HealthStr, nullptr, HealthColor, 1.2f, true, 1.0f);
	}
}

void ALSEnemyBase::OnDeath(AActor* Killer)
{
	if (bIsDead) return;
	bIsDead = true;

	ActiveGameplayTags.AddTag(LSTags::TAG_State_Dead);

	// 1. 战术正向解压：击杀变异体回退玩家地脉侵蚀度
	ApplyCorrosionReductionToKiller(Killer);

	// 2. 掉落元素能量微粒与视听反馈
	SpawnEnergyParticles(Killer);

	// 3. 隐藏头顶血条
	if (HealthBarComponent)
	{
		HealthBarComponent->SetVisibility(false);
	}

	// 4. 播放死亡音效（Dedicated Server 跳过本地音频）
	if (DeathSound && GetNetMode() != NM_DedicatedServer)
	{
		UGameplayStatics::PlaySoundAtLocation(this, DeathSound, GetActorLocation());
	}

	// 5. 全局事件总线广播击杀事件（供 HUD、任务与搜打撤战后结算系统订阅）
	if (ULSEventBus* EventBus = ULSEventBus::Get(this))
	{
		EventBus->OnEnemyKilled.Broadcast(this, Killer);
	}

	// 6. 停止移动组件并禁用 Tick，防止死后继续消耗物理寻路开销
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->DisableMovement();
		MoveComp->SetComponentTickEnabled(false);
	}

	// 7. 关闭胶囊体碰撞（防止死尸阻挡玩家射击射线或移动走位）
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Capsule->SetCollisionResponseToAllChannels(ECR_Ignore);
	}

	// 8. 解除 AI 控制器附身并停止寻路与感知聚焦
	if (AController* C = GetController())
	{
		C->StopMovement();
		if (AAIController* AIC = Cast<AAIController>(C))
		{
			AIC->ClearFocus(EAIFocusPriority::Gameplay);
		}
		DetachFromControllerPendingDestroy();
	}

	// 9. 物理布娃娃系统（Ragdoll）倒地或播放死亡动画蒙太奇
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		if (bUseRagdollDeath || !DeathMontage)
		{
			// 设置布娃娃碰撞 Profile（对场景静态/物理体 Block，对 Pawn/Camera 忽略）
			MeshComp->SetCollisionProfileName(TEXT("Ragdoll"));
			MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

			// 开启骨骼网格体全物理模拟
			MeshComp->SetAllBodiesSimulatePhysics(true);
			MeshComp->SetSimulatePhysics(true);
			MeshComp->WakeAllRigidBodies();
			MeshComp->bBlendPhysics = true;

			// 仅当物理模拟已激活时施加冲量，彻底消除底层未激活时的黄色警告！
			if (MeshComp->IsSimulatingPhysics())
			{
				FVector Impulse = LastHitDirection.GetSafeNormal() * DeathImpulseForce;
				Impulse.Z += 80.0f; // 略微上抛，防止直接贴地滑动或粘连

				if (LastHitBoneName != NAME_None)
				{
					MeshComp->AddImpulseAtLocation(Impulse, LastHitLocation, LastHitBoneName);
				}
				else
				{
					MeshComp->AddImpulse(Impulse, NAME_None, true); // true = 忽略质量施加速度突变
				}
			}
		}
		else
		{
			// 播放死亡动画蒙太奇
			PlayAnimMontage(DeathMontage);
		}
	}

	// 10. 延时销毁 Actor（给布娃娃留出倒地稳定与特写展示时间）
	SetLifeSpan(CorpseLifeSpan);
}

void ALSEnemyBase::ApplyCorrosionReductionToKiller(AActor* Killer)
{
	if (!Killer) return;

	ALSCharacterBase* PlayerChar = Cast<ALSCharacterBase>(Killer);
	if (!PlayerChar && Killer->GetOwner())
	{
		PlayerChar = Cast<ALSCharacterBase>(Killer->GetOwner());
	}

	if (PlayerChar)
	{
		if (ALSPlayerController* PC = Cast<ALSPlayerController>(PlayerChar->GetController()))
		{
			if (ULSCorrosionComponent* CorrosionComp = PC->GetCorrosionComponent())
			{
				const float Reduction = EnemyDataAsset ? EnemyDataAsset->DroppedCorrosionReduction : 20.0f;
				CorrosionComp->ReduceCorrosion(Reduction);
			}
		}
	}
}

void ALSEnemyBase::SpawnEnergyParticles(AActor* Killer)
{
	const int32 ParticleCount = EnemyDataAsset ? EnemyDataAsset->DroppedParticleCount : 2;
	const float EnergyPerPart = EnemyDataAsset ? EnemyDataAsset->EnergyPerParticle : 6.0f;
	const FGameplayTag ParticleElement = EnemyDataAsset ? EnemyDataAsset->DroppedParticleElement : FGameplayTag();

	// 1. 注入能量给击杀者玩家小队
	if (Killer)
	{
		ALSCharacterBase* PlayerChar = Cast<ALSCharacterBase>(Killer);
		if (!PlayerChar && Killer->GetOwner())
		{
			PlayerChar = Cast<ALSCharacterBase>(Killer->GetOwner());
		}

		if (PlayerChar)
		{
			if (ULSEnergyComponent* EnergyComp = PlayerChar->GetEnergyComponent())
			{
				for (int32 i = 0; i < ParticleCount; ++i)
				{
					EnergyComp->CollectParticle(ParticleElement, EnergyPerPart);
				}
			}
		}
	}

	// 2. 视觉表现：若配置了特效资产则播放，未配置则不绘制任何大光球，保持战场写实纯净
	const FVector DropLocation = GetActorLocation() + FVector(0.0f, 0.0f, 40.0f);

	if (EnergyDropNiagara)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), EnergyDropNiagara, DropLocation, GetActorRotation());
	}
	else if (EnergyDropEmitter)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), EnergyDropEmitter, DropLocation, GetActorRotation());
	}

	// 3. 屏幕单行击杀确认刷新（Key 2003 原地刷新，与 2000开火/2001命中/2002生命 组成完整打击闭环）
	if (GEngine)
	{
#if WITH_EDITOR
		const FString DisplayName = GetActorLabel();
#else
		const FString DisplayName = GetName();
#endif
		const float TotalEnergy = ParticleCount * EnergyPerPart;
		GEngine->AddOnScreenDebugMessage(2003, 3.0f, FColor::Cyan,
			FString::Printf(TEXT("💀 [击杀确认] %s 已被消灭！获得能量微粒 (+%.0f) | 侵蚀度已净化"),
				*DisplayName, TotalEnergy));
	}
}

void ALSEnemyBase::TryPlayHitSound()
{
	if (!HitSound || bIsDead)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 冷却校验：避免高射速武器、霰弹多弹丸或持续伤害在同一时间窗口内反复叠音
	const double CurrentTime = World->GetTimeSeconds();
	if (LastHitSoundTime >= 0.0 && (CurrentTime - LastHitSoundTime) < static_cast<double>(HitSoundCooldown))
	{
		return;
	}

	LastHitSoundTime = CurrentTime;

	if (HasAuthority())
	{
		Multicast_PlayHitSound();
	}
	else if (GetNetMode() != NM_DedicatedServer)
	{
		UGameplayStatics::PlaySoundAtLocation(this, HitSound, GetActorLocation());
	}
}

void ALSEnemyBase::Multicast_PlayHitSound_Implementation()
{
	if (GetNetMode() == NM_DedicatedServer || !HitSound)
	{
		return;
	}

	UGameplayStatics::PlaySoundAtLocation(this, HitSound, GetActorLocation());
}