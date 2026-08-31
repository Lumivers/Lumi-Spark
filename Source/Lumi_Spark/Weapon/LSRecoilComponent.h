#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LSRecoilComponent.generated.h"

/**
 * 武器后坐力与视角回正组件
 * 支持程序化弹道模式（Pattern Recoil）、开镜倍率衰减、随机扰动以及停火平滑回正
 */
UCLASS(ClassGroup=(Weapon), meta=(BlueprintSpawnableComponent))
class LUMI_SPARK_API ULSRecoilComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:
	ULSRecoilComponent();
	
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	//核心控制接口
	
	//单次射击时调用，计算当前发数偏移并施加至视口，同时累加回正池
	UFUNCTION(BlueprintCallable, Category = "Weapon|Recoil")
	void ApplyRecoil(bool bIsADS = false);
	
	//开始持续射击（全自动连射调用）
	UFUNCTION(BlueprintCallable, Category = "Weapon|Recoil")
	void StartRecoil();
	
	//停止射击：触发平滑回正，并重置弹道模式索引
	UFUNCTION(BlueprintCallable, Category = "Weapon|Recoil")
	void StopRecoil();
	
	//换弹或切枪时彻底重置后坐力状态
	UFUNCTION(BlueprintCallable, Category = "Weapon|Recoil")
	void ResetRecoil();
	
protected:
	//后坐力模式配置
	
	//预定义弹道序列：每发子弹的（yaw左右偏，pitch上抬量）
	UPROPERTY(EditDefaultsOnly, Category = "Recoil|Pattern")
	TArray<FVector2D> RecoilPattern;
	
	//全局后坐力强度倍率
	UPROPERTY(EditDefaultsOnly, Category = "Recoil|Modifier", meta = (ClampMin = "0.0"))
	float RecoilMultiplier = 1.0f;
	
	//开镜瞄准时的后坐力倍率（默认减小30抖动）
	UPROPERTY(EditDefaultsOnly, Category = "Recoil|Modifier", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ADSRecoilMultiplier = 0.7f;
	
	//停止射击后准星平滑回正倍率
	UPROPERTY(EditDefaultsOnly, Category = "Recoil|Recovery", meta = (ClampMin = "0.0"))
	float RecoverySpeed = 8.0f;
	
	//在固定弹道基础上叠加的伪随机扰动半径
	UPROPERTY(EditDefaultsOnly, Category = "Recoil|Pattern", meta = (ClampMin = "0.0"))
	float RandomSpreadFactor = 0.08f;
	
	//是否启用停火后自动回正
	UPROPERTY(EditDefaultsOnly, Category = "Recoil|Recovery")
	bool bEnableRecovery = true;
	
private:
	//当前连射第几发
	int32 CurrentPatternIndex = 0;
	
	//是否出于开火连射中
	bool bIsFiring = false;
	
	//累积尚未回正的 (Yaw, Pitch) 偏移量
	FVector2D AccumulatedRecoil = FVector2D::ZeroVector;
	
	//获取拥有该武器/组件的玩家控制器
	APlayerController* GetPlayerController() const;
	
	//施加旋转输入至控制器
	void ApplyInputToController(float PitchDelta, float YawDelta);
	
	///执行Tick平滑回正
	void ProcessRecovery(float DeltaTime);
};