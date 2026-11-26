#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"

namespace LSDebug
{
	/** 是否开启屏幕左上角调试日志（0=关闭，适合干净录屏；1=开启） */
	LUMI_SPARK_API bool IsScreenDebugEnabled();

	/** 是否开启 3D 世界空间伤害飘字（DrawDebugString） */
	LUMI_SPARK_API bool IsWorldDamagePopEnabled();

	/** 是否开启 3D 敌人头顶血量文字（DrawDebugString） */
	LUMI_SPARK_API bool IsWorldHealthTextEnabled();
}

/** 受控屏幕调试打印宏：未启用时不构建字符串，避免录屏污染与每帧字符串分配开销 */
#define LS_SCREEN_DEBUG(Key, TimeToDisplay, DisplayColor, DebugMessage) \
	do \
	{ \
		if (GEngine && LSDebug::IsScreenDebugEnabled()) \
		{ \
			GEngine->AddOnScreenDebugMessage((Key), (TimeToDisplay), (DisplayColor), (DebugMessage)); \
		} \
	} while (0)
