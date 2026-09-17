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
	
	//插槽名称配置
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Socket")
	FName HandSocketName = FName("hand_r");
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FOnLSWeaponChanged OnWeaponChanged;
	
	void StartFire();
	void StopFire();
	void Reload();
	
	//蓝图中配置的默认主副武器类
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Classes", meta = (DisplayName = "Default Weapon Class"))
	TSubclassOf<ALSWeaponBase> DefaultPrimaryClass;
	
protected:
	//运行时实例化的主武器Actor指针
	UPROPERTY(ReplicatedUsing = OnRep_CurrentWeapon)
	TObjectPtr<ALSWeaponBase> CurrentWeapon = nullptr;
	
	UFUNCTION()
	void OnRep_CurrentWeapon(ALSWeaponBase* OldWeapon);
	
private:
	ALSWeaponBase* SpawnWeapon(TSubclassOf<ALSWeaponBase> WeaponClass);
	void AttachWeaponToSocket(ALSWeaponBase* Weapon, FName SocketName);
};