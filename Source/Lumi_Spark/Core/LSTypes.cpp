#include "LSTypes.h"

namespace LSTags
{
	//角色状态
	UE_DEFINE_GAMEPLAY_TAG(TAG_State_Idle,       "State.Idle");
	UE_DEFINE_GAMEPLAY_TAG(TAG_State_Moving,     "State.Moving");
	UE_DEFINE_GAMEPLAY_TAG(TAG_State_Sprinting,  "State.Sprinting");
	UE_DEFINE_GAMEPLAY_TAG(TAG_State_Dashing,    "State.Dashing");
	UE_DEFINE_GAMEPLAY_TAG(TAG_State_Sliding,    "State.Sliding");
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
	UE_DEFINE_GAMEPLAY_TAG(TAG_Element_Hydro,    "Element.Hydro");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Element_Electro,  "Element.Electro");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Element_Cryo,     "Element.Cryo");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Element_Anemo,    "Element.Anemo");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Element_Dendro,   "Element.Dendro");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Element_Geo,      "Element.Geo");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Element_Physical, "Element.Physical");
	
	// 元素反应
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_Vaporize,       "Reaction.Vaporize");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_Melt,           "Reaction.Melt");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_Overload,       "Reaction.Overload");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_Superconduct,   "Reaction.Superconduct");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_Freeze,         "Reaction.Freeze");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_Shatter,        "Reaction.Shatter");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_ElectroCharged, "Reaction.ElectroCharged");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_Swirl,          "Reaction.Swirl");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_Crystallize,    "Reaction.Crystallize");
	
	// 草系反应
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_Burning,        "Reaction.Burning");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_Bloom,          "Reaction.Bloom");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_Hyperbloom,     "Reaction.Hyperbloom");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_Burgeon,        "Reaction.Burgeon");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_Quicken,        "Reaction.Quicken");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_Aggravate,      "Reaction.Aggravate");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_Spread,         "Reaction.Spread");
	
	// 月/星机制
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_LunarBloom,          "Reaction.LunarBloom");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_LunarElectroCharged, "Reaction.LunarElectroCharged");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_LunarCrystallize,    "Reaction.LunarCrystallize");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_StellarConduct,      "Reaction.StellarConduct");
	
	// 衍生实体
	UE_DEFINE_GAMEPLAY_TAG(TAG_Entity_DendroCore, "Entity.DendroCore");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Entity_Moondrift,  "Entity.Moondrift");
	
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
	
	//驱动核心6大流派套装标签
	UE_DEFINE_GAMEPLAY_TAG(TAG_DriveSet_TacticalSwap,       "DriveSet.TacticalSwap");
	UE_DEFINE_GAMEPLAY_TAG(TAG_DriveSet_PrecisionMarksman, "DriveSet.PrecisionMarksman");
	UE_DEFINE_GAMEPLAY_TAG(TAG_DriveSet_ElementalResonance,"DriveSet.ElementalResonance");
	UE_DEFINE_GAMEPLAY_TAG(TAG_DriveSet_MobileAssault,     "DriveSet.MobileAssault");
	UE_DEFINE_GAMEPLAY_TAG(TAG_DriveSet_HeavyBastion,      "DriveSet.HeavyBastion");
	UE_DEFINE_GAMEPLAY_TAG(TAG_DriveSet_TacticalOrdnance,  "DriveSet.TacticalOrdnance");
}
