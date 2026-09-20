#include "LSPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "LSCharacterBase.h"
#include "LSCameraComponent.h"
#include "LSMovementComponent.h"
#include "Weapon/LSWeaponComponent.h"
#include "Blueprint/UserWidget.h"
#include "Character/LSTeamSwitchComponent.h"
#include "Character/LSSkillComponent.h"
#include "Weapon/LSGrenadeBase.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "UI/LSUHDWidget.h"

ALSPlayerController::ALSPlayerController()
{
	bShowMouseCursor = false;

	TeamSwitchComponent = CreateDefaultSubobject<ULSTeamSwitchComponent>(TEXT("TeamSwitchComponent"));
}

void ALSPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	//默认进入纯游戏战斗输入模式
	SwitchToGameInputMode();
	
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
	
	// 仅在服务端权威且小队未初始化时，静默拉起小队待命副角色
	if (HasAuthority() && TeamSwitchComponent)
	{
		if (ALSCharacterBase* LSChar = Cast<ALSCharacterBase>(InPawn))
		{
			if (TeamSwitchComponent->GetActiveCharacter() == nullptr)
			{
				TeamSwitchComponent->SetupTeam(LSChar, StandbyCharacterClasses);
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
		if (IA_ToggleView) EnhancedInputComponent->BindAction(IA_ToggleView, ETriggerEvent::Started, this, &ALSPlayerController::HandleToggleView);
		
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
	if (ALSCharacterBase* Char = GetPawn<ALSCharacterBase>()) Char->Jump();
}

void ALSPlayerController::HandleJumpCompleted()
{
	if (ALSCharacterBase* Char = GetPawn<ALSCharacterBase>()) Char->StopJumping();
}

void ALSPlayerController::HandleSprintStarted()
{
	if (ALSCharacterBase* Char = GetPawn<ALSCharacterBase>())
	{
		if (ULSMovementComponent* MoveComp = Char->GetLSMovementComponent()) MoveComp->StartSprint();
	}
}

void ALSPlayerController::HandleSprintCompleted()
{
	if (ALSCharacterBase* Char = GetPawn<ALSCharacterBase>())
	{
		if (ULSMovementComponent* MoveComp = Char->GetLSMovementComponent()) MoveComp->StopSprint();
	}
}

void ALSPlayerController::HandleDash()
{
	if (ALSCharacterBase* Char = GetPawn<ALSCharacterBase>())
	{
		if (ULSMovementComponent* MoveComp = Char->GetLSMovementComponent()) MoveComp->TryDash();
	}
}

void ALSPlayerController::HandleCrouchStarted()
{
	if (ALSCharacterBase* Char = GetPawn<ALSCharacterBase>())
	{
		if (ULSMovementComponent* MoveComp = Char->GetLSMovementComponent())
		{
			if (MoveComp->IsSprinting()) MoveComp->StartSlide();
			else Char->Crouch();
		}
	}
}

void ALSPlayerController::HandleCrouchCompleted()
{
	if (ALSCharacterBase* Char = GetPawn<ALSCharacterBase>())
	{
		if (ULSMovementComponent* MoveComp = Char->GetLSMovementComponent())
		{
			if (MoveComp->IsSliding()) MoveComp->StopSlide();
		}
		Char->UnCrouch();
	}
}

void ALSPlayerController::HandleToggleView()
{
	if (ALSCharacterBase* Char = GetPawn<ALSCharacterBase>())
	{
		if (ULSCameraComponent* Cam = Char->GetCameraComponent()) Cam->ToggleCameraMode();
	}
}

void ALSPlayerController::HandleFireStarted()
{
	GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("🎯 [Input] 触发了鼠标左键开火！"));
	if (ALSCharacterBase* Char = GetPawn<ALSCharacterBase>())
	{
		if (ULSWeaponComponent* WeaponComp = Char->GetWeaponComponent())
		{
			WeaponComp->StartFire();
		}
	}
}

void ALSPlayerController::HandleFireCompleted()
{
	if (ALSCharacterBase* Char = GetPawn<ALSCharacterBase>())
	{
		if (ULSWeaponComponent* WeaponComp = Char->GetWeaponComponent()) WeaponComp->StopFire();
	}
}

void ALSPlayerController::HandleADSStarted()
{
	bIsAiming = true;
	if (ALSCharacterBase* Char = GetPawn<ALSCharacterBase>())
	{
		if (ULSCameraComponent* Cam = Char->GetCameraComponent()) Cam->EnterADS();
		if (ULSMovementComponent* MoveComp = Char->GetLSMovementComponent()) MoveComp->SetAiming(true);
	}
}

void ALSPlayerController::HandleADSCompleted()
{
	bIsAiming = false;
	if (ALSCharacterBase* Char = GetPawn<ALSCharacterBase>())
	{
		if (ULSCameraComponent* Cam = Char->GetCameraComponent()) Cam->ExitADS();
		if (ULSMovementComponent* MoveComp = Char->GetLSMovementComponent()) MoveComp->SetAiming(false);
	}
}

void ALSPlayerController::HandleReload()
{
	if (ALSCharacterBase* Char = GetPawn<ALSCharacterBase>())
	{
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
	
}

void ALSPlayerController::HandleThrowGrenadeCompleted()
{
	ALSCharacterBase* Char = GetPawn<ALSCharacterBase>();
	if (!Char || !Char->HasAuthority()) return;

	//从摄像机视口正中央向前投掷
	FVector CameraLoc;
	FRotator CameraRot;
	GetPlayerViewPoint(CameraLoc, CameraRot);

	// 投掷起点向前微调，防止手雷生成在自身胶囊体内引发碰撞穿模
	const FVector SpawnLoc = CameraLoc + (CameraRot.Vector() * 80.0f);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Char;
	SpawnParams.Instigator = Char;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 生成配置的基础手雷
	if (DefaultGrenadeClass)
	{
    	if (ALSGrenadeBase* Grenade = GetWorld()->SpawnActor<ALSGrenadeBase>(DefaultGrenadeClass, SpawnLoc, CameraRot, SpawnParams))
    	{
        	// 给手雷一个朝向准星仰角的初速度向量
        	if (UProjectileMovementComponent* ProjComp = Grenade->GetProjectileMovement())
        	{
            	ProjComp->Velocity = CameraRot.Vector() * ProjComp->InitialSpeed;
        	}
    	}
	}
}

void ALSPlayerController::HandleSwitchCharacter()
{
	if (TeamSwitchComponent)
	{
		TeamSwitchComponent->CycleNextCharacter(true);
	}
}

void ALSPlayerController::HandleSwitchToSlot1()
{
	if (TeamSwitchComponent) TeamSwitchComponent->SwitchTo(0);
}

void ALSPlayerController::HandleSwitchToSlot2()
{
	if (TeamSwitchComponent) TeamSwitchComponent->SwitchTo(1);
}

void ALSPlayerController::HandleSwitchToSlot3()
{
	if (TeamSwitchComponent) TeamSwitchComponent->SwitchTo(2);
}

void ALSPlayerController::HandleCycleCharacter(const FInputActionValue& Value)
{
	if (!TeamSwitchComponent) return;
	const float AxisVal = Value.Get<float>();
	if (FMath::IsNearlyZero(AxisVal)) return;
	// 滚轮向上 (>0) 顺切，滚轮向下 (<0) 逆切
	TeamSwitchComponent->CycleNextCharacter(AxisVal > 0.0f);
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

			// 长按时间达标 -> 完成交互！
			if (InteractTimer >= CurrentInteractDuration)
			{
				Server_CompleteInteract(Target);
				Interface->OnInteractComplete(ControlledPawn);
				CurrentInteractTarget = nullptr;
				InteractTimer = 0.0f;
			}
		}
	}
}

void ALSPlayerController::HandleInteractStarted()
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn) return;

	// 从摄像机视线向前 2.5 米做球体检测抓取可交互对象
	FVector CameraLoc;
	FRotator CameraRot;
	GetPlayerViewPoint(CameraLoc, CameraRot);

	const FVector TraceEnd = CameraLoc + CameraRot.Vector() * 250.0f;
	FHitResult Hit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(ControlledPawn);

	if (GetWorld()->SweepSingleByChannel(Hit, CameraLoc, TraceEnd, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(40.0f), QueryParams))
	{
		if (Hit.GetActor() && Hit.GetActor()->Implements<ULSInteractableInterface>())
		{
			ILSInteractableInterface* Interface = Cast<ILSInteractableInterface>(Hit.GetActor());
			if (Interface && Interface->CanInteract(ControlledPawn))
			{
				const float Duration = Interface->GetInteractDuration(ControlledPawn);
				if (Duration <= 0.0f)
				{
					// 单击瞬时交互
					Server_CompleteInteract(Hit.GetActor());
					Interface->OnInteractComplete(ControlledPawn);
				}
				else
				{
					// 长按蓄力交互（倒地救人）
					CurrentInteractTarget = Hit.GetActor();
					CurrentInteractDuration = Duration;
					InteractTimer = 0.0f;
					Interface->OnInteractStart(ControlledPawn);
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