#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "LSTypes.h"
#include "LSEventBus.generated.h"

//1，战斗与伤害事件委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLSDamageDealt, const FLSDamageContext&, DamageContext);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLSEnemyKilled, AActor*, Victim, AActor*, Killer);

//2，元素附着与反应触发事件委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnLSElementApplied, AActor*, Target, FGameplayTag, ElementTag, ELSElementGauge, Gauge);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnLSElementReactionTriggered, AActor*, Target, FGameplayTag, ReactionTag, float, ReactionDamage, AActor*, Instigator);

//3,角色切换与死亡事件委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLSCharacterSwitched, int32, OldCharacterIndex, int32, NewCharacterIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLSCharacterDied, AActor*, DeadCharacter);

UCLASS()
class LUMI_SPARK_API ULSEventBus : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	//静态单行获取接口：在任何 Actor 或 Widget 中输入 ULSEventBus::Get(this) 即可拿到
	static ULSEventBus* Get(const UObject* WorldContextObject);
	
	//战斗事件流
	UPROPERTY(BlueprintAssignable, Category = "Events|Combat")
	FOnLSDamageDealt OnDamageDealt;
	
	UPROPERTY(BlueprintAssignable, Category = "Events|Combat")
	FOnLSEnemyKilled OnEnemyKilled;
	
	//元素事件流
	UPROPERTY(BlueprintAssignable, Category = "Events|Element")
	FOnLSElementApplied OnElementApplied;
	
	UPROPERTY(BlueprintAssignable, Category = "Events|Element")
	FOnLSElementReactionTriggered OnElementReactionTriggered;
	
	//角色与小队事件流
	UPROPERTY(BlueprintAssignable, Category = "Events|Team")
	FOnLSCharacterSwitched OnCharacterSwitched;
	
	UPROPERTY(BlueprintAssignable, Category = "Events|Team")
	FOnLSCharacterDied OnCharacterDied;
};