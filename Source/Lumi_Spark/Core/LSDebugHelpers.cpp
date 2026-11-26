#include "Core/LSDebugHelpers.h"
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarLSShowScreenDebug(
	TEXT("LS.ShowScreenDebug"),
	0,
	TEXT("是否显示屏幕左上角调试信息 (0: 关闭以干净录屏, 1: 开启全部调试文本)"),
	ECVF_Default
);

static TAutoConsoleVariable<int32> CVarLSShowWorldDamagePop(
	TEXT("LS.ShowWorldDamagePop"),
	0,
	TEXT("是否显示旧版 3D 调试伤害数值 (0: 关闭以使用 UMG 限量跳字, 1: 开启)"),
	ECVF_Default
);

static TAutoConsoleVariable<int32> CVarLSShowWorldHealthText(
	TEXT("LS.ShowWorldHealthText"),
	0,
	TEXT("是否显示旧版头顶 3D 血量文本 (0: 关闭以使用头顶常驻绿血条, 1: 开启)"),
	ECVF_Default
);

namespace LSDebug
{
	bool IsScreenDebugEnabled()
	{
		return CVarLSShowScreenDebug.GetValueOnAnyThread() != 0;
	}

	bool IsWorldDamagePopEnabled()
	{
		return CVarLSShowWorldDamagePop.GetValueOnAnyThread() != 0;
	}

	bool IsWorldHealthTextEnabled()
	{
		return CVarLSShowWorldHealthText.GetValueOnAnyThread() != 0;
	}
}
