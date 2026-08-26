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
	
	// ─── 元素类型 ───
	UE_DEFINE_GAMEPLAY_TAG(TAG_Element_Pyro,     "Element.Pyro");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Element_Hydro,    "Element.Hydro");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Element_Electro,  "Element.Electro");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Element_Cryo,     "Element.Cryo");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Element_Anemo,    "Element.Anemo");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Element_Dendro,   "Element.Dendro");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Element_Geo,      "Element.Geo");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Element_Physical, "Element.Physical");
	
	// ─── 元素反应 ───
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_Vaporize,       "Reaction.Vaporize");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_Melt,           "Reaction.Melt");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_Overload,       "Reaction.Overload");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_Superconduct,   "Reaction.Superconduct");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_Freeze,         "Reaction.Freeze");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_Shatter,        "Reaction.Shatter");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_ElectroCharged, "Reaction.ElectroCharged");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_Swirl,          "Reaction.Swirl");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_Crystallize,    "Reaction.Crystallize");
	
	// ─── 草系反应 ───
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_Burning,        "Reaction.Burning");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_Bloom,          "Reaction.Bloom");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_Hyperbloom,     "Reaction.Hyperbloom");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_Burgeon,        "Reaction.Burgeon");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_Quicken,        "Reaction.Quicken");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_Aggravate,      "Reaction.Aggravate");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_Spread,         "Reaction.Spread");
	
	// ─── 月/星机制 ───
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_LunarBloom,          "Reaction.LunarBloom");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_LunarElectroCharged, "Reaction.LunarElectroCharged");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_LunarCrystallize,    "Reaction.LunarCrystallize");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_StellarConduct,      "Reaction.StellarConduct");
	
	// ─── 衍生实体 ───
	UE_DEFINE_GAMEPLAY_TAG(TAG_Entity_DendroCore, "Entity.DendroCore");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Entity_Moondrift,  "Entity.Moondrift");
	
	// ─── 伤害类型 ───
	UE_DEFINE_GAMEPLAY_TAG(TAG_Damage_Type_Bullet,    "Damage.Type.Bullet");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Damage_Type_Explosion, "Damage.Type.Explosion");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Damage_Type_Skill,     "Damage.Type.Skill");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Damage_Type_Burst,     "Damage.Type.Burst");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Damage_Type_Reaction,  "Damage.Type.Reaction");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Damage_Type_DoT,       "Damage.Type.DoT");
}
