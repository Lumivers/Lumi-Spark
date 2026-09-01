// Copyright Epic Games, Inc. All Rights Reserved.

#include "Lumi_SparkGameMode.h"
#include "LSPlayerController.h"

ALumi_SparkGameMode::ALumi_SparkGameMode()
{
	//指定默认玩家控制器
	PlayerControllerClass = ALSPlayerController::StaticClass();
	
	//不在 C++ 里硬编码 DefaultPawnClass！
	//请在 BP_GameMode 蓝图中设置 Default Pawn Class = 你的角色蓝图（如 BP_LSCharacter_Base）
	//或者在关卡中放置的角色蓝图上设置 Auto Possess Player = Player 0
	DefaultPawnClass = nullptr;
}
