#include "LSPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "LSCharacterBase.h"
#include "LSCameraComponent.h"
#include "LSMovementComponent.h"
#include "Weapon/LSWeaponComponent.h"

ALSPlayerController::ALSPlayerController()
{
	bShowMouseCursor = false;
}

void ALSPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	//默认进入纯游戏战斗输入模式
	SwitchToGameInputMode();
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
		if (IA_Interact) EnhancedInputComponent->BindAction(IA_Interact, ETriggerEvent::Started, this, &ALSPlayerController::HandleInteract);
		
		if (IA_SwitchWeapon1) EnhancedInputComponent->BindAction(IA_SwitchWeapon1, ETriggerEvent::Started, this, &ALSPlayerController::HandleSwitchWeapon1);
		if (IA_SwitchWeapon2) EnhancedInputComponent->BindAction(IA_SwitchWeapon2, ETriggerEvent::Started, this, &ALSPlayerController::HandleSwitchWeapon2);
		if (IA_QuickSwitchWeapon) EnhancedInputComponent->BindAction(IA_QuickSwitchWeapon, ETriggerEvent::Started, this, &ALSPlayerController::HandleQuickSwitchWeapon);
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
	// 预留：通知 SkillComponent 释放 E 战技
}

void ALSPlayerController::HandleBurst()
{
	// 预留：通知 SkillComponent 释放 Q 爆发大招
}

void ALSPlayerController::HandleThrowGrenadeStarted()
{
	// 预留：显示元素手雷抛物线轨迹预览 (Niagara Spline)
}

void ALSPlayerController::HandleThrowGrenadeCompleted()
{
	// 预留：按物理抛物线掷出水/火/冰/雷元素手雷
}

void ALSPlayerController::HandleSwitchCharacter()
{
	// 预留：通知 TeamSwitchComponent 执行 Tab 键双人小队对调
}

void ALSPlayerController::HandleInteract()
{
	// 预留：拾取掉落物 / 交互
}

void ALSPlayerController::HandleSwitchWeapon1()
{
	GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan, TEXT("🔄 [Input] 按了 1 键切主武器！"));
	if (ALSCharacterBase* Char = GetPawn<ALSCharacterBase>())
		if (ULSWeaponComponent* Comp = Char->GetWeaponComponent()) Comp->EquipWeapon(ELSWeaponSlot::MainWeapon);
}
void ALSPlayerController::HandleSwitchWeapon2()
{
	GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan, TEXT("🔄 [Input] 按了 2 键切副武器！"));
	if (ALSCharacterBase* Char = GetPawn<ALSCharacterBase>())
		if (ULSWeaponComponent* Comp = Char->GetWeaponComponent()) Comp->EquipWeapon(ELSWeaponSlot::SubWeapon);
}

void ALSPlayerController::HandleQuickSwitchWeapon()
{
	if (ALSCharacterBase* Char = GetPawn<ALSCharacterBase>())
		if (ULSWeaponComponent* Comp = Char->GetWeaponComponent()) Comp->QuickSwitchWeapon();
}