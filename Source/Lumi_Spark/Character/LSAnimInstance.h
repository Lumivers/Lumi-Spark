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

	//是否处于闪避无敌帧状态
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bIsDashing = false;

	//战斗与射击瞄准状态

	//是否处于右键开镜瞄准状态 (ADS)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	bool bIsAiming = false;

	// 是否处于开火射击状态 (鼠标左键开火)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	bool bIsFiring = false;

	//视口仰俯角 (-90° ~ 90°)，用于 AimOffset 瞄准偏移
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	float AimPitch = 0.0f;

	//视口偏航相对角 (-180° ~ 180°)，用于 AimOffset 瞄准偏移
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	float AimYaw = 0.0f;

	// ═══ 武器与左手逆运动学 (Left Hand IK) ═══

	// 左手吸附目标变换（在角色 Mesh 的组件空间 Component Space，直接连入 TwoBoneIK）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "IK")
	FTransform LeftHandIKTransform = FTransform::Identity;

	// 左手吸附位置向量（方便蓝图直接连线）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "IK")
	FVector LeftHandIKLocation = FVector::ZeroVector;

	// 左手吸附旋转量
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "IK")
	FRotator LeftHandIKRotation = FRotator::ZeroRotator;

	// 是否激活左手 IK 权重（当前武器有 LeftHandSocket 则平滑为 1.0，否则为 0.0）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "IK")
	float LeftHandIKAlpha = 0.0f;

	// ═══ 角色身体开火受力抖动 (Procedural Spine Recoil) ═══

	// 当前计算得到的脊椎受力后坐旋转偏移（连入 AnimGraph 的 Transform Bone）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Recoil")
	FRotator SpineRecoilRotation = FRotator::ZeroRotator;

	// 当前计算得到的脊椎受力后挫位移偏移（连入 AnimGraph 的 Transform Bone）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Recoil")
	FVector SpineRecoilLocation = FVector::ZeroVector;

	// 单发后坐躯干后仰仰角冲量基础值（度）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil|Settings")
	float RecoilSpinePitch = 2.0f;

	// 单发后坐右肩后抗产生的偏航扭矩基础值（度）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil|Settings")
	float RecoilSpineYaw = 0.6f;

	// 单发后坐侧倾扰动随机范围（度）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil|Settings")
	float RecoilSpineRoll = 0.3f;

	// 单发后坐躯干后挫位移冲量（cm）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil|Settings")
	float RecoilSpineBack = 0.8f;

	// 连续开火时脊椎最大累积仰角
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil|Settings")
	float MaxRecoilSpinePitch = 5.0f;

	// 连续开火时脊椎最大后挫位移（cm）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil|Settings")
	float MaxRecoilSpineBack = 2.0f;

	// 开火后坐弹性平滑回正速率
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil|Settings")
	float RecoilSpineRecoverySpeed = 24.0f;

public:
	// 触发一次开火后坐力受力脉冲
	UFUNCTION(BlueprintCallable, Category = "Recoil")
	void AddRecoilKick(float Multiplier = 1.0f);
};

