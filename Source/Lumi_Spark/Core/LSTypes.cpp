#include "LSTypes.h"

namespace LSTags
{
	//角色状态
	UE_DEFINE_GAMEPLAY_TAG(TAG_State_Idle,       "State.Idle");
	UE_DEFINE_GAMEPLAY_TAG(TAG_State_Moving,     "State.Moving");
	UE_DEFINE_GAMEPLAY_TAG(TAG_State_Sprinting,  "State.Sprinting");
	UE_DEFINE_GAMEPLAY_TAG(TAG_State_Dashing,    "State.Dashing");
	UE_DEFINE_GAMEPLAY_TAG(TAG_State_ADS,        "State.ADS");
	UE_DEFINE_GAMEPLAY_TAG(TAG_State_Invincible, "State.Invincible");
	UE_DEFINE_GAMEPLAY_TAG(TAG_State_Dead,       "State.Dead");
	UE_DEFINE_GAMEPLAY_TAG(TAG_State_Downed,     "State.Downed");
	UE_DEFINE_GAMEPLAY_TAG(TAG_State_Reviving,   "State.Reviving");
	UE_DEFINE_GAMEPLAY_TAG(TAG_State_Stunned,    "State.Stunned");
	UE_DEFINE_GAMEPLAY_TAG(TAG_State_SuperArmor, "State.SuperArmor");
	UE_DEFINE_GAMEPLAY_TAG(TAG_State_AimingThrow,"State.AimingThrow");
	
	//元素类型
	UE_DEFINE_GAMEPLAY_TAG(TAG_Element_Pyro,     "Element.Pyro");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Element_Cryo,     "Element.Cryo");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Element_Electro,  "Element.Electro");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Element_Hydro,    "Element.Hydro");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Element_Anemo,    "Element.Anemo");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Element_Geo,      "Element.Geo");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Element_Physical, "Element.Physical");
	
	// 元素反应 (聚焦火/冰/雷黄金三角)
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_Melt,           "Reaction.Melt");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_Vaporize,       "Reaction.Vaporize");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_Overload,       "Reaction.Overload");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_Superconduct,   "Reaction.Superconduct");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_Shatter,        "Reaction.Shatter");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_Freeze,         "Reaction.Freeze");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_ElectroCharged, "Reaction.ElectroCharged");
	
	// 伤害类型
	UE_DEFINE_GAMEPLAY_TAG(TAG_Damage_Type_Bullet,    "Damage.Type.Bullet");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Damage_Type_Explosion, "Damage.Type.Explosion");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Damage_Type_Skill,     "Damage.Type.Skill");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Damage_Type_Burst,     "Damage.Type.Burst");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Damage_Type_Reaction,  "Damage.Type.Reaction");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Damage_Type_DoT,       "Damage.Type.DoT");

	// 武器类别标签
	UE_DEFINE_GAMEPLAY_TAG(TAG_Weapon_Type_Rifle,    "Weapon.Type.Rifle");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Weapon_Type_SMG,      "Weapon.Type.SMG");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Weapon_Type_Shotgun,  "Weapon.Type.Shotgun");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Weapon_Type_Sniper,   "Weapon.Type.Sniper");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Weapon_Type_Launcher, "Weapon.Type.Launcher");
	
	//AI状态标签
	UE_DEFINE_GAMEPLAY_TAG(TAG_AI_State_Idle,      "AI.State.Idle");
	UE_DEFINE_GAMEPLAY_TAG(TAG_AI_State_Alert,     "AI.State.Alert");
	UE_DEFINE_GAMEPLAY_TAG(TAG_AI_State_Combat,    "AI.State.Combat");
	UE_DEFINE_GAMEPLAY_TAG(TAG_AI_State_Fleeing,   "AI.State.Fleeing");
	
	//敌人特化兵种
	UE_DEFINE_GAMEPLAY_TAG(TAG_Enemy_Type_Melee,     "Enemy.Type.Melee");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Enemy_Type_Ranged,    "Enemy.Type.Ranged");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Enemy_Type_Sniper,    "Enemy.Type.Sniper");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Enemy_Type_Shielder,  "Enemy.Type.Shielder");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Enemy_Type_Bomber,    "Enemy.Type.Bomber");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Enemy_Type_Elite,     "Enemy.Type.Elite");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Enemy_Type_Boss,      "Enemy.Type.Boss");
}
