#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"
#include "LSTypes.generated.h"

//武器装备槽位枚举
UENUM(Blueprintable)
enum class ELSWeaponSlot : uint8
{
	None = 0 UMETA(DisplayName = "无/空手"),
	MainWeapon UMETA(DisplayName = "主武器 (slot 1)"),
	SubWeapon UMETA(DisplayName = "副武器 (slot 2)"),
	Throwable UMETA(DisplayName = "元素投掷物 (slot 3)")
};

//场景掩体防护类型
UENUM(Blueprintable)
enum class ELSCoverType : uint8
{
	Full	UMETA(DisplayName = "全掩体（完全遮挡）"),
	Half	UMETA(DisplayName = "半掩体（部分遮挡）"),
	Destructible	UMETA(DisplayName = "可破坏掩体")
};

//射击模式枚举
UENUM(Blueprintable)
enum class ELSFireMode : uint8
{
	SemiAuto UMETA(DisplayName = "半自动（单发）"),
	FullAuto UMETA(DisplayName = "全自动（连发）"),
	Burst UMETA(DisplayName = "三连发（点射）")
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
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_ADS);
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Invincible);
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Dead);
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Downed);
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Reviving);
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Stunned);
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_SuperArmor);
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_AimingThrow);
	
	//元素类型
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Element_Pyro);    //火
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Element_Cryo);    //冰
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Element_Electro); //雷
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Element_Hydro);   //水
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Element_Anemo);   //风
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Element_Geo);     //岩
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Element_Physical);//物理
	
	//元素反应类型
	//1，增幅反应 (黄金三角：火+冰 2.0x 融化爆发)
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Reaction_Melt);          // 融化 (火+冰)
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Reaction_Vaporize);      // 蒸发 (火+水)
	
	//2，剧变反应 (黄金三角：火+雷 超载爆轰击退，冰+雷 碎裂/超导破甲易伤)
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Reaction_Overload);      // 超载 (火+雷)
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Reaction_Superconduct);  // 超导/极性碎裂 (冰+雷)
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Reaction_Shatter);       // 碎裂 (极性破冻)
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Reaction_Freeze);        // 冻结 (水+冰)
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Reaction_ElectroCharged);// 感电 (水+雷)
	
	//伤害类型
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Damage_Type_Bullet);//子弹伤害
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Damage_Type_Explosion);//投掷物爆炸伤害
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Damage_Type_Skill);//技能伤害
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Damage_Type_Burst);//元素爆发伤害
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Damage_Type_Reaction);//元素反应伤害
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Damage_Type_DoT);//持续伤害

	// ─── 武器类别标签 ───
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_Type_Rifle);
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_Type_SMG);
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_Type_Shotgun);
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_Type_Sniper);
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_Type_Launcher);
	
	//敌人AI行为状态
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_AI_State_Idle);
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_AI_State_Alert);
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_AI_State_Combat);
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_AI_State_Fleeing);
	
	//敌人特化兵种标签
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Enemy_Type_Melee);    // 近战杂兵
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Enemy_Type_Ranged);   // 远程射手
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Enemy_Type_Sniper);   // 红外狙击手
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Enemy_Type_Shielder); // 防弹盾兵
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Enemy_Type_Bomber);   // 元素自爆兵
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Enemy_Type_Elite);    // 元素精英怪
	LUMI_SPARK_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Enemy_Type_Boss);     // 多阶段首领
}

//行为树黑板常用key常亮定义
namespace LSBlackboardKeys
{
	const FName TargetActor = FName(TEXT("TargetActor"));
	const FName LastKnownLocation = FName(TEXT("LastKnownLocation"));
	const FName CoverLocation = FName(TEXT("CoverLocation"));
	const FName DistanceToTarget = FName(TEXT("DistanceToTarget"));
	const FName AIState = FName(TEXT("AIState"));
}
