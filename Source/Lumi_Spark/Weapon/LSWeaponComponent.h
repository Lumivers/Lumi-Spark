#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/LSTypes.h"
#include "LSWeaponComponent.generated.h"

class ALSWeaponBase;

UCLASS(ClassGroup=(Custom), meta = (BlueprintSpawnableComponent))
class LUMI_SPARK_API ULSWeaponComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:
	ULSWeaponComponent();
	virtual void BeginPlay() override;
	
	//获取当前手持武器
	FORCEINLINE ALSWeaponBase* GetCurrentWeapon() const { return CurrentWeapon; }
	FORCEINLINE ELSWeaponSlot GetCurrentSlot() const { return CurrentSlot; }
	
	//插槽名称配置
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Socket")
	FName HandSocketName = FName("hand_r");
	
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Socket")
	FName HolsterSocketName = FName("WeaponHolsterSocket");
	
	//1，指定槽位切枪
	UFUNCTION(BlueprintCallable, Category = "Weapon|Switch")
	void EquipWeapon(ELSWeaponSlot NewSlot);
	
	UFUNCTION(BlueprintCallable, Category = "Weapon|Switch")
	void QuickSwitchWeapon(); //快速切换主副武器
	
	void StartFire();
	void StopFire();
	void Reload();
	
	//蓝图中配置的默认主副武器类
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Classes")
	TSubclassOf<ALSWeaponBase> DefaultPrimaryClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Classes")
	TSubclassOf<ALSWeaponBase> DefaultSecondaryClass;
	
protected:
	//运行时实例化的主副武器Actor指针
	UPROPERTY(Transient)
	TObjectPtr<ALSWeaponBase> PrimaryWeapon = nullptr;
	
	UPROPERTY(Transient)
	TObjectPtr<ALSWeaponBase> SecondaryWeapon = nullptr;
	
	UPROPERTY(Transient)
	TObjectPtr<ALSWeaponBase> CurrentWeapon = nullptr;
	
	ELSWeaponSlot CurrentSlot = ELSWeaponSlot::MainWeapon;
	
private:
	ALSWeaponBase* SpawnWeapon(TSubclassOf<ALSWeaponBase> WeaponClass);
	void AttachWeaponToSocket(ALSWeaponBase* Weapon, FName SocketName);
};