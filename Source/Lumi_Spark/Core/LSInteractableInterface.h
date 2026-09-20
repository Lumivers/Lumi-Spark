#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "LSInteractableInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class ULSInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 通用交互接口
 * 供倒地角色拉起、物资箱、机关门等实现
 */
class LUMI_SPARK_API ILSInteractableInterface
{
	GENERATED_BODY()
	
public:
	//是否允许发起交互
	virtual bool CanInteract(AActor* Interactor) const = 0;
	
	//获取交互浮窗文本提示
	virtual FText GetInteractPrompt(AActor* Interactor) const = 0;
	
	/**
	 * 所需长按蓄力时长（秒）
	 * 返回 0.0f 表示单击瞬时触发；返回 > 0.0f（如 3.0f）表示长按充能触发
	 */
	virtual float GetInteractDuration(AActor* Interactor) const { return 0.0f; }
	
	//交互开始瞬间（按下 F 键）
	virtual void OnInteractStart(AActor* Interactor) {}
	
	//交互进行中（每帧由 Controller 驱动调用，Progress: 0.0 ~ 1.0）
	virtual void OnInteractProgress(AActor* Interactor, float Progress) {}
	
	//交互成功完成（进度满 100%）
	virtual void OnInteractComplete(AActor* Interactor) {}
	
	//交互被取消或打断（中途松开按键、玩家被怪物击退击飞、离开交互半径）
	virtual void OnInteractCanceled(AActor* Interactor) {}
};