// Copyright Epic Games, Inc. All Rights Reserved.

#include "Lumi_SparkGameMode.h"
#include "LSPlayerController.h"
#include "LSCharacterBase.h"

ALumi_SparkGameMode::ALumi_SparkGameMode()
{
	//指定默认玩家控制器
	PlayerControllerClass = ALSPlayerController::StaticClass();
	
	//指定默认生成的Pawn角色基类
	DefaultPawnClass = ALSCharacterBase::StaticClass();
}
