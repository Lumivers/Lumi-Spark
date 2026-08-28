#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"
#include "LSTypes.generated.h"

//武器装备槽位枚举
UENUM(Blueprintable)
enum class ELSWeaponSlot : uint8
{
	MainWeapon UMETA(DisplayName = "主武器 (slot 1)"),
	SubWeapon UMETA(DisplayName = "副武器 (slot 2)"),
	Throwable UMETA(DisplayName = "元素投掷物 (slot 3)")
};

//元素附着量级
UENUM(Blueprintable)
enum class ELSElementGauge : uint8
{
	None = 0,
	Light = 1	UMETA(DisplayName = "弱元素（1U，衰减9.5s）"),
	Heavy = 2	UMETA(DisplayName = "强元素（2U，衰减12s）"),
	SuperHeavy = 4	UMETA(DisplayName = "超强元素（4U，衰减17s）")
};

//战斗伤害上下文（用于攻击判定，暴击结算与伤害广播）
USTRUCT(Blueprintable)
struct FLSDamageContext
{
	GENERATED_BODY()
	
	//攻击来源Actor（玩家或者敌人）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	TObjectPtr<AActor> DamageCauser = nullptr;
	
	//受击目标Actor
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	TObjectPtr<AActor> TargetActor = nullptr;
	
	//基础伤害数值
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float BaseDamage = 0.f;
	
	//最终结算伤害（经过公式计算之后）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float FinalDamage = 0.f;
	
	//元素类型标签
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	FGameplayTag ElementTag;
	
	//伤害来源类型
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	FGameplayTag DamageTypeTag;
	
	//是否命中弱点
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	bool bIsHeadshot = false;
	
	//是否暴击
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	bool bIsCritical = false;
	
	//是否为元素反应伤害
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	bool bIsReactionDamage = false;
	
	//命中点物理检测信息
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	FHitResult HitResult;
};

//cpp gameplayTags集中管理
namespace LSTags
{
	//角色行为状态
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Idle);
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Moving);
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Sprinting);
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Dashing);
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Sliding);
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_ADS);
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Invincible);
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Dead);
	
	//七大元素类型
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Element_Pyro);//火
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Element_Hydro);//水
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Element_Dendro);//草
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Element_Electro);//雷
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Element_Anemo);//风
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Element_Cryo);//冰
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Element_Geo);//岩
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Element_Physical);//物理
	
	//元素反应类型
	//1，增幅反应
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Reaction_Vaporize);      // 蒸发 (火+水)
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Reaction_Melt);          // 融化 (火+冰)
	
	//2,剧变反应
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Reaction_Overload);      // 超载 (火+雷)
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Reaction_Superconduct);  // 超导 (冰+雷)
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Reaction_Freeze);        // 冻结 (水+冰)
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Reaction_Shatter);       // 碎冰 (冻结+重击/破甲)
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Reaction_ElectroCharged);// 感电 (水+雷)
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Reaction_Swirl);         // 扩散 (风+元素)
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Reaction_Crystallize);   // 结晶 (岩+元素)
	
	//3，草系反应
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Reaction_Burning);       // 燃烧 (火+草)
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Reaction_Bloom);         // 绽放 (水+草)
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Reaction_Hyperbloom);    // 超绽放 (草种子+雷)
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Reaction_Burgeon);       // 烈绽放 (草种子+火)
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Reaction_Quicken);       // 原激化 (雷+草)
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Reaction_Aggravate);     // 超激化 (激化+雷)
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Reaction_Spread);        // 蔓激化 (激化+草)
	
	//月/星反应（设定为与特定武器相关）
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Reaction_LunarBloom);         // 月绽放 (水+草)
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Reaction_LunarElectroCharged);// 月感电 (水+雷)
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Reaction_LunarCrystallize);   // 月结晶 (水/岩)
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Reaction_StellarSuperconduct); // 星超导 (冰+雷)
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Reaction_StellarSwirl);        // 星扩散 (风+元素)
	
	//实体/召唤物
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Entity_DendroCore); // 草原核 / 草种子
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Entity_Moondrift);  // 月笼
	
	//伤害类型
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Damage_Type_Bullet);//子弹伤害
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Damage_Type_Explosion);//投掷物爆炸伤害
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Damage_Type_Skill);//技能伤害
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Damage_Type_Burst);//元素爆发伤害
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Damage_Type_Reaction);//元素反应伤害
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Damage_Type_DoT);//持续伤害
}
