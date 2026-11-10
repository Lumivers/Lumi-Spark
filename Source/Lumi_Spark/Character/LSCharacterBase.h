#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LSMovementComponent.h"
#include "Core/LSTypes.h"
#include "Core/LSInteractableInterface.h"
#include "LSCharacterBase.generated.h"

// 前向声明：基础设施与玩法组件
class ULSCameraComponent;
class USpringArmComponent;
class ULSMovementComponent;
class USkeletalMeshComponent;
class ULSWeaponComponent;
class ULSElementComponent;
class ULSSkillComponent;
class ULSHealthComponent;
class ULSStaminaComponent;
class ULSEnergyComponent;
class ULSThrowableComponent;

UCLASS()
class LUMI_SPARK_API ALSCharacterBase : public ACharacter, public ILSInteractableInterface
{
	GENERATED_BODY()

public:
	ALSCharacterBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 引擎统一承伤入口：内部直接分流至 HealthComponent 权威结算
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
	
	// ILSInteractableInterface 交互接口实现
	virtual bool CanInteract(AActor* Interactor) const override;
	virtual FText GetInteractPrompt(AActor* Interactor) const override;
	virtual float GetInteractDuration(AActor* Interactor) const override;
	virtual void OnInteractComplete(AActor* Interactor) override;
	
	// ─── 统一组件访问器 ───
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE ULSCameraComponent* GetCameraComponent() const { return CameraComponent; }
	FORCEINLINE ULSMovementComponent* GetLSMovementComponent() const { return Cast<ULSMovementComponent>(GetCharacterMovement()); }
	FORCEINLINE ULSWeaponComponent* GetWeaponComponent() const { return WeaponComponent; }
	FORCEINLINE ULSElementComponent* GetElementComponent() const { return ElementComponent; }
	FORCEINLINE ULSSkillComponent* GetSkillComponent() const { return SkillComponent; }
	FORCEINLINE ULSHealthComponent* GetHealthComponent() const { return HealthComponent; }
	FORCEINLINE ULSStaminaComponent* GetStaminaComponent() const { return StaminaComponent; }
	FORCEINLINE ULSEnergyComponent* GetEnergyComponent() const { return EnergyComponent; }
	FORCEINLINE ULSThrowableComponent* GetThrowableComponent() const { return ThrowableComponent; }

	// 战斗/射击模式：角色是否始终正对准星（指哪转哪，扫射步法）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Rotation")
	bool bAlwaysOrientToControlRotation = true;

	// 动态切换战斗朝向模式（开火或瞄准时锁定准星并通知动画系统进入 Strafe 模式；退出后恢复自由跑动）
	UFUNCTION(BlueprintCallable, Category = "Character|Rotation")
	virtual void SetCombatFacing(bool bEnable);

	// 向底层动画/状态机（如 ALS 蓝图接口）派发旋转模式指令 (0: VelocityDirection, 1: LookingDirection, 2: Aiming)
	UFUNCTION(BlueprintCallable, Category = "Character|Rotation")
	void SetALSRotationMode(uint8 InRotationMode);

	// 向底层动画/状态机（如 ALS 蓝图接口）派发步态指令 (0: Walking, 1: Running, 2: Sprinting)
	UFUNCTION(BlueprintCallable, Category = "Character|Gait")
	void SetALSDesiredGait(uint8 InGait);

	// 向底层动画/状态机（如 ALS 蓝图接口）派发覆层姿态指令 (0: Default, 1: Rifle, 2: Bow, 3: Pistol 1H, etc.)
	UFUNCTION(BlueprintCallable, Category = "Character|Overlay")
	void SetALSOverlayState(uint8 InOverlayState);

	// 向底层动画蓝图派发覆层覆盖状态指令 (0: 默认摆臂, 1: 锁死手臂无晃动稳固姿态 Poses_NoCurves, 2: 全身锁定)
	UFUNCTION(BlueprintCallable, Category = "Character|Overlay")
	void SetALSOverlayOverrideState(int32 InOverrideState);

	// 是否在平时走动时也保持手臂稳固（避免走路时步枪像大钟摆一样夸张摇晃）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Combat")
	bool bLockArmSwayDuringWalk = true;

	// ═══ 战术开火：角色躯干与脊椎程序化受力后坐 (Spine Procedural Recoil) ═══

	// 当前帧计算出的脊椎后坐旋转偏移（直接连入 AnimGraph 骨骼变换节点）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Recoil")
	FRotator SpineRecoilRotation = FRotator::ZeroRotator;

	// 当前帧计算出的脊椎后坐位移偏移
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Recoil")
	FVector SpineRecoilLocation = FVector::ZeroVector;

	// 单发开火身体后仰角度冲量（Pitch，度）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Recoil")
	float RecoilSpinePitch = 2.0f;

	// 单发开火右肩后挫偏航扭矩（Yaw，度）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Recoil")
	float RecoilSpineYaw = 0.6f;

	// 单发开火身体侧倾扰动随机范围（Roll，度）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Recoil")
	float RecoilSpineRoll = 0.3f;

	// 单发开火躯干后挫位移冲量（cm）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Recoil")
	float RecoilSpineBack = 0.8f;

	// 连续开火最大累计仰角限制（防止全自动连续射击时仰角过大）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Recoil")
	float MaxRecoilSpinePitch = 5.0f;

	// 连续开火最大累计后挫位移限制（cm）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Recoil")
	float MaxRecoilSpineBack = 2.0f;

	// 躯干后坐力平滑弹性回正阻尼速率
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Recoil")
	float RecoilSpineRecoverySpeed = 24.0f;

	// 触发一次躯干受力脉冲（由武器开火或技能调用）
	UFUNCTION(BlueprintCallable, Category = "Combat|Recoil")
	void AddSpineRecoilKick(float Multiplier = 1.0f);

	// 角色固有元素属性（供同色微粒判定）
	UFUNCTION(BlueprintPure, Category = "Character|Identity")
	FGameplayTag GetCharacterElementTag() const { return CharacterElementTag; }

	// 存活状态：直接向生命组件查询唯一真相源
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Health")
	bool IsDead() const;
	
	// 倒地与救援生命周期
	UFUNCTION(BlueprintCallable, Category = "Health|Downed")
	bool IsDowned() const { return bIsDowned; }
	
	UFUNCTION(BlueprintCallable, Category = "Health|Downed")
	float GetBleedoutRatio() const { return DownedBleedoutMaxTime > 0.0f ? (BleedoutRemainingTimer / DownedBleedoutMaxTime) : 0.0f; }
	
	UFUNCTION(BlueprintCallable, Category = "Health|Downed")
	virtual void EnterDownedState(AActor* Killer);
	
	UFUNCTION(BlueprintCallable, Category = "Health|Downed")
	virtual void Revive(AActor* Reviver, float RestoredHealthPercent = 0.5f);
	
	//角色gameplay容器与状态查询
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "State")
	FGameplayTagContainer ActiveGameplayTags;
	
	UFUNCTION(BlueprintCallable, Category = "State")
	void AddGameplayTag(FGameplayTag Tag) { ActiveGameplayTags.AddTag(Tag); }
	
	UFUNCTION(BlueprintCallable, Category = "State")
	void RemoveGameplayTag(FGameplayTag Tag) { ActiveGameplayTags.RemoveTag(Tag); }
	
	UFUNCTION(BlueprintPure, Category = "State")
	bool HasMatchingGameplayTag(FGameplayTag Tag) const { return ActiveGameplayTags.HasTag(Tag); }
	
	UFUNCTION(BlueprintPure, Category = "State")
	bool HasSuperArmor() const { return ActiveGameplayTags.HasTag(LSTags::TAG_State_SuperArmor); }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULSCameraComponent> CameraComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULSMovementComponent> LSMovementComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULSWeaponComponent> WeaponComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Element", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULSElementComponent> ElementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skill", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULSSkillComponent> SkillComponent;

	// ═══ 三大独立资源组件 ═══
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULSHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stamina", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULSStaminaComponent> StaminaComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Energy", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULSEnergyComponent> EnergyComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Throwable", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULSThrowableComponent> ThrowableComponent;

	// 角色固有属性
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Identity")
	FGameplayTag CharacterElementTag;

	// 倒地网络状态同步
	UPROPERTY(ReplicatedUsing = OnRep_IsDowned, VisibleInstanceOnly, BlueprintReadOnly, Category = "Health|Downed")
	bool bIsDowned = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health|Downed")
	float DownedBleedoutMaxTime = 45.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health|Downed")
	float DownedWalkSpeed = 150.0f;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Health|Downed")
	float BleedoutRemainingTimer = 0.0f;
	
	FTimerHandle BleedoutTimerHandle;
	
	UFUNCTION()
	void OnRep_IsDowned();
	
	void HandleBleedoutTick();

	// 彻底死亡回调
	virtual void Die(AActor* Killer);

	// 响应生命组件广播的 OnDeath 事件
	UFUNCTION()
	void HandleHealthComponentDeath();
};