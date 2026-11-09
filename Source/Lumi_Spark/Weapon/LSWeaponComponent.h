#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/LSTypes.h"
#include "LSWeaponComponent.generated.h"

class ALSWeaponBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLSWeaponChanged, ALSWeaponBase*, NewWeapon);

UCLASS(ClassGroup=(Custom), meta = (BlueprintSpawnableComponent))
class LUMI_SPARK_API ULSWeaponComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:
	ULSWeaponComponent();
	virtual void BeginPlay() override;
	
	//获取当前手持武器
	FORCEINLINE ALSWeaponBase* GetCurrentWeapon() const { return CurrentWeapon; }
	
	// 插槽名称配置
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Socket")
	FName HandSocketName = FName("WeaponSocket");

	// 背部收纳插槽名称（切换到副手时挂在后背）
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Socket")
	FName BackSocketName = FName("Weapon_Back");

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FOnLSWeaponChanged OnWeaponChanged;
	
	void StartFire();
	void StopFire();
	void Reload();

	// ═══ 单人双武器系统接口 ═══
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void EquipPrimaryWeapon();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void EquipSecondaryWeapon();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void ToggleWeapon();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SwitchWeapon(int32 SlotIndex);

	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetCurrentWeaponSlotIndex() const { return CurrentWeaponIndex; }

	// 蓝图中配置的默认主副武器类
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Classes", meta = (DisplayName = "默认主武器类 (1号位)"))
	TSubclassOf<ALSWeaponBase> DefaultPrimaryClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Classes", meta = (DisplayName = "默认副武器类 (2号位)"))
	TSubclassOf<ALSWeaponBase> DefaultSecondaryClass;

protected:
	// 运行时实例化的主副武器 Actor 指针
	UPROPERTY(Replicated)
	TObjectPtr<ALSWeaponBase> PrimaryWeapon = nullptr;

	UPROPERTY(Replicated)
	TObjectPtr<ALSWeaponBase> SecondaryWeapon = nullptr;

	// 当前激活的手持武器指针
	UPROPERTY(ReplicatedUsing = OnRep_CurrentWeapon)
	TObjectPtr<ALSWeaponBase> CurrentWeapon = nullptr;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentWeaponIndex)
	int32 CurrentWeaponIndex = 0;

	UFUNCTION()
	void OnRep_CurrentWeapon(ALSWeaponBase* OldWeapon);

	UFUNCTION()
	void OnRep_CurrentWeaponIndex();
	
private:
	ALSWeaponBase* SpawnWeapon(TSubclassOf<ALSWeaponBase> WeaponClass);
	void AttachWeaponToSocket(ALSWeaponBase* Weapon, FName SocketName);
};