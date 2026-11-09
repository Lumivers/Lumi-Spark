#include "LSPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "LSCharacterBase.h"
#include "LSCameraComponent.h"
#include "LSMovementComponent.h"
#include "Weapon/LSWeaponComponent.h"
#include "Blueprint/UserWidget.h"
#include "Character/LSSkillComponent.h"
#include "Weapon/LSGrenadeBase.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "UI/LSUHDWidget.h"
#include "Weapon/LSThrowableComponent.h"
#include "Extraction/ULSBackpackComponent.h"
#include "Extraction/LSExtractionTypes.h"
#include "Extraction/ULSCorrosionComponent.h"
ALSPlayerController::ALSPlayerController()
{
	bShowMouseCursor = false;
	BackpackComponent = CreateDefaultSubobject<ULSBackpackComponent>(TEXT("BackpackComponent"));
	CorrosionComponent = CreateDefaultSubobject<ULSCorrosionComponent>(TEXT("CorrosionComponent"));
}

void ALSPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	//默认进入纯游戏战斗输入模式
	SwitchToGameInputMode();
	
	if (IsLocalController() && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1100, 8.0f, FColor::Green, 
			FString::Printf(TEXT("🎮 [LumiSpark] 控制器就绪 | IA_Crouch: %s | IA_Dash: %s | IMC: %s"), 
				IA_Crouch ? TEXT("已配置") : TEXT("❌未配置(NULL)"), 
				IA_Dash ? TEXT("已配置") : TEXT("❌未配置(NULL)"),
				DefaultMappingContext ? TEXT("已配置") : TEXT("❌未配置(NULL)")));
	}
	if (IsLocalController())
	{
		// 如果蓝图没配，C++ 自动去把刚才做好的 WBP_LSUHD 加载出来！
		if (!HUDWidgetClass)
		{
			HUDWidgetClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/WBP_LSUHD.WBP_LSUHD_C"));
		}
		if (HUDWidgetClass && !HUDWidgetInstance)
		{
			HUDWidgetInstance = CreateWidget<UUserWidget>(this, HUDWidgetClass);
			if (HUDWidgetInstance)
			{
				HUDWidgetInstance->AddToViewport();
				UE_LOG(LogTemp, Warning, TEXT("[LumiSpark] HUD 成功加载并添加到视口！"));
			}
		}

		// 绑定当前在场角色
		if (HUDWidgetInstance)
		{
			if (ALSCharacterBase* Char = GetPawn<ALSCharacterBase>())
			{
				if (ULSHUDWidget* HUD = Cast<ULSHUDWidget>(HUDWidgetInstance))
				{
					HUD->BindToCharacter(Char);
				}
			}
		}
	}
}

void ALSPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	// 【防止异步延迟】：如果角色是在 Controller 之后才生成附着的，这里再次确保绑定
	if (IsLocalController() && HUDWidgetInstance)
	{
		if (ULSHUDWidget* HUD = Cast<ULSHUDWidget>(HUDWidgetInstance))
		{
			if (ALSCharacterBase* LSChar = Cast<ALSCharacterBase>(InPawn))
			{
				HUD->BindToCharacter(LSChar);
			}
		}
	}
}

void ALSPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		//1，移动与视角
		if (IA_Move) EnhancedInputComponent->BindAction(IA_Move, ETriggerEvent::Triggered, this, &ALSPlayerController::HandleMove);
		if (IA_Look) EnhancedInputComponent->BindAction(IA_Look, ETriggerEvent::Triggered, this, &ALSPlayerController::HandleLook);
		if (IA_Jump)
		{
			EnhancedInputComponent->BindAction(IA_Jump, ETriggerEvent::Started, this, &ALSPlayerController::HandleJumpStarted);
			EnhancedInputComponent->BindAction(IA_Jump, ETriggerEvent::Completed, this, &ALSPlayerController::HandleJumpCompleted);
		}
		if (IA_Sprint)
		{
			EnhancedInputComponent->BindAction(IA_Sprint, ETriggerEvent::Started, this, &ALSPlayerController::HandleSprintStarted);
			EnhancedInputComponent->BindAction(IA_Sprint, ETriggerEvent::Completed, this, &ALSPlayerController::HandleSprintCompleted);
		}
		if (IA_Dash) EnhancedInputComponent->BindAction(IA_Dash, ETriggerEvent::Started, this, &ALSPlayerController::HandleDash);
		if (IA_Crouch)
		{
			EnhancedInputComponent->BindAction(IA_Crouch, ETriggerEvent::Started, this, &ALSPlayerController::HandleCrouchStarted);
			EnhancedInputComponent->BindAction(IA_Crouch, ETriggerEvent::Completed, this, &ALSPlayerController::HandleCrouchCompleted);
		}
		
		//2，射击与开镜
		if (IA_Fire)
		{
			EnhancedInputComponent->BindAction(IA_Fire, ETriggerEvent::Started, this, &ALSPlayerController::HandleFireStarted);
			EnhancedInputComponent->BindAction(IA_Fire, ETriggerEvent::Completed, this, &ALSPlayerController::HandleFireCompleted);
		}
		
		if (IA_ADS)
		{
			EnhancedInputComponent->BindAction(IA_ADS, ETriggerEvent::Started, this, &ALSPlayerController::HandleADSStarted);
			EnhancedInputComponent->BindAction(IA_ADS, ETriggerEvent::Completed, this, &ALSPlayerController::HandleADSCompleted);
			EnhancedInputComponent->BindAction(IA_ADS, ETriggerEvent::Canceled, this, &ALSPlayerController::HandleADSCompleted);
		}
		if (IA_Reload) EnhancedInputComponent->BindAction(IA_Reload, ETriggerEvent::Started, this, &ALSPlayerController::HandleReload);
		
		//3，技能，手雷与切人
		if (IA_Skill) EnhancedInputComponent->BindAction(IA_Skill, ETriggerEvent::Started, this, &ALSPlayerController::HandleSkill);
		if (IA_Burst) EnhancedInputComponent->BindAction(IA_Burst, ETriggerEvent::Started, this, &ALSPlayerController::HandleBurst);
		if (IA_ThrowGrenade)
		{
			EnhancedInputComponent->BindAction(IA_ThrowGrenade, ETriggerEvent::Started, this, &ALSPlayerController::HandleThrowGrenadeStarted);
			EnhancedInputComponent->BindAction(IA_ThrowGrenade, ETriggerEvent::Completed, this, &ALSPlayerController::HandleThrowGrenadeCompleted);
		}
		if (IA_SwitchCharacter) EnhancedInputComponent->BindAction(IA_SwitchCharacter, ETriggerEvent::Started, this, &ALSPlayerController::HandleSwitchCharacter);
		if (IA_Interact)
		{
			EnhancedInputComponent->BindAction(IA_Interact, ETriggerEvent::Started, this, &ALSPlayerController::HandleInteractStarted);
			EnhancedInputComponent->BindAction(IA_Interact, ETriggerEvent::Completed, this, &ALSPlayerController::HandleInteractCompleted);
		}
		if (IA_SwitchToSlot1) EnhancedInputComponent->BindAction(IA_SwitchToSlot1, ETriggerEvent::Started, this, &ALSPlayerController::HandleSwitchToSlot1);
		if (IA_SwitchToSlot2) EnhancedInputComponent->BindAction(IA_SwitchToSlot2, ETriggerEvent::Started, this, &ALSPlayerController::HandleSwitchToSlot2);
		if (IA_SwitchToSlot3) EnhancedInputComponent->BindAction(IA_SwitchToSlot3, ETriggerEvent::Started, this, &ALSPlayerController::HandleSwitchToSlot3);
		if (IA_CycleCharacter) EnhancedInputComponent->BindAction(IA_CycleCharacter, ETriggerEvent::Triggered, this, &ALSPlayerController::HandleCycleCharacter);
		if (IA_Zoom) EnhancedInputComponent->BindAction(IA_Zoom, ETriggerEvent::Triggered, this, &ALSPlayerController::HandleZoom);
	}

	// 4. 滚轮缩放直接绑定
	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::MouseScrollUp, IE_Pressed, this, &ALSPlayerController::HandleZoomIn);
		InputComponent->BindKey(EKeys::MouseScrollDown, IE_Pressed, this, &ALSPlayerController::HandleZoomOut);
	}
}

void ALSPlayerController::SwitchToGameInputMode()
{
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->ClearAllMappings();
		if (DefaultMappingContext)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
	
	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = false;
}

void ALSPlayerController::SwitchToUIInputMode()
{
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->ClearAllMappings();
		if (UIModeMappingContext)
		{
			Subsystem->AddMappingContext(UIModeMappingContext, 1);
		}
	}
	
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	bShowMouseCursor = true;
}

void ALSPlayerController::HandleMove(const FInputActionValue& Value)
{
	if (ALSCharacterBase* Char = GetPawn<ALSCharacterBase>())
	{
		const FVector2D MoveVector = Value.Get<FVector2D>();
		const FRotator YawRotation(0, GetControlRotation().Yaw, 0);
		const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		
		Char->AddMovementInput(Forward, MoveVector.Y);
		Char->AddMovementInput(Right, MoveVector.X);
	}
}

void ALSPlayerController::HandleLook(const FInputActionValue& Value)
{
	const FVector2D LookVector = Value.Get<FVector2D>();
	const float Sensitivity = BaseLookSensitivity * (bIsAiming ? ADSSensitivityMultiplier : 1.0f);
	
	AddYawInput(LookVector.X * Sensitivity);
	AddPitchInput(LookVector.Y * Sensitivity);
}

void ALSPlayerController::HandleJumpStarted()
{
	if (ALSCharacterBase* Char = GetPawn<ALSCharacterBase>())
	{
		Char->Jump();
	}
}

void ALSPlayerController::HandleJumpCompleted()
{
	if (ALSCharacterBase* Char = GetPawn<ALSCharacterBase>()) Char->StopJumping();
}

void ALSPlayerController::HandleSprintStarted()
{
	if (ALSCharacterBase* Char = GetPawn<ALSCharacterBase>())
	{
		GEngine->AddOnScreenDebugMessage(1101, 1.5f, FColor::Cyan, TEXT("🏃 [EnhancedInput] 触发了疾跑 (Sprint Started)！"));
		if (ULSMovementComponent* MoveComp = Char->GetLSMovementComponent()) MoveComp->StartSprint();
	}
}

void ALSPlayerController::HandleSprintCompleted()
{
	if (ALSCharacterBase* Char = GetPawn<ALSCharacterBase>())
	{ 
		if (ULSMovementComponent* MoveComp = Char->GetLSMovementComponent())
		{
			MoveComp->StopSprint();
		}
		// 关键防护：冲刺松开时重新仲裁战斗姿态，若正在开火或开镜，绝对不退出 Aiming 端枪姿态
		UpdateCombatFacing();
	}
}

void ALSPlayerController::HandleDash()
{
	if (ALSCharacterBase* Char = GetPawn<ALSCharacterBase>())
	{
		if (ULSMovementComponent* MoveComp = Char->GetLSMovementComponent())
		{
			MoveComp->TryDash();
		}
	}
}

void ALSPlayerController::HandleCrouchStarted()
{
	if (ALSCharacterBase* Char = GetPawn<ALSCharacterBase>())
	{
		Char->Crouch();
	}
}

void ALSPlayerController::HandleCrouchCompleted()
{
	if (ALSCharacterBase* Char = GetPawn<ALSCharacterBase>())
	{
		Char->UnCrouch();
	}
}

void ALSPlayerController::HandleZoom(const FInputActionValue& Value)
{
	const float ZoomDelta = Value.Get<float>();
	if (FMath::IsNearlyZero(ZoomDelta)) return;
	
	if (ALSCharacterBase* Char = GetPawn<ALSCharacterBase>())
	{
		if (ULSCameraComponent* Cam = Char->GetCameraComponent())
		{
			Cam->AdjustCameraZoom(ZoomDelta);
		}
	}
}

void ALSPlayerController::HandleZoomIn()
{
	GEngine->AddOnScreenDebugMessage(54320, 1.0f, FColor::Yellow, TEXT("🔭 [Input] 滚轮向上，拉近视角"));
	if (ALSCharacterBase* Char = GetPawn<ALSCharacterBase>())
	{
		if (ULSCameraComponent* Cam = Char->GetCameraComponent())
		{
			Cam->AdjustCameraZoom(1.0f);
		}
	}
}

void ALSPlayerController::HandleZoomOut()
{
	GEngine->AddOnScreenDebugMessage(54320, 1.0f, FColor::Yellow, TEXT("🔭 [Input] 滚轮向下，拉远视角"));
	if (ALSCharacterBase* Char = GetPawn<ALSCharacterBase>())
	{
		if (ULSCameraComponent* Cam = Char->GetCameraComponent())
		{
			Cam->AdjustCameraZoom(-1.0f);
		}
	}
}

void ALSPlayerController::UpdateCombatFacing()
{
	if (ALSCharacterBase* Char = GetPawn<ALSCharacterBase>())
	{
		const bool bIsActiveCombat = bIsAiming || bIsFiring;
		Char->SetCombatFacing(bIsActiveCombat);
	}
}

void ALSPlayerController::HandleFireStarted()
{
	GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("🎯 [Input] 触发了鼠标左键开火！"));
	bIsFiring = true;

	if (ALSCharacterBase* Char = GetPawn<ALSCharacterBase>())
	{
		// 1. 开火瞬间立即打断冲刺并切入瞄准战术步态
		if (ULSMovementComponent* MoveComp = Char->GetLSMovementComponent())
		{
			MoveComp->StopSprint();
			MoveComp->SetAiming(true);
		}

		// 2. 开火瞬间：角色身形水平立即对齐当前视线朝向，消除转向延迟
		FRotator NewRot = Char->GetActorRotation();
		NewRot.Yaw = GetControlRotation().Yaw;
		Char->SetActorRotation(NewRot);

		// 3. 激活战斗朝向锁定与 Aiming(2) 极速端枪姿态
		UpdateCombatFacing();

		// 4. 触发武器组件开火
		if (ULSWeaponComponent* WeaponComp = Char->GetWeaponComponent())
		{
			WeaponComp->StartFire();
		}
	}
}

void ALSPlayerController::HandleFireCompleted()
{
	bIsFiring = false;

	if (ALSCharacterBase* Char = GetPawn<ALSCharacterBase>())
	{
		if (ULSWeaponComponent* WeaponComp = Char->GetWeaponComponent())
		{
			WeaponComp->StopFire();
		}

		// 停火后，若未按住右键开镜，则退出瞄准移动步态
		if (ULSMovementComponent* MoveComp = Char->GetLSMovementComponent())
		{
			if (!bIsAiming)
			{
				MoveComp->SetAiming(false);
			}
		}

		// 停止开火后，根据是否仍处于瞄准状态动态仲裁朝向
		UpdateCombatFacing();
	}
}

void ALSPlayerController::HandleADSStarted()
{
	GEngine->AddOnScreenDebugMessage(12345, 2.0f, FColor::Green, TEXT("🔍 [Input] 触发右键瞄准 (ADS) - 进入过肩模式！"));
	if (bIsAiming) return;
	bIsAiming = true;

	if (ALSCharacterBase* Char = GetPawn<ALSCharacterBase>())
	{
		if (ULSCameraComponent* Cam = Char->GetCameraComponent()) Cam->EnterADS();
		if (ULSMovementComponent* MoveComp = Char->GetLSMovementComponent()) MoveComp->SetAiming(true);
		UpdateCombatFacing();
	}
}

void ALSPlayerController::HandleADSCompleted()
{
	GEngine->AddOnScreenDebugMessage(12345, 2.0f, FColor::Cyan, TEXT("🔄 [Input] 释放右键，退出瞄准！"));
	if (!bIsAiming) return;
	bIsAiming = false;

	if (ALSCharacterBase* Char = GetPawn<ALSCharacterBase>())
	{
		if (ULSCameraComponent* Cam = Char->GetCameraComponent()) Cam->ExitADS();
		if (ULSMovementComponent* MoveComp = Char->GetLSMovementComponent()) MoveComp->SetAiming(false);
		UpdateCombatFacing();
	}
}

void ALSPlayerController::HandleReload()
{
	if (ALSCharacterBase* Char = GetPawn<ALSCharacterBase>())
	{
		GEngine->AddOnScreenDebugMessage(1102, 1.5f, FColor::Yellow, TEXT("🔄 [EnhancedInput] 触发了换弹 (Reload)！"));
		if (ULSWeaponComponent* WeaponComp = Char->GetWeaponComponent()) WeaponComp->Reload();
	}
}

void ALSPlayerController::HandleSkill()
{
	if (ALSCharacterBase* Char = GetPawn<ALSCharacterBase>())
	{
		if (ULSSkillComponent* SkillComp = Char->GetSkillComponent())
		{
			if (SkillComp->CanCastSkill())
			{
				SkillComp->CastSkill();
			}
			else
			{
				//屏幕屏幕调试提示或触发 CD 未就绪音效
                GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Yellow, FString::Printf(TEXT("⏳ E 战技冷却中，剩余: %.1fs"), SkillComp->GetSkillCooldownRemaining()));
			}
		}
	}
}

void ALSPlayerController::HandleBurst()
{
	if (ALSCharacterBase* Char = GetPawn<ALSCharacterBase>())
    {
        if (ULSSkillComponent* SkillComp = Char->GetSkillComponent())
        {
            if (SkillComp->CanCastBurst())
            {
                SkillComp->CastBurst();
            }
            else
            {
                GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Yellow, FString::Printf(TEXT("⚡ Q 能量不足: %.0f / %.0f"), SkillComp->GetCurrentEnergy(), SkillComp->GetMaxEnergy()));
            }
        }
    }
}

void ALSPlayerController::HandleThrowGrenadeStarted()
{
	if (ALSCharacterBase* Char = GetPawn<ALSCharacterBase>())
	{
		if (ULSThrowableComponent* ThrowComp = Char->GetThrowableComponent())
		{
			ThrowComp->StartAimingThrow();
		}
	}
}

void ALSPlayerController::HandleThrowGrenadeCompleted()
{
	if (ALSCharacterBase* Char = GetPawn<ALSCharacterBase>())
	{
		if (ULSThrowableComponent* ThrowComp = Char->GetThrowableComponent())
		{
			ThrowComp->StopAimingThrow(false);
		}
	}
}

void ALSPlayerController::HandleSwitchCharacter()
{
	if (ALSCharacterBase* Char = GetPawn<ALSCharacterBase>())
	{
		if (ULSWeaponComponent* WeaponComp = Char->GetWeaponComponent())
		{
			WeaponComp->ToggleWeapon();
		}
	}
}

void ALSPlayerController::HandleSwitchToSlot1()
{
	if (ALSCharacterBase* Char = GetPawn<ALSCharacterBase>())
	{
		if (ULSWeaponComponent* WeaponComp = Char->GetWeaponComponent())
		{
			WeaponComp->EquipPrimaryWeapon();
		}
	}
}

void ALSPlayerController::HandleSwitchToSlot2()
{
	if (ALSCharacterBase* Char = GetPawn<ALSCharacterBase>())
	{
		if (ULSWeaponComponent* WeaponComp = Char->GetWeaponComponent())
		{
			WeaponComp->EquipSecondaryWeapon();
		}
	}
}

void ALSPlayerController::HandleSwitchToSlot3()
{
	// 预留战术槽位
}

void ALSPlayerController::HandleCycleCharacter(const FInputActionValue& Value)
{
	const float AxisVal = Value.Get<float>();
	if (FMath::IsNearlyZero(AxisVal)) return;

	if (ALSCharacterBase* Char = GetPawn<ALSCharacterBase>())
	{
		if (ULSWeaponComponent* WeaponComp = Char->GetWeaponComponent())
		{
			WeaponComp->ToggleWeapon();
		}
	}
}

void ALSPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	// 长按交互进度推进
	if (CurrentInteractTarget.IsValid())
	{
		AActor* Target = CurrentInteractTarget.Get();
		APawn* ControlledPawn = GetPawn();

		// 检查距离（超过 3 米自动中断）
		if (!ControlledPawn || FVector::Dist(ControlledPawn->GetActorLocation(), Target->GetActorLocation()) > 300.0f)
		{
			HandleInteractCompleted();
			return;
		}
		
		InteractTimer += DeltaSeconds;

		// 驱动接口进度更新
		if (ILSInteractableInterface* Interface = Cast<ILSInteractableInterface>(Target))
		{
			const float Alpha = FMath::Clamp(InteractTimer / CurrentInteractDuration, 0.0f, 1.0f);
			Interface->OnInteractProgress(ControlledPawn, Alpha);
			
			//同步推流给HUD进度条控件
			if (ULSHUDWidget* HUD = Cast<ULSHUDWidget>(HUDWidgetInstance))
			{
				HUD->OnInteractProgressUpdated(Alpha);
			}

			// 长按时间达标 -> 完成交互
			if (InteractTimer >= CurrentInteractDuration)
			{
				Server_CompleteInteract(Target);
				Interface->OnInteractComplete(ControlledPawn);
				CurrentInteractTarget = nullptr;
				InteractTimer = 0.0f;

				//完成后重置HUD进度条
				if (ULSHUDWidget* HUD = Cast<ULSHUDWidget>(HUDWidgetInstance))
				{
					HUD->OnInteractProgressUpdated(0.0f);
				}
			}
		}
	}
}

void ALSPlayerController::HandleInteractCompleted()
{
	if (CurrentInteractTarget.IsValid())
	{
		if (ILSInteractableInterface* Interface = Cast<ILSInteractableInterface>(CurrentInteractTarget.Get()))
		{
			Interface->OnInteractCanceled(GetPawn());
		}
		CurrentInteractTarget = nullptr;
		if (ULSHUDWidget* HUD = Cast<ULSHUDWidget>(HUDWidgetInstance))
		{
			HUD->OnInteractProgressUpdated(0.0f);
		}
		InteractTimer = 0.0f;
	}
}

void ALSPlayerController::Server_CompleteInteract_Implementation(AActor* TargetActor)
{
	if (!TargetActor || !GetPawn()) return;
	if (ILSInteractableInterface* Interface = Cast<ILSInteractableInterface>(TargetActor))
	{
		if (Interface->CanInteract(GetPawn()))
		{
			Interface->OnInteractComplete(GetPawn());
		}
	}
}


void ALSPlayerController::LSAddLoot(const FString& LootType)
{
	if (!BackpackComponent) return;

	if (LootType.Equals(TEXT("Bearing"), ESearchCase::IgnoreCase))
	{
		// 材料：旧式降噪轴承（堆叠 100，单价 120）
		FLSInventoryItem Bearing = FLSInventoryItem::CreateMaterial(
			TEXT("Item_Mat_Bearing"),
			FText::FromString(TEXT("旧式降噪轴承")),
			15,
			120.0f
		);
		BackpackComponent->AddItem(Bearing);
		GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Cyan, TEXT("📦 拾取材料: [旧式降噪轴承] x15（可用于 Meta-Tree 加点与修枪）"));
	}
	else if (LootType.Equals(TEXT("Tape"), ESearchCase::IgnoreCase))
	{
		// 收藏品：特化加密录像带（单价 8,000）
		FLSInventoryItem Tape = FLSInventoryItem::CreateCollectible(
			TEXT("Item_Col_EncryptedTape"),
			FText::FromString(TEXT("【精密】特化加密录像带")),
			8000.0f,
			ELSExtractionRarity::Precision
		);
		BackpackComponent->AddItem(Tape);
		GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Yellow, TEXT("📼 摸到高价值收藏品: [特化加密录像带]！单价 8000！"));
	}
	else if (LootType.Equals(TEXT("Record"), ESearchCase::IgnoreCase))
	{
		// 顶级收藏品：绝密黄金黑胶唱片（单价 60,000，优先进安全箱）
		FLSInventoryItem Record = FLSInventoryItem::CreateCollectible(
			TEXT("Item_Col_GoldenRecord"),
			FText::FromString(TEXT("【绝密】初代黄金黑胶唱片")),
			60000.0f,
			ELSExtractionRarity::Classified
		);
		BackpackComponent->AddItem(Record, true);
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("📀 摸到顶级大红收藏品: [初代黄金黑胶唱片]！价值 60000，已自动护送入安全箱！"));
	}
	else if (LootType.Equals(TEXT("Disc"), ESearchCase::IgnoreCase))
	{
		// 驱动盘装备：Slot4 致命爆发暴击盘
		FLSDriveDisc Disc;
		Disc.DiscID = FGuid::NewGuid();
		Disc.DiscName = FText::FromString(TEXT("精密特化·致命爆发"));
		Disc.Slot = ELSDriveDiscSlot::Slot4_Critical;
		Disc.Rarity = ELSDriveDiscRarity::Precision;
		Disc.Level = 12;
		Disc.MainStat = FLSDriveDiscStat(ELSDriveStatType::CritRate, 0.311f);
		Disc.SubStats.Add(FLSDriveDiscStat(ELSDriveStatType::CritDamage, 0.155f));
		Disc.SubStats.Add(FLSDriveDiscStat(ELSDriveStatType::AttackPercent, 0.082f));

		FLSInventoryItem DiscItem = FLSInventoryItem::CreateDriveDiscItem(Disc);
		BackpackComponent->AddItem(DiscItem, false);
		GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Purple, FString::Printf(TEXT("💿 摸到驱动盘战利品: [%s]（占 1 格，不可堆叠）"), *Disc.DiscName.ToString()));
	}
	else if (LootType.Equals(TEXT("Blueprint"), ESearchCase::IgnoreCase))
	{
		// 武器原型图纸（自动入安全箱）
		FLSInventoryItem Wpn = FLSInventoryItem::CreateWeaponBlueprint(
			TEXT("Item_Wpn_ArcSMG"),
			FText::FromString(TEXT("【绝密】电弧连发冲锋枪原型图纸")),
			100000.0f
		);
		BackpackComponent->AddItem(Wpn, true);
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("🔫 摸到大红武器图纸！价值 100000，已锁定在安全箱中！"));
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::White, TEXT("用法: LS.AddLoot [Bearing / Tape / Record / Disc / Blueprint]"));
	}
}

void ALSPlayerController::LSUpgradeBackpack()
{
	if (!BackpackComponent) return;
	const int32 NewCap = BackpackComponent->UpgradeBackpackTier();
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("🎒 背包阶位晋级！新容量上限: %d 格！"), NewCap));
}

void ALSPlayerController::LSPrintBackpack()
{
	if (!BackpackComponent) return;

	const float TotalVal = BackpackComponent->CalculateTotalLootValue();
	const int32 TotalMats = BackpackComponent->GetTotalMaterialCount();

	UE_LOG(LogTemp, Warning, TEXT("================ 🎒 全队战利品背包清单 ================"));
	UE_LOG(LogTemp, Warning, TEXT("当前背包阶位: %d | 格数: %d/60 | 安全箱: %d/6"), static_cast<int32>(BackpackComponent->CurrentTier), BackpackComponent->BackpackCapacity, BackpackComponent->SecureBoxCapacity);
	UE_LOG(LogTemp, Warning, TEXT("总身价: %.0f  | 科技材料总数: %d"), TotalVal, TotalMats);

	UE_LOG(LogTemp, Warning, TEXT("--- 🛡️ 绝密安全箱 (100%% 保底) ---"));
	for (int32 i = 0; i < BackpackComponent->SecureBoxSlots.Num(); ++i)
	{
		const FLSInventoryItem& Item = BackpackComponent->SecureBoxSlots[i];
		if (Item.IsValid())
		{
			UE_LOG(LogTemp, Display, TEXT("  [安全格 %d] %s x%d (估值: %.0f) [安全]"), i, *Item.DisplayName.ToString(), Item.Quantity, Item.UnitValue * Item.Quantity);
		}
		else
		{
			UE_LOG(LogTemp, Display, TEXT("  [安全格 %d] (空)"), i);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("--- 📦 普通战利品格 (死亡掉落) ---"));
	for (int32 i = 0; i < BackpackComponent->BackpackSlots.Num(); ++i)
	{
		const FLSInventoryItem& Item = BackpackComponent->BackpackSlots[i];
		if (Item.IsValid())
		{
			UE_LOG(LogTemp, Display, TEXT("  [背包格 %d] %s x%d (估值: %.0f)"), i, *Item.DisplayName.ToString(), Item.Quantity, Item.UnitValue * Item.Quantity);
		}
		else
		{
			UE_LOG(LogTemp, Display, TEXT("  [背包格 %d] (空)"), i);
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("======================================================"));
	
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("🎒 背包状态已输出！容量: %d格 | 总身价: %.0f"), BackpackComponent->BackpackCapacity, TotalVal));
}

void ALSPlayerController::LSSimulateDeath()
{
	if (!BackpackComponent) return;

	TArray<FLSInventoryItem> Dropped;
	TArray<FLSInventoryItem> Retained;

	// 模拟战死（常规遗迹 0% 保留全部掉落在现场）
	BackpackComponent->ProcessDeathDrop(0.0f, Dropped, Retained);
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, FString::Printf(TEXT("💀 就地战死！遗留在尸体标包: %d 件 | 🛡️ 安全箱绝对带回: %d 件"), Dropped.Num(), Retained.Num()));
	UE_LOG(LogTemp, Error, TEXT("💥 战死模拟结算：现场留下 %d 堆掉落物（生成 CorpseMarker 供跑尸）；安全箱内 %d 件物品 100%% 成功保底！"), Dropped.Num(), Retained.Num());
}

void ALSPlayerController::HandleInteractStarted()
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn) return;

	FHitResult Hit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(ControlledPawn);

	bool bFoundInteractable = false;

	// 通道 1：近身扇形球体检测（优先抓取正前方的异体刃背后处决目标）
	const FVector PawnLoc = ControlledPawn->GetActorLocation();
	const FVector PawnForward = ControlledPawn->GetActorForwardVector();
	const FVector MeleeTraceEnd = PawnLoc + (PawnForward * 200.0f);

	if (GetWorld()->SweepSingleByChannel(Hit, PawnLoc, MeleeTraceEnd, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(50.0f), QueryParams))
	{
		if (Hit.GetActor() && Hit.GetActor()->Implements<ULSInteractableInterface>())
		{
			ILSInteractableInterface* Interface = Cast<ILSInteractableInterface>(Hit.GetActor());
			if (Interface && Interface->CanInteract(ControlledPawn))
			{
				bFoundInteractable = true;
			}
		}
	}

	// 通道 2：摄像机视线射线检测（抓取远距离宝箱、拉起倒地队友等常规交互物）
	if (!bFoundInteractable)
	{
		FVector CameraLoc;
		FRotator CameraRot;
		GetPlayerViewPoint(CameraLoc, CameraRot);
		const FVector TraceEnd = CameraLoc + (CameraRot.Vector() * 450.0f);
		if (GetWorld()->SweepSingleByChannel(Hit, CameraLoc, TraceEnd, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(40.0f), QueryParams))
		{
			if (Hit.GetActor() && Hit.GetActor()->Implements<ULSInteractableInterface>())
			{
				ILSInteractableInterface* Interface = Cast<ILSInteractableInterface>(Hit.GetActor());
				if (Interface && Interface->CanInteract(ControlledPawn))
				{
					bFoundInteractable = true;
				}
			}
		}
	}

	// 执行交互路由
	if (bFoundInteractable && Hit.GetActor())
	{
		ILSInteractableInterface* Interface = Cast<ILSInteractableInterface>(Hit.GetActor());
		const float Duration = Interface->GetInteractDuration(ControlledPawn);
		if (Duration <= 0.0f)
		{
			// 单击瞬发（处决、普通拾取）
			Server_CompleteInteract(Hit.GetActor());
			Interface->OnInteractComplete(ControlledPawn);
		}
		else
		{
			// 蓄力长按（救助倒地队友）
			CurrentInteractTarget = Hit.GetActor();
			CurrentInteractDuration = Duration;
			InteractTimer = 0.0f;
			Interface->OnInteractStart(ControlledPawn);
		}
	}
}

// ─── 控制台指令实现 ───
void ALSPlayerController::LSAddCorrosion(float Amount)
{
	if (!CorrosionComponent) return;
	CorrosionComponent->CurrentCorrosion = FMath::Clamp(CorrosionComponent->CurrentCorrosion + Amount, 0.0f, CorrosionComponent->MaxCorrosion);
	if (CorrosionComponent->CurrentCorrosion >= CorrosionComponent->MaxCorrosion && !CorrosionComponent->bIsOverloaded)
	{
		CorrosionComponent->bIsOverloaded = true;
		CorrosionComponent->OnCorrosionOverloadChanged.Broadcast(true);
	}
	CorrosionComponent->OnCorrosionUpdated.Broadcast(CorrosionComponent->CurrentCorrosion, CorrosionComponent->MaxCorrosion, CorrosionComponent->CurrentMaskDurability);
	GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Yellow, FString::Printf(TEXT("🧪 [Debug] 侵蚀度已增加 %.1f -> 当前: %.1f%% (过载: %s)"), Amount, CorrosionComponent->CurrentCorrosion, CorrosionComponent->bIsOverloaded ? TEXT("是") : TEXT("否")));
}

void ALSPlayerController::LSUseInjector()
{
	if (CorrosionComponent)
	{
		CorrosionComponent->UsePurificationInjector(100.0f);
	}
}

void ALSPlayerController::LSInstallFilter(float Durability)
{
	if (CorrosionComponent)
	{
		CorrosionComponent->EquipNewMask(FMath::RoundToInt(Durability > 0.0f ? Durability : 10.0f));
	}
}

void ALSPlayerController::LSPrintCorrosion()
{
	if (!CorrosionComponent) return;
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, FString::Printf(TEXT("═══ 地脉侵蚀监控 ═══\n侵蚀度: %.1f / %.1f (%.0f%%)\n面罩耐久: %d / %d 点 (状态: %s)\n过载状态: %s\n暗角强度: %.2f\n生命回复上限: %.0f%%"),
		CorrosionComponent->CurrentCorrosion, CorrosionComponent->MaxCorrosion, CorrosionComponent->GetCorrosionPercent() * 100.0f,
		CorrosionComponent->CurrentMaskDurability, CorrosionComponent->GetMaxMaskDurability(), CorrosionComponent->HasActiveMask() ? TEXT("完好防护") : TEXT("已报废/跑刀"),
		CorrosionComponent->bIsOverloaded ? TEXT("🚨 过载中 (扣血+上限压制)") : TEXT("正常"),
		CorrosionComponent->GetVignetteIntensity(),
		CorrosionComponent->GetHealthRecoveryCapPercent() * 100.0f
	));
}

void ALSPlayerController::Client_OnExtractionCountdownStarted_Implementation(const FText& PointName, float Duration)
{
	GEngine->AddOnScreenDebugMessage(1101, 3.0f, FColor::Cyan,
		FString::Printf(TEXT("🚨 正在撤离 [%s] - 保持在区域内 (%.1f 秒)..."), *PointName.ToString(), Duration));
}

void ALSPlayerController::Client_OnExtractionCountdownProgress_Implementation(float Progress, float RemainingTime)
{
	GEngine->AddOnScreenDebugMessage(1101, 0.5f, FColor::Cyan,
		FString::Printf(TEXT("⏳ 撤离倒计时: %.1f 秒 [%.0f%%]"), RemainingTime, Progress * 100.0f));
}

void ALSPlayerController::Client_OnExtractionCountdownCanceled_Implementation()
{
	GEngine->AddOnScreenDebugMessage(1101, 3.0f, FColor::Yellow, TEXT("⚠️ 离开撤离区，撤离已取消！"));
}

void ALSPlayerController::Client_OnExtractionSuccess_Implementation(const FLSRaidReport& Report)
{
	GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Green,
		FString::Printf(TEXT("🎉 撤离成功！带出物资估值: %.0f 金币 | 抗蚀耐久: -%d (剩余 %d) | 物品数: %d"),
			Report.TotalExtractedValue, Report.DurabilityDeducted, Report.RemainingMaskDurability, Report.ExtractedItemCount));

	SwitchToUIInputMode();
	if (APawn* ControlledPawn = GetPawn())
	{
		ControlledPawn->DisableInput(this);
	}
}

void ALSPlayerController::LSSimulateExtract(const FString& HazardStr)
{
	ELSHazardDifficulty Hazard = ELSHazardDifficulty::Medium;
	if (HazardStr.Equals(TEXT("Low"), ESearchCase::IgnoreCase)) Hazard = ELSHazardDifficulty::Low;
	else if (HazardStr.Equals(TEXT("Hard"), ESearchCase::IgnoreCase)) Hazard = ELSHazardDifficulty::Hard;
	else if (HazardStr.Equals(TEXT("Extreme"), ESearchCase::IgnoreCase)) Hazard = ELSHazardDifficulty::Extreme;
	
	if (CorrosionComponent)
	{
		CorrosionComponent->ApplyRaidDurabilityDeduction(Hazard);
	}

	const float TotalVal = BackpackComponent ? BackpackComponent->CalculateTotalLootValue() : 0.0f;
	const int32 ItemCount = BackpackComponent ? (BackpackComponent->BackpackSlots.Num() + BackpackComponent->SecureBoxSlots.Num()) : 0;
	const int32 RemainingDur = CorrosionComponent ? CorrosionComponent->CurrentMaskDurability : 0;

	GEngine->AddOnScreenDebugMessage(-1, 6.0f, FColor::Green,
		FString::Printf(TEXT("✅ [模拟撤离] 难度: %s | 估值: %.0f | 剩余面罩耐久: %d | 物品格: %d"),
			*HazardStr, TotalVal, RemainingDur, ItemCount));
}
