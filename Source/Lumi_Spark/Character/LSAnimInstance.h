#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Core/LSTypes.h"
#include "LSAnimInstance.generated.h"

class ALSCharacterBase;
class ULSMovementComponent;

/**
 * 角色主控动画实例基类 (C++ AnimInstance)
 * 在底层 C++ 中高频提取角色的移动、开火、瞄准、跳跃等关键状态，
 * 为动画图表 (AnimGraph) 提供零蓝图开销的驱动参数。
 */
UCLASS()
class LUMI_SPARK_API ULSAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	//角色与组件指针缓存
	UPROPERTY(BlueprintReadOnly, Category = "Character")
	TObjectPtr<ALSCharacterBase> Character = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Character")
	TObjectPtr<ULSMovementComponent> MovementComponent = nullptr;

	//运动学动画驱动参数

	//地面水平移动速率 (单位: cm/s)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float GroundSpeed = 0.0f;

	// 移动方向偏角 (-180° ~ 180°)，用于8方向移动混合空间 (BlendSpace)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float Direction = 0.0f;

	// 是否拥有有效移动输入并应当播放走跑动作
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bShouldMove = false;

	//是否在空中滞空/坠落
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bIsFalling = false;

	// ═══ 战术动作状态 ═══

	//是否处于下蹲状态
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bIsCrouching = false;

	// 是否处于冲刺加速状态
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bIsSprinting = false;

	// 是否处于滑铲状态
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bIsSliding = false;

	//是否处于闪避无敌帧状态
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bIsDashing = false;

	//战斗与射击瞄准状态

	//是否处于右键开镜瞄准状态 (ADS)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	bool bIsAiming = false;

	//视口仰俯角 (-90° ~ 90°)，用于 AimOffset 瞄准偏移
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	float AimPitch = 0.0f;

	//视口偏航相对角 (-180° ~ 180°)，用于 AimOffset 瞄准偏移
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	float AimYaw = 0.0f;
};
