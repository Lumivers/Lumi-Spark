# Lumi-Spark: UE5 元素 FPS 游戏设计文档

> **定位：** 原神核心玩法（元素反应、4人队伍切换、技能CD体系）× FPS 射击战斗  
> **引擎：** Unreal Engine 5.4+  
> **目标平台：** Windows PC（优先）  
> **开发模式：** 纯 PvE，后续再考虑 PvP  

---

## 目录

1. [游戏概述](#1-游戏概述)
2. [整体架构](#2-整体架构)
3. [角色 3C 系统（Camera / Character / Control）](#3-角色-3c-系统camera--character--control)
4. [武器系统](#4-武器系统)
5. [核心战斗系统](#5-核心战斗系统)
6. [元素反应系统](#6-元素反应系统)
7. [队伍切换系统](#7-队伍切换系统)
8. [资源与状态系统](#8-资源与状态系统)
9. [敌人 AI 系统](#9-敌人-ai-系统)
10. [UI / HUD 系统](#10-ui--hud-系统)
11. [场景与关卡管理](#11-场景与关卡管理)
12. [背包与装备系统](#12-背包与装备系统)
13. [性能优化方案](#13-性能优化方案)
14. [网络架构预留（PvP 扩展）](#14-网络架构预留pvp-扩展)
15. [项目结构与文件清单](#15-项目结构与文件清单)
16. [开发时间线](#16-开发时间线)
17. [验证方案](#17-验证方案)

---

## 1. 游戏概述

### 1.1 核心概念

**Lumi-Spark** 是一款将原神的元素反应战斗体系与 FPS 射击玩法深度融合的 PvE 游戏。

- **攻击方式：** 枪械射击与元素投掷结合，枪支自带元素属性（火步枪、冰狙击、雷冲锋等），射击与投掷直接施加元素附着。
- **2枪 + 1投掷物配装架构：**
  - **主武器（Slot 1）：** 核心输出枪械（突击步枪 / 狙击枪 / 散弹枪等）。
  - **副武器（Slot 2）：** 战术辅助/近身枪械（冲锋枪 / 手枪 / 辅助削抗枪等）。
  - **元素投掷物（Slot 3 / G键）：** **水、火、冰、雷四大元素手雷**，落地产生大范围元素附着领域与伤害，是制造大范围群体元素反应（如大范围冻结、群体蒸发、超载清场）的战术核心。
- **双角色切换机制：** **2 人双角色小队（Dual-Character）** 即时切换（Tab 键一键对调）。两位角色拥有独立的武器配装（即全队共 4 把枪 + 2 种元素投掷物）、独立天赋与 E/Q 技能组。
- **视角系统：** 以**第一人称沉浸式射击**为主，支持第三人称（TPS）与过肩瞄准（ADS）自由切换。
- **战斗节奏：** 快节奏射击 + 抛物线丢雷挂元素 + 原神技能/大招 CD 体系 + 极速切枪/切人反应 Combo。
- **世界结构：** 枢纽大厅（Hub）+ 关卡入口，纯 PvE 副本制。

### 1.2 核心玩法循环

```
配置双人队伍（各带2枪+1元素雷） → 进入关卡 → 投掷水/火/冰/雷手雷挂范围元素 → 切枪射击打出群体反应 → 技能爆发 + 适时切人连携 → 击败敌人/Boss → 结算奖励强化
```

### 1.3 与原神的关键差异

| 维度 | 原神 | Lumi-Spark |
|------|------|---------------|
| 攻击方式 | 近战挥砍 / 弓箭 | **枪械射击 + 抛物线元素投掷物** |
| 队伍规模 | 4 人即时切换 | **2 人双角色小队**（降低第一人称频繁换人眩晕感） |
| 单人装备槽位 | 仅限 1 把固定武器 | **2 把枪械（主+副） + 1 元素投掷物（水/火/冰/雷）** |
| 元素范围附着 | 依赖特定角色 E/Q 技能 | **水/火/冰/雷元素手雷随时投掷制造元素领域** |
| 元素反应触发 | 频繁切 4 个人释放技能 | **自身快速切枪/丢雷**挂元素 + **双角色技能连携** |
| 视角 | 固定第三人称 | **第一人称为主**，支持 TPS 与过肩瞄准 |
| 打击感来源 | 顿帧 + 受击动画 | 后坐力 + 命中反馈(HitMarker) + 抛物线爆炸击退 |

---

## 2. 整体架构

### 2.1 模块依赖关系

```
┌──────────────────────────────────────────────────────────┐
│                    Application Layer                      │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐  │
│  │   Core   │  │   UI     │  │  Level   │  │  Audio   │  │
│  │ Runtime  │  │  System  │  │ Manager  │  │  System  │  │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘  │
├───────┼──────────────┼────────────┼──────────────┼────────┤
│                    Gameplay Layer                          │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐  │
│  │ Character│  │  Combat  │  │ Element  │  │  Enemy   │  │
│  │   3C     │  │  System  │  │ Reaction │  │    AI    │  │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘  │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐  │
│  │  Weapon  │  │ Resource │  │Inventory │  │  Quest   │  │
│  │  System  │  │  System  │  │  System  │  │  System  │  │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘  │
├───────┼──────────────┼────────────┼──────────────┼────────┤
│                    Foundation Layer                        │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐  │
│  │ Object   │  │  Data    │  │  Event   │  │ Gameplay │  │
│  │  Pool    │  │  Asset   │  │   Bus    │  │   Tags   │  │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘  │
└──────────────────────────────────────────────────────────┘
```

**分层原则：**
- **Foundation Layer** — 不依赖任何游戏逻辑的基础设施（对象池、数据资产、事件总线、GameplayTags）
- **Gameplay Layer** — 核心玩法模块，模块间通过 Delegate + Interface 解耦
- **Application Layer** — 上层业务（UI 绑定、关卡管理、音频调度）

### 2.2 项目目录结构

```
Lumi-Spark/
├── Source/
│   └── Lumi-Spark/
│       ├── Lumi-Spark.Build.cs
│       ├── Lumi-Spark.h / .cpp
│       │
│       ├── Core/                              # 核心框架
│       │   ├── ESGameInstance.h/.cpp          # GameInstance 全局单例
│       │   ├── ESGameMode.h/.cpp             # GameMode（Hub / Combat 两种）
│       │   ├── ESGameState.h/.cpp            # GameState 全局共享状态
│       │   ├── ESPlayerController.h/.cpp     # 输入处理 + 视角切换
│       │   ├── ESPlayerState.h/.cpp          # 玩家持久状态
│       │   ├── ESEventBus.h/.cpp             # 全局事件总线（解耦模块通信）
│       │   └── ESTypes.h                     # 全局类型定义 + 枚举
│       │
│       ├── Character/                         # 角色 3C 系统
│       │   ├── ESCharacterBase.h/.cpp        # 角色基类
│       │   ├── ESPlayerCharacter.h/.cpp      # 玩家可控角色
│       │   ├── ESCameraComponent.h/.cpp      # 三模式摄像机（FPS/TPS/肩射）
│       │   ├── ESMovementComponent.h/.cpp    # 增强移动组件
│       │   ├── TeamSwitchComponent.h/.cpp    # 队伍切换组件
│       │   └── CharacterDataAsset.h/.cpp     # 角色配置数据资产
│       │
│       ├── Weapon/                            # 武器系统
│       │   ├── ESWeaponBase.h/.cpp           # 武器基类
│       │   ├── ESWeaponComponent.h/.cpp      # 武器管理组件
│       │   ├── ESProjectile.h/.cpp           # 子弹/投射物基类
│       │   ├── ESHitscanTrace.h/.cpp         # 射线检测（即时命中）
│       │   ├── WeaponDataAsset.h/.cpp        # 武器配置数据资产
│       │   ├── RecoilComponent.h/.cpp        # 后坐力系统
│       │   └── Weapons/                      # 具体武器类型
│       │       ├── ESRifle.h/.cpp            # 步枪
│       │       ├── ESShotgun.h/.cpp          # 霰弹枪
│       │       ├── ESSniperRifle.h/.cpp      # 狙击枪
│       │       ├── ESSMG.h/.cpp             # 冲锋枪
│       │       └── ESLauncher.h/.cpp         # 榴弹发射器
│       │
│       ├── Combat/                            # 战斗系统
│       │   ├── CombatComponent.h/.cpp        # 战斗主组件
│       │   ├── DamageCalculator.h/.cpp       # 伤害公式（纯函数）
│       │   ├── DamageTypes.h                 # 伤害类型定义
│       │   ├── HitFeedbackComponent.h/.cpp   # 命中反馈（音效/粒子/HitMarker）
│       │   ├── SkillComponent.h/.cpp         # 主动技能组件（E/Q）
│       │   └── SkillDataAsset.h/.cpp         # 技能配置数据资产
│       │
│       ├── Element/                           # 元素反应系统
│       │   ├── ElementTypes.h                # 元素枚举 + GameplayTags
│       │   ├── ElementComponent.h/.cpp       # 元素附着组件（含ICD）
│       │   ├── ElementReactionManager.h/.cpp # 反应规则引擎
│       │   └── ReactionEffects/              # 反应特效
│       │       ├── VaporizeEffect.h/.cpp     # 蒸发
│       │       ├── OverloadEffect.h/.cpp     # 超载
│       │       ├── FreezeEffect.h/.cpp       # 冻结
│       │       ├── SuperconductEffect.h/.cpp # 超导
│       │       ├── ElectroChargedEffect.h/.cpp # 感电
│       │       ├── SwirlEffect.h/.cpp        # 扩散
│       │       └── CrystallizeEffect.h/.cpp  # 结晶
│       │
│       ├── Resource/                          # 资源组件
│       │   ├── HealthComponent.h/.cpp        # 生命值
│       │   ├── ShieldComponent.h/.cpp        # 护盾
│       │   ├── StaminaComponent.h/.cpp       # 体力
│       │   ├── EnergyComponent.h/.cpp        # 元素能量
│       │   └── ResourceTypes.h               # 资源类型定义
│       │
│       ├── AI/                                # 敌人 AI
│       │   ├── ESAIController.h/.cpp         # AI 控制器基类
│       │   ├── EnemyBase.h/.cpp              # 敌人基类
│       │   ├── EnemyDataAsset.h/.cpp         # 敌人配置数据
│       │   ├── AIPerceptionSetup.h/.cpp      # 感知系统配置
│       │   ├── Enemies/                      # 具体敌人类型
│       │   │   ├── EnemyMelee.h/.cpp         # 近战兵
│       │   │   ├── EnemyRanged.h/.cpp        # 远程射手
│       │   │   ├── EnemySniper.h/.cpp        # 狙击手
│       │   │   ├── EnemyShielder.h/.cpp      # 盾兵
│       │   │   └── EnemyBoss.h/.cpp          # Boss 基类
│       │   └── BehaviorTree/                 # 行为树资产
│       │       ├── BTTask_FindCover.h/.cpp   # 寻找掩体
│       │       ├── BTTask_FlankPlayer.h/.cpp # 侧翼包抄
│       │       ├── BTTask_ElementalAttack.h/.cpp # 元素攻击
│       │       └── BTDecorator_CheckElement.h/.cpp # 元素状态检查
│       │
│       ├── UI/                                # UI 系统
│       │   ├── ESHUD.h/.cpp                  # HUD 主控
│       │   ├── CrosshairWidget.h/.cpp        # 准星系统
│       │   ├── TeamPortraitBar.h/.cpp        # 队伍头像栏
│       │   ├── HealthBarWidget.h/.cpp        # 血条（敌人/队友）
│       │   ├── SkillCooldownWidget.h/.cpp    # 技能 CD
│       │   ├── AmmoWidget.h/.cpp             # 弹药显示
│       │   ├── StaminaBarWidget.h/.cpp       # 体力条
│       │   ├── DamageNumberWidget.h/.cpp     # 飘字伤害
│       │   ├── HitMarkerWidget.h/.cpp        # 命中标记
│       │   ├── ElementIndicatorWidget.h/.cpp # 元素附着指示器
│       │   └── KillFeedWidget.h/.cpp         # 击杀信息流
│       │
│       ├── Level/                             # 场景管理
│       │   ├── ESLevelManager.h/.cpp         # 关卡管理器
│       │   ├── ESHubGameMode.h/.cpp          # 枢纽大厅 GameMode
│       │   ├── ESCombatGameMode.h/.cpp       # 战斗关卡 GameMode
│       │   ├── LevelPortal.h/.cpp            # 关卡入口传送门
│       │   ├── SpawnManager.h/.cpp           # 敌人刷新管理
│       │   ├── WaveSystem.h/.cpp             # 波次系统
│       │   └── LevelDataAsset.h/.cpp         # 关卡配置数据
│       │
│       ├── Inventory/                         # 背包系统
│       │   ├── InventoryComponent.h/.cpp     # 背包组件
│       │   ├── InventoryItem.h/.cpp          # 物品基类
│       │   ├── InventoryTypes.h              # 类型定义
│       │   └── ItemDataAsset.h/.cpp          # 物品配置数据
│       │
│       ├── Quest/                             # 任务系统
│       │   ├── QuestManager.h/.cpp           # 任务管理器
│       │   ├── QuestData.h/.cpp              # 任务数据
│       │   └── QuestTypes.h                  # 类型定义
│       │
│       └── Optimization/                      # 性能优化
│           ├── ObjectPoolSubsystem.h/.cpp    # 对象池子系统
│           ├── ProjectilePool.h/.cpp         # 子弹对象池
│           ├── LODManager.h/.cpp             # LOD 管理
│           └── CullingOptimizer.h/.cpp       # 裁剪优化
│
├── Config/
│   ├── DefaultEngine.ini
│   ├── DefaultGame.ini
│   ├── DefaultInput.ini                       # Enhanced Input 配置
│   └── DefaultGameplayTags.ini                # GameplayTags 注册
│
├── Content/
│   ├── Characters/                            # 角色蓝图 + 动画
│   │   ├── BP_Character_Pyro/
│   │   ├── BP_Character_Hydro/
│   │   ├── BP_Character_Electro/
│   │   └── BP_Character_Cryo/
│   ├── Weapons/                               # 武器蓝图 + 模型
│   │   ├── BP_Rifle_Pyro/
│   │   ├── BP_Sniper_Cryo/
│   │   ├── BP_Shotgun_Electro/
│   │   └── BP_SMG_Hydro/
│   ├── Enemies/                               # 敌人蓝图 + AI
│   ├── FX/                                    # Niagara 特效
│   │   ├── NS_MuzzleFlash/
│   │   ├── NS_BulletImpact/
│   │   ├── NS_ElementalTrail/
│   │   └── NS_Reaction_*/
│   ├── Maps/                                  # 关卡
│   │   ├── Map_Hub/                           # 枢纽大厅
│   │   ├── Map_Arena_01/                      # 战斗场景
│   │   └── Map_Boss_01/                       # Boss 关卡
│   ├── UI/                                    # UMG Widget 蓝图
│   ├── DataAssets/                            # 数据资产
│   │   ├── DA_Characters/
│   │   ├── DA_Weapons/
│   │   ├── DA_Enemies/
│   │   ├── DA_Skills/
│   │   └── DA_Levels/
│   ├── Input/                                 # Enhanced Input 资产
│   │   ├── IA_Move.uasset
│   │   ├── IA_Look.uasset
│   │   ├── IA_Fire.uasset
│   │   ├── IA_ADS.uasset
│   │   ├── IA_Reload.uasset
│   │   ├── IA_Skill.uasset
│   │   ├── IA_Burst.uasset
│   │   ├── IA_Switch_1~4.uasset
│   │   ├── IA_Dash.uasset
│   │   ├── IA_ToggleView.uasset
│   │   └── IMC_Default.uasset                # Input Mapping Context
│   └── Audio/                                 # 音效
│       ├── SFX_Weapons/
│       ├── SFX_Elements/
│       └── SFX_Reactions/
│
└── Plugins/                                   # 第三方插件
```

### 2.3 Gameplay Tags 命名规范

GameplayTags 是整个项目的状态标签统一管理方案，所有模块通过 Tags 而非硬编码枚举来标识状态，便于扩展和跨模块查询。

```cpp
// ═══════════ 角色状态 ═══════════
State.Idle
State.Moving
State.Sprinting
State.Dashing
State.InAir
State.Firing              // 正在射击
State.Reloading           // 换弹中
State.ADS                 // 瞄准镜/肩射模式
State.UsingSkill          // 释放 E 技能
State.UsingBurst          // 释放 Q 大招
State.Switching           // 正在切人
State.HitStun             // 受击硬直
State.Invincible          // 无敌帧
State.Dead

// ═══════════ 元素类型 ═══════════
Element.Pyro              // 火
Element.Hydro             // 水
Element.Electro           // 雷
Element.Cryo              // 冰
Element.Anemo             // 风
Element.Dendro            // 草
Element.Geo               // 岩
Element.Physical          // 物理（无元素）

// ═══════════ 元素反应 ═══════════
Reaction.Vaporize         // 蒸发（火+水）
Reaction.Melt             // 融化（火+冰）
Reaction.Overload         // 超载（火+雷）
Reaction.Superconduct     // 超导（冰+雷）
Reaction.Freeze           // 冻结（水+冰）
Reaction.ElectroCharged   // 感电（水+雷）
Reaction.Swirl            // 扩散（风+任意）
Reaction.Crystallize      // 结晶（岩+任意）
Reaction.Bloom            // 绽放（水+草）
Reaction.Quicken          // 激化（雷+草）
Reaction.Burning          // 燃烧（火+草）

// ═══════════ 武器类型 ═══════════
Weapon.Rifle              // 步枪
Weapon.Shotgun            // 霰弹枪
Weapon.SniperRifle        // 狙击枪
Weapon.SMG                // 冲锋枪
Weapon.Launcher           // 榴弹发射器

// ═══════════ 敌人状态 ═══════════
Enemy.State.Idle
Enemy.State.Patrol
Enemy.State.Alert
Enemy.State.Chase
Enemy.State.Attack
Enemy.State.TakeCover
Enemy.State.Flanking
Enemy.State.Stunned
Enemy.State.Frozen
Enemy.State.Dead

// ═══════════ 伤害类型标签 ═══════════
Damage.Type.Bullet        // 子弹伤害
Damage.Type.Explosion     // 爆炸伤害
Damage.Type.ElementSkill  // 技能伤害
Damage.Type.ElementBurst  // 大招伤害
Damage.Type.Reaction      // 反应伤害
Damage.Type.DoT           // 持续伤害

// ═══════════ 关卡 / 区域 ═══════════
Level.Hub                 // 枢纽大厅
Level.Arena               // 竞技场
Level.Dungeon             // 副本
Level.Boss                // Boss 关

// ═══════════ 统计属性 ═══════════
Stat.ATK                  // 攻击力
Stat.DEF                  // 防御力
Stat.HP                   // 生命值
Stat.CritRate             // 暴击率
Stat.CritDMG              // 暴击伤害
Stat.ElementalMastery     // 元素精通
Stat.EnergyRecharge       // 元素充能效率
```

### 2.4 事件总线（模块解耦核心）

所有模块之间的通信通过全局事件总线解耦，避免直接引用和循环依赖：

```cpp
// ESEventBus.h — 基于 UE5 的 UGameInstanceSubsystem 实现
UCLASS()
class UESEventBus : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    // ═══ 战斗事件 ═══
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnDamageDealt,
        AActor*, Attacker, AActor*, Target, float, Damage, FGameplayTag, Element);
    UPROPERTY() FOnDamageDealt OnDamageDealt;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEnemyKilled,
        AActor*, Enemy, AActor*, Killer);
    UPROPERTY() FOnEnemyKilled OnEnemyKilled;

    // ═══ 元素事件 ═══
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnElementApplied,
        AActor*, Target, FGameplayTag, Element, float, Gauge);
    UPROPERTY() FOnElementApplied OnElementApplied;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnElementReaction,
        AActor*, Target, FGameplayTag, Reaction, float, Damage);
    UPROPERTY() FOnElementReaction OnElementReaction;

    // ═══ 角色事件 ═══
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCharacterSwitched,
        int32, OldIndex, int32, NewIndex);
    UPROPERTY() FOnCharacterSwitched OnCharacterSwitched;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterDeath,
        AESCharacterBase*, Character);
    UPROPERTY() FOnCharacterDeath OnCharacterDeath;

    // ═══ 关卡事件 ═══
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWaveStart, int32, WaveIndex);
    UPROPERTY() FOnWaveStart OnWaveStart;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWaveClear);
    UPROPERTY() FOnWaveClear OnWaveClear;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLevelComplete, FGameplayTag, LevelID);
    UPROPERTY() FOnLevelComplete OnLevelComplete;

    // ═══ 辅助方法 ═══
    static UESEventBus* Get(const UObject* WorldContext);
};
```

**使用范例（任何模块都可以发布/订阅事件）：**

```cpp
// 发布事件（在 CombatComponent 中）
UESEventBus::Get(this)->OnDamageDealt.Broadcast(Attacker, Target, FinalDamage, Element);

// 订阅事件（在 UI HUD 中）
UESEventBus::Get(this)->OnDamageDealt.AddDynamic(this, &UESHUD::OnDamageDealt);

// 订阅事件（在 QuestManager 中 — 同一事件多处监听）
UESEventBus::Get(this)->OnEnemyKilled.AddDynamic(this, &UQuestManager::OnEnemyKilled);
```

---

## 3. 角色 3C 系统（Camera / Character / Control）

3C 系统是 FPS 游戏的根基。本项目的核心挑战在于：**同时支持 FPS / TPS / 过肩瞄准三种视角**，并在切人时保持视角连贯。

### 3.1 摄像机系统（Camera）

三模式摄像机是本项目最核心的 3C 差异化设计。三种模式共用一个 `UESCameraComponent`，通过插值平滑切换：

```cpp
// ESCameraComponent.h
UENUM(BlueprintType)
enum class ECameraMode : uint8
{
    FirstPerson,    // 第一人称：摄像机在角色眼部位置
    ThirdPerson,    // 第三人称：摄像机在角色身后偏上
    OverShoulder    // 过肩瞄准：摄像机偏移到右肩上方，准星居中
};

UCLASS()
class UESCameraComponent : public UCameraComponent
{
    GENERATED_BODY()
public:
    // ═══ 当前模式 ═══
    UPROPERTY(BlueprintReadOnly) ECameraMode CurrentMode = ECameraMode::FirstPerson;

    // ═══ 模式配置参数 ═══
    // 第一人称
    UPROPERTY(EditDefaultsOnly, Category="Camera|FPS")
    FVector FPSOffset = FVector(0, 0, 70);           // 相对角色根的偏移（眼部高度）
    UPROPERTY(EditDefaultsOnly, Category="Camera|FPS")
    float FPSFov = 90.f;

    // 第三人称
    UPROPERTY(EditDefaultsOnly, Category="Camera|TPS")
    float TPSArmLength = 300.f;                       // 弹簧臂长度
    UPROPERTY(EditDefaultsOnly, Category="Camera|TPS")
    FVector TPSSocketOffset = FVector(0, 60, 80);     // 弹簧臂终点偏移
    UPROPERTY(EditDefaultsOnly, Category="Camera|TPS")
    float TPSFov = 80.f;

    // 过肩瞄准（ADS 时自动切入）
    UPROPERTY(EditDefaultsOnly, Category="Camera|Shoulder")
    FVector ShoulderOffset = FVector(0, 70, 60);      // 右肩偏移
    UPROPERTY(EditDefaultsOnly, Category="Camera|Shoulder")
    float ShoulderFov = 65.f;                         // 收窄 FOV 增强瞄准感
    UPROPERTY(EditDefaultsOnly, Category="Camera|Shoulder")
    float ADSZoomFov = 45.f;                          // 开镜后 FOV（狙击枪更低）

    // ═══ 过渡参数 ═══
    UPROPERTY(EditDefaultsOnly, Category="Camera|Transition")
    float TransitionSpeed = 10.f;                     // 视角切换插值速率

    // ═══ 核心方法 ═══
    UFUNCTION(BlueprintCallable)
    void SetCameraMode(ECameraMode NewMode);

    UFUNCTION(BlueprintCallable)
    void ToggleCameraMode();  // V 键循环切换

    UFUNCTION(BlueprintCallable)
    void EnterADS();          // 右键按下 → 进入过肩/开镜
    UFUNCTION(BlueprintCallable)
    void ExitADS();           // 右键松开 → 退出

    // Tick 中执行平滑插值
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
                               FActorComponentTickFunction* ThisTickFunction) override;

private:
    // ═══ 插值状态 ═══
    FVector TargetOffset;
    float TargetFov;
    float TargetArmLength;
    bool bIsInADS = false;

    // ═══ 弹簧臂碰撞检测 ═══（TPS 模式防穿墙）
    UPROPERTY(VisibleAnywhere)
    USpringArmComponent* SpringArm;

    void UpdateCameraInterpolation(float DeltaTime);

    // ═══ 模式切换时的 Mesh 可见性控制 ═══
    void UpdateMeshVisibility(ECameraMode Mode);
    // FPS 模式：隐藏角色全身 Mesh，只显示手臂 Mesh（FP Arms）
    // TPS / Shoulder 模式：显示角色全身 Mesh，隐藏手臂 Mesh
};
```

**摄像机碰撞处理（TPS 模式防穿墙）：**

```cpp
void UESCameraComponent::UpdateCameraInterpolation(float DeltaTime)
{
    // 1. 目标位置插值
    FVector CurrentOffset = FMath::VInterpTo(GetRelativeLocation(), TargetOffset,
                                             DeltaTime, TransitionSpeed);
    SetRelativeLocation(CurrentOffset);

    // 2. FOV 插值
    float CurrentFov = FMath::FInterpTo(FieldOfView, TargetFov, DeltaTime, TransitionSpeed);
    SetFieldOfView(CurrentFov);

    // 3. TPS 模式弹簧臂碰撞检测
    if (CurrentMode == ECameraMode::ThirdPerson || CurrentMode == ECameraMode::OverShoulder)
    {
        FHitResult Hit;
        FVector Start = GetOwner()->GetActorLocation() + FVector(0, 0, 70);
        FVector End = Start - GetForwardVector() * TargetArmLength;

        if (GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity,
            ECC_Camera, FCollisionShape::MakeSphere(12.f)))
        {
            // 碰到墙壁 → 缩短弹簧臂 → 防止穿墙
            float ClampedLength = FMath::Max((Hit.Location - Start).Size() - 12.f, 0.f);
            SpringArm->TargetArmLength = ClampedLength;
        }
        else
        {
            SpringArm->TargetArmLength = FMath::FInterpTo(
                SpringArm->TargetArmLength, TargetArmLength, DeltaTime, TransitionSpeed);
        }
    }
}
```

**Mesh 可见性切换（FPS/TPS 模型差异）：**

```cpp
void UESCameraComponent::UpdateMeshVisibility(ECameraMode Mode)
{
    AESPlayerCharacter* Owner = Cast<AESPlayerCharacter>(GetOwner());
    if (!Owner) return;

    if (Mode == ECameraMode::FirstPerson)
    {
        // FPS：隐藏全身模型，显示第一人称手臂
        Owner->GetMesh()->SetOwnerNoSee(true);       // 全身 Mesh 本地不可见
        Owner->GetMesh()->SetCastShadow(true);        // 但仍投射阴影（看到自己的影子）
        Owner->FPArmsMesh->SetVisibility(true);       // 第一人称手臂可见
    }
    else
    {
        // TPS / Shoulder：显示全身模型，隐藏手臂
        Owner->GetMesh()->SetOwnerNoSee(false);
        Owner->FPArmsMesh->SetVisibility(false);
    }
}
```

### 3.2 角色移动组件（Character）

继承 `UCharacterMovementComponent`，扩展 FPS 特有的移动机制：

```cpp
// ESMovementComponent.h
UCLASS()
class UESMovementComponent : public UCharacterMovementComponent
{
    GENERATED_BODY()
public:
    // ═══ 移动参数 ═══
    UPROPERTY(EditDefaultsOnly, Category="Movement")
    float WalkSpeed = 600.f;

    UPROPERTY(EditDefaultsOnly, Category="Movement")
    float SprintSpeed = 900.f;

    UPROPERTY(EditDefaultsOnly, Category="Movement")
    float ADSSpeed = 300.f;              // 瞄准时移速大幅降低

    UPROPERTY(EditDefaultsOnly, Category="Movement")
    float CrouchSpeed = 250.f;

    UPROPERTY(EditDefaultsOnly, Category="Movement")
    float AirControl = 0.3f;             // 空中控制力

    // ═══ 冲刺（消耗体力）═══
    UFUNCTION(BlueprintCallable)
    void StartSprint();
    UFUNCTION(BlueprintCallable)
    void StopSprint();

    // ═══ 闪避（消耗体力 + 无敌帧）═══
    UFUNCTION(BlueprintCallable)
    void Dash();

    UPROPERTY(EditDefaultsOnly, Category="Dash")
    float DashDistance = 500.f;
    UPROPERTY(EditDefaultsOnly, Category="Dash")
    float DashDuration = 0.2f;           // 闪避持续时间
    UPROPERTY(EditDefaultsOnly, Category="Dash")
    float DashCooldown = 0.8f;           // 闪避 CD
    UPROPERTY(EditDefaultsOnly, Category="Dash")
    float DashStaminaCost = 18.f;        // 体力消耗
    UPROPERTY(EditDefaultsOnly, Category="Dash")
    float DashIFrameDuration = 0.15f;    // 无敌帧持续时间

    // ═══ 滑铲（冲刺 + 蹲下触发）═══
    UFUNCTION(BlueprintCallable)
    void StartSlide();

    UPROPERTY(EditDefaultsOnly, Category="Slide")
    float SlideSpeed = 1000.f;
    UPROPERTY(EditDefaultsOnly, Category="Slide")
    float SlideDuration = 0.6f;
    UPROPERTY(EditDefaultsOnly, Category="Slide")
    float SlideDeceleration = 800.f;

    // ═══ 蹬墙跳（可选，增加纵向机动性）═══
    UFUNCTION(BlueprintCallable)
    void WallJump();

    UPROPERTY(EditDefaultsOnly, Category="WallJump")
    float WallJumpForce = 600.f;
    UPROPERTY(EditDefaultsOnly, Category="WallJump")
    float WallCheckDistance = 50.f;

    // ═══ 状态查询 ═══
    UPROPERTY(BlueprintReadOnly) bool bIsSprinting = false;
    UPROPERTY(BlueprintReadOnly) bool bIsDashing = false;
    UPROPERTY(BlueprintReadOnly) bool bIsSliding = false;
    UPROPERTY(BlueprintReadOnly) bool bIsInADS = false;

private:
    FTimerHandle DashTimerHandle;
    FTimerHandle DashCooldownHandle;
    bool bDashOnCooldown = false;

    // 闪避实现：沿输入方向施加 RootMotion 位移
    void PerformDash(FVector Direction);
    void EndDash();
    void ResetDashCooldown();

    // 速度修正：根据当前状态动态调整 MaxWalkSpeed
    virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation,
                                    const FVector& OldVelocity) override;
};
```

**移动速度状态机：**

```cpp
void UESMovementComponent::OnMovementUpdated(float DeltaSeconds,
    const FVector& OldLocation, const FVector& OldVelocity)
{
    Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);

    // 优先级：滑铲 > 闪避 > 冲刺 > ADS > 蹲下 > 行走
    if (bIsSliding)
    {
        MaxWalkSpeed = FMath::FInterpTo(MaxWalkSpeed, 0.f, DeltaSeconds,
                                        SlideDeceleration / SlideSpeed);
    }
    else if (bIsDashing)
    {
        // Dash 由 RootMotion 控制，不修改 MaxWalkSpeed
    }
    else if (bIsInADS)
    {
        MaxWalkSpeed = ADSSpeed;
    }
    else if (bIsSprinting)
    {
        // 检查体力
        if (UStaminaComponent* Stamina = GetOwner()->FindComponentByClass<UStaminaComponent>())
        {
            if (Stamina->ConsumeStamina(SprintStaminaCost * DeltaSeconds))
                MaxWalkSpeed = SprintSpeed;
            else
                StopSprint();  // 体力耗尽自动停止冲刺
        }
    }
    else if (IsCrouching())
    {
        MaxWalkSpeed = CrouchSpeed;
    }
    else
    {
        MaxWalkSpeed = WalkSpeed;
    }
}
```

### 3.3 输入系统（Control）

使用 UE5 Enhanced Input System，支持动态切换 Input Mapping Context（战斗模式 / UI 模式 / 载具模式）：

```cpp
// ESPlayerController.h
UCLASS()
class AESPlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    // ═══ Input Mapping Context ═══
    UPROPERTY(EditDefaultsOnly, Category="Input")
    UInputMappingContext* DefaultMappingContext;    // 战斗默认

    UPROPERTY(EditDefaultsOnly, Category="Input")
    UInputMappingContext* UIModeMappingContext;     // UI 菜单模式

    // ═══ Input Actions ═══
    UPROPERTY(EditDefaultsOnly, Category="Input") UInputAction* IA_Move;
    UPROPERTY(EditDefaultsOnly, Category="Input") UInputAction* IA_Look;
    UPROPERTY(EditDefaultsOnly, Category="Input") UInputAction* IA_Fire;
    UPROPERTY(EditDefaultsOnly, Category="Input") UInputAction* IA_ADS;         // 右键瞄准
    UPROPERTY(EditDefaultsOnly, Category="Input") UInputAction* IA_Reload;
    UPROPERTY(EditDefaultsOnly, Category="Input") UInputAction* IA_Skill;       // E 技能
    UPROPERTY(EditDefaultsOnly, Category="Input") UInputAction* IA_Burst;       // Q 大招
    UPROPERTY(EditDefaultsOnly, Category="Input") UInputAction* IA_Sprint;
    UPROPERTY(EditDefaultsOnly, Category="Input") UInputAction* IA_Dash;
    UPROPERTY(EditDefaultsOnly, Category="Input") UInputAction* IA_Crouch;
    UPROPERTY(EditDefaultsOnly, Category="Input") UInputAction* IA_Jump;
    UPROPERTY(EditDefaultsOnly, Category="Input") UInputAction* IA_SwitchWeapon1;    // 1键：主武器
    UPROPERTY(EditDefaultsOnly, Category="Input") UInputAction* IA_SwitchWeapon2;    // 2键：副武器
    UPROPERTY(EditDefaultsOnly, Category="Input") UInputAction* IA_QuickSwitchWeapon;// 滚轮：主副武器快速轮换
    UPROPERTY(EditDefaultsOnly, Category="Input") UInputAction* IA_ThrowGrenade;     // G键 / 3键：水火冰雷元素手雷（按住预览，松开投掷）
    UPROPERTY(EditDefaultsOnly, Category="Input") UInputAction* IA_SwitchCharacter;   // Tab键 / C键：双角色即时切换
    UPROPERTY(EditDefaultsOnly, Category="Input") UInputAction* IA_Interact;          // F 交互

protected:
    virtual void SetupInputComponent() override;
    virtual void BeginPlay() override;

    // ═══ 输入回调 ═══
    void OnMove(const FInputActionValue& Value);
    void OnLook(const FInputActionValue& Value);
    void OnFireStarted();
    void OnFireCompleted();
    void OnADSStarted();
    void OnADSCompleted();
    void OnReload();
    void OnSkill();
    void OnBurst();
    void OnSprintStarted();
    void OnSprintCompleted();
    void OnDash();
    void OnCrouch();
    void OnJump();
    void OnToggleView();
    void OnSwitchWeaponSlot(int32 SlotIndex);
    void OnQuickSwitchWeapon(float Direction);
    void OnThrowGrenadeStarted();                     // 按住 G：显示抛物线轨迹
    void OnThrowGrenadeCompleted();                   // 松开 G：投掷手雷
    void OnSwitchCharacter();                         // Tab：双角色切换
    void OnInteract();

    // ═══ 鼠标灵敏度 ═══
    UPROPERTY(EditDefaultsOnly, Category="Input|Sensitivity")
    float MouseSensitivity = 1.0f;

    UPROPERTY(EditDefaultsOnly, Category="Input|Sensitivity")
    float ADSSensitivityMultiplier = 0.6f;  // ADS 时降低灵敏度
};
```

**输入绑定表：**

| Input Action | 默认按键 | 触发类型 | 说明 |
|-------------|---------|---------|------|
| `IA_Move` | WASD | Held | 角色移动（Axis2D） |
| `IA_Look` | 鼠标移动 | Continuous | 摄像机旋转 |
| `IA_Fire` | 鼠标左键 | Started + Held + Completed | 射击（支持全自动/半自动） |
| `IA_ADS` | 鼠标右键 | Started + Completed | 进入/退出瞄准 |
| `IA_Reload` | R | Started | 换弹 |
| `IA_Skill` | E | Started | 元素技能 |
| `IA_Burst` | Q | Started | 元素爆发（大招） |
| `IA_ThrowGrenade` | G / 3 | Started + Completed | **水/火/冰/雷 元素手雷（长按瞄准抛物线，松开投掷）** |
| `IA_Sprint` | Left Shift (Hold) | Started + Completed | 冲刺（持续） |
| `IA_Dash` | Left Shift (Tap) | Started | 闪避（短按） |
| `IA_Crouch` | Left Ctrl | Started | 蹲下（切换） |
| `IA_Jump` | Space | Started | 跳跃 |
| `IA_ToggleView` | V | Started | 切换 FPS/TPS 视角 |
| `IA_SwitchWeapon1` | 1 | Started | 切换至主武器（Primary） |
| `IA_SwitchWeapon2` | 2 | Started | 切换至副武器（Secondary） |
| `IA_QuickSwitchWeapon`| 鼠标滚轮 | Triggered | 主/副武器快速轮换 |
| `IA_SwitchCharacter` | Tab / C | Started | **双角色即时切换** |
| `IA_Interact` | F | Started | 交互（拾取/对话/传送门） |

**Sprint vs Dash 区分逻辑：**

```cpp
// Shift 键同时绑定 Sprint 和 Dash，通过按压时长区分
// 方案：Shift 按下 → 启动 0.15s 延迟计时器
//       0.15s 内松开 → 判定为 Dash（短按闪避）
//       0.15s 后仍按住 → 判定为 Sprint（长按冲刺）

void AESPlayerController::OnSprintStarted()
{
    bShiftPressed = true;
    GetWorld()->GetTimerManager().SetTimer(SprintDelayHandle, [this]()
    {
        if (bShiftPressed)
        {
            // 长按 → 冲刺
            GetPawn<AESPlayerCharacter>()->GetMovementComp()->StartSprint();
        }
    }, 0.15f, false);
}

void AESPlayerController::OnSprintCompleted()
{
    bShiftPressed = false;
    GetWorld()->GetTimerManager().ClearTimer(SprintDelayHandle);

    auto* Movement = GetPawn<AESPlayerCharacter>()->GetMovementComp();
    if (Movement->bIsSprinting)
    {
        Movement->StopSprint();  // 松开停止冲刺
    }
    else
    {
        Movement->Dash();  // 短按 → 闪避
    }
}
```

---

## 4. 武器系统

### 4.1 武器基类

每把武器自带元素属性，射击即附着元素。武器是挂载在角色上的 Actor Component，切人时武器跟随角色切换。

```cpp
// ESWeaponBase.h
UENUM(BlueprintType)
enum class EFireMode : uint8
{
    SemiAuto,     // 半自动（点一下打一发）
    FullAuto,     // 全自动（按住连射）
    Burst,        // 点射（按一下打 N 发）
    Charge        // 蓄力（按住蓄力，松开发射）
};

UCLASS(Abstract)
class AESWeaponBase : public AActor
{
    GENERATED_BODY()
public:
    // ═══ 武器身份 ═══
    UPROPERTY(EditDefaultsOnly, Category="Weapon")
    FGameplayTag WeaponID;                    // e.g. "Weapon.Rifle.Pyro.AK47"

    UPROPERTY(EditDefaultsOnly, Category="Weapon")
    FText WeaponName;

    UPROPERTY(EditDefaultsOnly, Category="Weapon")
    FGameplayTag WeaponType;                  // Weapon.Rifle / Weapon.Shotgun / ...

    UPROPERTY(EditDefaultsOnly, Category="Weapon")
    FGameplayTag Element;                     // Element.Pyro / Element.Cryo / ...

    // ═══ 射击参数 ═══
    UPROPERTY(EditDefaultsOnly, Category="Weapon|Fire")
    EFireMode FireMode = EFireMode::FullAuto;

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Fire")
    float FireRate = 600.f;                   // 每分钟射速（RPM）

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Fire")
    float BaseDamage = 35.f;                  // 单发基础伤害

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Fire")
    float HeadshotMultiplier = 2.0f;          // 爆头倍率

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Fire")
    float Range = 5000.f;                     // 有效射程（cm）

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Fire")
    float DamageDropoffStart = 2000.f;        // 伤害衰减起始距离
    UPROPERTY(EditDefaultsOnly, Category="Weapon|Fire")
    float DamageDropoffEnd = 5000.f;          // 伤害衰减终止距离
    UPROPERTY(EditDefaultsOnly, Category="Weapon|Fire")
    float MinDamageMultiplier = 0.5f;         // 最远距离最低伤害倍率

    // ═══ 弹药 ═══
    UPROPERTY(EditDefaultsOnly, Category="Weapon|Ammo")
    int32 MagazineSize = 30;                  // 弹匣容量

    UPROPERTY(BlueprintReadOnly, Category="Weapon|Ammo")
    int32 CurrentAmmo;                        // 当前弹匣剩余

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Ammo")
    int32 MaxReserveAmmo = 150;               // 备弹上限

    UPROPERTY(BlueprintReadOnly, Category="Weapon|Ammo")
    int32 CurrentReserveAmmo;                 // 当前备弹

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Ammo")
    float ReloadTime = 2.0f;                  // 换弹时间（秒）

    // ═══ 精准度 & 散布 ═══
    UPROPERTY(EditDefaultsOnly, Category="Weapon|Accuracy")
    float BaseSpread = 1.0f;                  // 基础散布角度（度）

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Accuracy")
    float MaxSpread = 5.0f;                   // 连射时最大散布

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Accuracy")
    float SpreadIncreasePerShot = 0.3f;       // 每次射击增加的散布

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Accuracy")
    float SpreadRecoveryRate = 3.0f;          // 散布每秒恢复速率

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Accuracy")
    float ADSSpreadMultiplier = 0.3f;         // ADS 时散布缩减为 30%

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Accuracy")
    float MovingSpreadMultiplier = 1.5f;      // 移动时散布增大 50%

    // ═══ 元素附着参数 ═══
    UPROPERTY(EditDefaultsOnly, Category="Weapon|Element")
    float ElementGaugePerShot = 1.0f;         // 每发子弹附着的元素量

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Element")
    float ElementGaugePerSecond = 0.f;        // 持续附着速率（针对射线型武器）

    // ═══ 核心方法 ═══
    UFUNCTION(BlueprintCallable) virtual void StartFire();
    UFUNCTION(BlueprintCallable) virtual void StopFire();
    UFUNCTION(BlueprintCallable) virtual void Reload();
    UFUNCTION(BlueprintCallable) virtual bool CanFire() const;
    UFUNCTION(BlueprintCallable) virtual bool CanReload() const;

    // ═══ 委托 ═══
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAmmoChanged, int32, Current, int32, Max);
    UPROPERTY(BlueprintAssignable) FOnAmmoChanged OnAmmoChanged;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReloadStart);
    UPROPERTY(BlueprintAssignable) FOnReloadStart OnReloadStart;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReloadEnd);
    UPROPERTY(BlueprintAssignable) FOnReloadEnd OnReloadEnd;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFireSingle);
    UPROPERTY(BlueprintAssignable) FOnFireSingle OnFireSingle;

protected:
    // ═══ 射击实现 ═══
    virtual void FireOnce();                   // 单次射击
    virtual void ProcessHit(FHitResult& Hit);  // 命中处理
    float CalculateDamageDropoff(float Distance) const;  // 距离衰减

    // ═══ 状态 ═══
    bool bIsFiring = false;
    bool bIsReloading = false;
    float CurrentSpread;                       // 当前散布
    float LastFireTime = 0.f;                  // 上次射击时间

    // ═══ 组件 ═══
    UPROPERTY(VisibleAnywhere) USkeletalMeshComponent* WeaponMesh;
    UPROPERTY(VisibleAnywhere) URecoilComponent* RecoilComp;

    // ═══ 射击模式执行 ═══
    FTimerHandle FireTimerHandle;              // 全自动连射定时器
    void AutoFireTick();
};
```

### 4.2 后坐力系统

后坐力是 FPS 手感的核心。采用**可学习的后坐力模式**（类 CS2），每把枪有固定的后坐力曲线，玩家可以练习压枪：

```cpp
// RecoilComponent.h
UCLASS()
class URecoilComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    // ═══ 后坐力模式配置 ═══
    // 每把枪有一条预定义的后坐力曲线（Pitch 上抬 + Yaw 左右偏移）
    UPROPERTY(EditDefaultsOnly, Category="Recoil")
    TArray<FVector2D> RecoilPattern;           // 每发子弹的 (Pitch, Yaw) 偏移
    // 例如 AK 步枪前 10 发：
    // [(0.8, 0), (0.7, 0.1), (0.6, 0.2), (0.5, -0.3), ...]

    UPROPERTY(EditDefaultsOnly, Category="Recoil")
    float RecoilMultiplier = 1.0f;             // 全局后坐力倍率

    UPROPERTY(EditDefaultsOnly, Category="Recoil")
    float ADSRecoilMultiplier = 0.7f;          // ADS 时后坐力降低 30%

    UPROPERTY(EditDefaultsOnly, Category="Recoil")
    float RecoverySpeed = 5.0f;                // 停止射击后准星回正速率

    UPROPERTY(EditDefaultsOnly, Category="Recoil")
    float RandomSpreadFactor = 0.1f;           // 在固定模式基础上叠加的随机散布

    // ═══ 核心方法 ═══
    UFUNCTION(BlueprintCallable)
    void ApplyRecoil();                        // 每次射击调用

    UFUNCTION(BlueprintCallable)
    void ResetRecoil();                        // 停止射击 / 换弹 → 重置模式索引

    void TickComponent(float DeltaTime, ELevelTick TickType,
                       FActorComponentTickFunction* ThisTickFunction) override;

private:
    int32 PatternIndex = 0;                    // 当前后坐力模式索引
    FVector2D AccumulatedRecoil;               // 累积后坐力（用于回正）

    void ApplyRecoilToController(FVector2D RecoilOffset);
    void RecoverRecoil(float DeltaTime);       // 平滑回正
};
```

**后坐力执行流程：**

```cpp
void URecoilComponent::ApplyRecoil()
{
    if (RecoilPattern.Num() == 0) return;

    // 1. 从预定义模式取当前发数的偏移
    int32 Index = FMath::Min(PatternIndex, RecoilPattern.Num() - 1);
    FVector2D BaseOffset = RecoilPattern[Index];

    // 2. 叠加少量随机散布（让每次射击不完全一样）
    FVector2D RandomOffset(
        FMath::FRandRange(-RandomSpreadFactor, RandomSpreadFactor),
        FMath::FRandRange(-RandomSpreadFactor, RandomSpreadFactor)
    );

    // 3. 应用 ADS 倍率
    float Multiplier = RecoilMultiplier;
    if (auto* Camera = GetOwner()->FindComponentByClass<UESCameraComponent>())
    {
        if (Camera->CurrentMode == ECameraMode::OverShoulder)
            Multiplier *= ADSRecoilMultiplier;
    }

    FVector2D FinalOffset = (BaseOffset + RandomOffset) * Multiplier;

    // 4. 施加到玩家 Controller 的视角
    ApplyRecoilToController(FinalOffset);

    // 5. 累积记录（用于停火后回正）
    AccumulatedRecoil += FinalOffset;
    PatternIndex++;
}

void URecoilComponent::RecoverRecoil(float DeltaTime)
{
    // 停止射击后，准星平滑回正到原位
    if (!bIsFiring && AccumulatedRecoil.Size() > 0.01f)
    {
        FVector2D Recovery = AccumulatedRecoil * RecoverySpeed * DeltaTime;
        ApplyRecoilToController(-Recovery);
        AccumulatedRecoil -= Recovery;
    }
}
```

### 4.3 各武器类型参数预设

| 参数 | 步枪 (Rifle) | 霰弹枪 (Shotgun) | 狙击枪 (Sniper) | 冲锋枪 (SMG) | 榴弹 (Launcher) |
|------|-------------|------------------|-----------------|-------------|-----------------|
| **射速 (RPM)** | 600 | 80 | 40 | 900 | 60 |
| **单发伤害** | 35 | 12×8 pellets | 150 | 22 | 200 (AoE) |
| **弹匣** | 30 | 6 | 5 | 45 | 3 |
| **换弹时间** | 2.0s | 0.5s×6 (逐发) | 3.0s | 1.8s | 2.5s |
| **射击模式** | FullAuto | SemiAuto | SemiAuto | FullAuto | SemiAuto |
| **有效射程** | 5000cm | 1500cm | 15000cm | 3000cm | 4000cm |
| **爆头倍率** | ×2.0 | ×1.5 | ×3.0 | ×1.8 | ×1.0 (无爆头) |
| **元素量/发** | 1.0 | 0.3×8 | 2.0 | 0.5 | 2.5 |
| **基础散布** | 1.0° | 8.0° | 0.1° | 2.0° | 0.5° |
| **后坐力** | 中等 | 极高 | 极高(单发) | 低 | 高 |
| **ADS速度** | 300 | 350 | 200 | 350 | 280 |
| **特殊机制** | — | 近距伤害加成 | 蓄力满伤 | — | 范围伤害 |

### 4.4 射击实现（Hitscan vs Projectile）

大部分武器使用 **Hitscan（射线检测即时命中）** 以保证手感，榴弹发射器使用 **Projectile（抛物线投射物）**：

```cpp
// Hitscan 射击（步枪/狙击/冲锋枪/霰弹）
void AESWeaponBase::FireOnce()
{
    if (!CanFire()) return;

    CurrentAmmo--;
    OnAmmoChanged.Broadcast(CurrentAmmo, MagazineSize);
    OnFireSingle.Broadcast();

    // 1. 计算射击方向（应用散布）
    AESPlayerCharacter* Owner = Cast<AESPlayerCharacter>(GetOwner());
    FVector MuzzleLocation = WeaponMesh->GetSocketLocation(FName("Muzzle"));
    FVector AimDirection = Owner->GetCameraComponent()->GetForwardVector();

    // 散布：在锥形范围内随机偏移
    float FinalSpread = CurrentSpread;
    if (Owner->GetCameraComponent()->CurrentMode == ECameraMode::OverShoulder)
        FinalSpread *= ADSSpreadMultiplier;
    if (Owner->GetMovementComponent()->Velocity.Size() > 10.f)
        FinalSpread *= MovingSpreadMultiplier;

    FVector SpreadDir = FMath::VRandCone(AimDirection,
        FMath::DegreesToRadians(FinalSpread / 2.f));

    // 2. 射线检测
    FVector TraceEnd = MuzzleLocation + SpreadDir * Range;
    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);
    Params.AddIgnoredActor(Owner);

    bool bHit = GetWorld()->LineTraceSingleByChannel(
        Hit, MuzzleLocation, TraceEnd, ECC_GameTraceChannel1, Params);

    // 3. 命中处理
    if (bHit)
    {
        ProcessHit(Hit);
    }

    // 4. 后坐力
    RecoilComp->ApplyRecoil();

    // 5. 散布增加
    CurrentSpread = FMath::Min(CurrentSpread + SpreadIncreasePerShot, MaxSpread);

    // 6. 视觉效果（枪口火焰、弹道拖尾）
    SpawnMuzzleFlash();
    SpawnBulletTrail(MuzzleLocation, bHit ? Hit.ImpactPoint : TraceEnd);

    LastFireTime = GetWorld()->GetTimeSeconds();
}

// 命中处理
void AESWeaponBase::ProcessHit(FHitResult& Hit)
{
    AActor* HitActor = Hit.GetActor();
    if (!HitActor) return;

    // 1. 计算伤害
    float Distance = (Hit.ImpactPoint - GetActorLocation()).Size();
    float DropoffMultiplier = CalculateDamageDropoff(Distance);
    float FinalDamage = BaseDamage * DropoffMultiplier;

    // 爆头检测（通过物理材质或骨骼名判定）
    bool bHeadshot = false;
    if (Hit.BoneName == FName("head") ||
        (Hit.PhysMaterial.IsValid() && Hit.PhysMaterial->SurfaceType == EPhysicalSurface::SurfaceType1))
    {
        FinalDamage *= HeadshotMultiplier;
        bHeadshot = true;
    }

    // 2. 应用伤害
    if (UHealthComponent* HealthComp = HitActor->FindComponentByClass<UHealthComponent>())
    {
        HealthComp->TakeDamage(FinalDamage, Element);
    }

    // 3. 元素附着
    if (UElementComponent* ElemComp = HitActor->FindComponentByClass<UElementComponent>())
    {
        ElemComp->ApplyElement(Element, ElementGaugePerShot);
    }

    // 4. 命中反馈
    if (UHitFeedbackComponent* FeedbackComp = GetOwner()->FindComponentByClass<UHitFeedbackComponent>())
    {
        FeedbackComp->PlayHitFeedback(Hit, FinalDamage, bHeadshot, Element);
    }

    // 5. 广播事件
    UESEventBus::Get(this)->OnDamageDealt.Broadcast(GetOwner(), HitActor, FinalDamage, Element);
}

// 距离伤害衰减
float AESWeaponBase::CalculateDamageDropoff(float Distance) const
{
    if (Distance <= DamageDropoffStart) return 1.0f;
    if (Distance >= DamageDropoffEnd) return MinDamageMultiplier;

    float Alpha = (Distance - DamageDropoffStart) / (DamageDropoffEnd - DamageDropoffStart);
    return FMath::Lerp(1.0f, MinDamageMultiplier, Alpha);
}
```

### 4.5 霰弹枪特殊处理（多弹丸）

```cpp
// ESShotgun.cpp
void AESShotgun::FireOnce()
{
    if (!CanFire()) return;
    CurrentAmmo--;

    AESPlayerCharacter* Owner = Cast<AESPlayerCharacter>(GetOwner());
    FVector MuzzleLocation = WeaponMesh->GetSocketLocation(FName("Muzzle"));
    FVector AimDirection = Owner->GetCameraComponent()->GetForwardVector();

    // 霰弹枪发射多个弹丸，每个弹丸独立散布
    for (int32 i = 0; i < PelletCount; i++)
    {
        FVector PelletDir = FMath::VRandCone(AimDirection,
            FMath::DegreesToRadians(BaseSpread / 2.f));

        FVector TraceEnd = MuzzleLocation + PelletDir * Range;
        FHitResult Hit;
        FCollisionQueryParams Params;
        Params.AddIgnoredActor(this);
        Params.AddIgnoredActor(Owner);

        if (GetWorld()->LineTraceSingleByChannel(Hit, MuzzleLocation, TraceEnd,
            ECC_GameTraceChannel1, Params))
        {
            // 每个弹丸独立计算伤害和元素附着
            ProcessHit(Hit);
        }

        // 每个弹丸独立弹道特效
        SpawnBulletTrail(MuzzleLocation, Hit.bBlockingHit ? Hit.ImpactPoint : TraceEnd);
    }

    // 后坐力（霰弹枪一次性大后坐力）
    RecoilComp->ApplyRecoil();
    SpawnMuzzleFlash();

    OnAmmoChanged.Broadcast(CurrentAmmo, MagazineSize);
    OnFireSingle.Broadcast();
}
```

### 4.6 武器数据资产

所有武器参数通过 DataAsset 配置，C++ 代码只写逻辑，策划可以直接在编辑器中调参数：

```cpp
// WeaponDataAsset.h
UCLASS()
class UWeaponDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditDefaultsOnly) FGameplayTag WeaponID;
    UPROPERTY(EditDefaultsOnly) FText DisplayName;
    UPROPERTY(EditDefaultsOnly) FText Description;
    UPROPERTY(EditDefaultsOnly) UTexture2D* Icon;
    UPROPERTY(EditDefaultsOnly) USkeletalMesh* WeaponMeshAsset;
    UPROPERTY(EditDefaultsOnly) UAnimMontage* FireMontage;
    UPROPERTY(EditDefaultsOnly) UAnimMontage* ReloadMontage;
    UPROPERTY(EditDefaultsOnly) USoundCue* FireSound;
    UPROPERTY(EditDefaultsOnly) USoundCue* ReloadSound;
    UPROPERTY(EditDefaultsOnly) UNiagaraSystem* MuzzleFlashFX;
    UPROPERTY(EditDefaultsOnly) UNiagaraSystem* BulletTrailFX;
    UPROPERTY(EditDefaultsOnly) UNiagaraSystem* ImpactFX;

    // 射击参数
    UPROPERTY(EditDefaultsOnly) FGameplayTag WeaponType;
    UPROPERTY(EditDefaultsOnly) FGameplayTag Element;
    UPROPERTY(EditDefaultsOnly) EFireMode FireMode;
    UPROPERTY(EditDefaultsOnly) float FireRate;
    UPROPERTY(EditDefaultsOnly) float BaseDamage;
    UPROPERTY(EditDefaultsOnly) float HeadshotMultiplier;
    UPROPERTY(EditDefaultsOnly) int32 MagazineSize;
    UPROPERTY(EditDefaultsOnly) float ReloadTime;
    UPROPERTY(EditDefaultsOnly) float Range;

    // 精准度
    UPROPERTY(EditDefaultsOnly) float BaseSpread;
    UPROPERTY(EditDefaultsOnly) float MaxSpread;
    UPROPERTY(EditDefaultsOnly) float SpreadIncreasePerShot;
    UPROPERTY(EditDefaultsOnly) float ADSSpreadMultiplier;

    // 后坐力模式
    UPROPERTY(EditDefaultsOnly) TArray<FVector2D> RecoilPattern;
    UPROPERTY(EditDefaultsOnly) float RecoilMultiplier;

    // 元素
    UPROPERTY(EditDefaultsOnly) float ElementGaugePerShot;

    // 稀有度
    UPROPERTY(EditDefaultsOnly) int32 Rarity = 3;  // 1-5 星
};
```

### 4.7 双武器切换与槽位管理（ESWeaponComponent）

在 FPS 模式下，角色配置 **2 个武器槽位（0: 主武器 Primary, 1: 副武器 Secondary）**，支持数字键 1/2 或鼠标滚轮极速对调，结合 G 键元素投掷物打出元素反应 Combo：

```cpp
// ESWeaponComponent.h — 挂载在角色身上的武器管理中枢
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UESWeaponComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    // ═══ 2 个武器槽位（0: 主武器, 1: 副武器）═══
    UPROPERTY(EditDefaultsOnly, Category="WeaponSlots")
    int32 MaxWeaponSlots = 2;

    UPROPERTY(BlueprintReadOnly, Category="WeaponSlots")
    TArray<AESWeaponBase*> EquippedWeapons;    // 大小固定为 2

    UPROPERTY(BlueprintReadOnly, Category="WeaponSlots")
    int32 CurrentWeaponIndex = 0;              // 0 或 1

    UPROPERTY(BlueprintReadOnly, Category="WeaponSlots")
    AESWeaponBase* CurrentWeapon = nullptr;

    // ═══ 切枪参数 ═══
    UPROPERTY(EditDefaultsOnly, Category="WeaponSwitch")
    float HolsterDuration = 0.18f;             // 极速收枪（0.18s）
    UPROPERTY(EditDefaultsOnly, Category="WeaponSwitch")
    float UnholsterDuration = 0.22f;           // 极速拔枪（0.22s）
    UPROPERTY(BlueprintReadOnly)
    bool bIsSwitchingWeapon = false;

    // ═══ 核心方法 ═══
    UFUNCTION(BlueprintCallable)
    bool EquipWeaponToSlot(TSubclassOf<AESWeaponBase> WeaponClass, int32 SlotIndex);

    UFUNCTION(BlueprintCallable)
    bool SwitchToSlot(int32 TargetSlot);

    UFUNCTION(BlueprintCallable)
    void ToggleWeapon();                      // 滚轮一键对调主副武器

    UFUNCTION(BlueprintCallable)
    AESWeaponBase* GetCurrentWeapon() const { return CurrentWeapon; }

    // ═══ 委托 ═══
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnWeaponSwitched,
        AESWeaponBase*, OldWeapon, AESWeaponBase*, NewWeapon, int32, NewSlot);
    UPROPERTY(BlueprintAssignable) FOnWeaponSwitched OnWeaponSwitched;

private:
    FTimerHandle SwitchTimerHandle;

    void PerformWeaponSwitch(int32 TargetSlot);
    void FinishWeaponSwitch(AESWeaponBase* NewWeapon, int32 TargetSlot);
};
```

**切枪逻辑实现：**

```cpp
// ESWeaponComponent.cpp
bool UESWeaponComponent::SwitchToSlot(int32 TargetSlot)
{
    if (bIsSwitchingWeapon || TargetSlot == CurrentWeaponIndex) return false;
    if (!EquippedWeapons.IsValidIndex(TargetSlot) || !EquippedWeapons[TargetSlot]) return false;

    if (CurrentWeapon)
    {
        CurrentWeapon->StopFire();
    }
    if (auto* OwnerChar = Cast<AESPlayerCharacter>(GetOwner()))
    {
        OwnerChar->GetCameraComponent()->ExitADS();
    }

    bIsSwitchingWeapon = true;
    PerformWeaponSwitch(TargetSlot);
    return true;
}

void UESWeaponComponent::ToggleWeapon()
{
    int32 TargetSlot = (CurrentWeaponIndex == 0) ? 1 : 0;
    SwitchToSlot(TargetSlot);
}

void UESWeaponComponent::PerformWeaponSwitch(int32 TargetSlot)
{
    AESWeaponBase* OldWeapon = CurrentWeapon;
    AESWeaponBase* NewWeapon = EquippedWeapons[TargetSlot];

    if (OldWeapon)
    {
        OldWeapon->SetActorHiddenInGame(false);
    }

    GetWorld()->GetTimerManager().SetTimer(SwitchTimerHandle, [this, OldWeapon, NewWeapon, TargetSlot]()
    {
        if (OldWeapon)
        {
            OldWeapon->SetActorHiddenInGame(true);
            OldWeapon->AttachToComponent(Cast<ACharacter>(GetOwner())->GetMesh(),
                FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("HolsterSocket"));
        }

        if (NewWeapon)
        {
            NewWeapon->SetActorHiddenInGame(false);
            NewWeapon->AttachToComponent(Cast<ACharacter>(GetOwner())->GetMesh(),
                FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("WeaponHandSocket"));
        }

        FinishWeaponSwitch(NewWeapon, TargetSlot);
    }, HolsterDuration, false);
}

void UESWeaponComponent::FinishWeaponSwitch(AESWeaponBase* NewWeapon, int32 TargetSlot)
{
    GetWorld()->GetTimerManager().SetTimer(SwitchTimerHandle, [this, NewWeapon, TargetSlot]()
    {
        AESWeaponBase* OldWeapon = CurrentWeapon;
        CurrentWeapon = NewWeapon;
        CurrentWeaponIndex = TargetSlot;
        bIsSwitchingWeapon = false;

        OnWeaponSwitched.Broadcast(OldWeapon, NewWeapon, TargetSlot);
    }, UnholsterDuration, false);
}
```

---

### 4.8 元素投掷物系统（水 / 火 / 冰 / 雷 元素手雷）

元素投掷物（Throwable）是单兵或小队作战中**大范围施加元素附着与控场**的核心手段。玩家按住 `G`（或 `3` 键）实时预览抛物线轨迹，松开后投掷。

```
┌─────────────────────────────────────────────────────────────┐
│                    四大元素投掷物设计                       │
├─────────────────────────────────────────────────────────────┤
│ 🔥 烈焰爆轰雷 (Pyro Grenade)   │ 落地瞬间剧烈火爆，强火附着 (2.0U)，   │
│                                │ 留下 3s 燃烧火海，持续造成火伤与附着  │
├────────────────────────────────┤────────────────────────────┤
│ 💧 潮汐洪流雷 (Hydro Grenade)  │ 爆裂大范围水雾，强水附着 (2.0U)，     │
│                                │ 形成 4s 水雾领域，持续潮湿大范围敌人  │
├────────────────────────────────┤────────────────────────────┤
│ 🧊 霜华极寒雷 (Cryo Grenade)   │ 冰晶爆碎，强冰附着 (2.0U)，造成 50%   │
│                                │ 大范围减速，与水雷/水枪配合瞬间群冻   │
├────────────────────────────────┤────────────────────────────┤
│ ⚡ 脉冲雷暴雷 (Electro Grenade)│ 释放电磁脉冲波，强雷附着 (2.0U)，     │
│                                │ 向周围 4 个目标发射连锁电弧持续破盾   │
└─────────────────────────────────────────────────────────────┘
```

#### 4.8.1 投掷物基类（AESThrowableBase）

```cpp
// ESThrowableBase.h
UCLASS(Abstract)
class AESThrowableBase : public AActor
{
    GENERATED_BODY()
public:
    AESThrowableBase();

    // ═══ 核心组件 ═══
    UPROPERTY(VisibleAnywhere) USphereComponent* CollisionComp;
    UPROPERTY(VisibleAnywhere) UProjectileMovementComponent* ProjectileMovement;
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* MeshComp;

    // ═══ 元素与爆炸属性 ═══
    UPROPERTY(EditDefaultsOnly, Category="Throwable|Element")
    FGameplayTag Element;                     // Element.Pyro / Hydro / Cryo / Electro

    UPROPERTY(EditDefaultsOnly, Category="Throwable|Stats")
    float BaseDamage = 120.f;                 // 基础爆炸伤害

    UPROPERTY(EditDefaultsOnly, Category="Throwable|Stats")
    float ExplosionRadius = 600.f;            // 爆炸影响半径（cm）

    UPROPERTY(EditDefaultsOnly, Category="Throwable|Stats")
    float ElementGauge = 2.0f;                // 强元素附着（2.0 单位）

    UPROPERTY(EditDefaultsOnly, Category="Throwable|Stats")
    float FuseTime = 2.0f;                    // 引信引爆倒计时（0 = 触地即炸）

    UPROPERTY(EditDefaultsOnly, Category="Throwable|Stats")
    bool bExplodeOnImpact = false;            // 是否碰触敌人立即引爆

    UPROPERTY(EditDefaultsOnly, Category="Throwable|Field")
    bool bLeavesElementalField = true;        // 是否留下地面持续元素领域
    UPROPERTY(EditDefaultsOnly, Category="Throwable|Field")
    float FieldDuration = 3.5f;               // 元素领域持续时间

    // ═══ 特效与音效 ═══
    UPROPERTY(EditDefaultsOnly, Category="Throwable|FX")
    UNiagaraSystem* ExplosionFX;
    UPROPERTY(EditDefaultsOnly, Category="Throwable|FX")
    UNiagaraSystem* ResidualFieldFX;
    UPROPERTY(EditDefaultsOnly, Category="Throwable|FX")
    USoundCue* ExplosionSound;

    // ═══ 核心触发 ═══
    UFUNCTION()
    virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
                       UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

    virtual void Explode();

protected:
    virtual void BeginPlay() override;
    FTimerHandle FuseTimerHandle;

    void ApplyExplosionDamageAndElement();
    void SpawnResidualField();
};
```

**爆炸结算与元素附着实现：**

```cpp
// ESThrowableBase.cpp
void AESThrowableBase::Explode()
{
    // 1. 播放爆炸音效与 Niagara 粒子
    if (ExplosionFX)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ExplosionFX, GetActorLocation());
    }
    if (ExplosionSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, GetActorLocation());
    }

    // 2. 球形射线范围伤害与元素附着
    ApplyExplosionDamageAndElement();

    // 3. 生成残留元素领域（如燃烧地面或水雾）
    if (bLeavesElementalField)
    {
        SpawnResidualField();
    }

    // 4. 销毁或回收到对象池
    Destroy();
}

void AESThrowableBase::ApplyExplosionDamageAndElement()
{
    TArray<FHitResult> HitResults;
    FVector Center = GetActorLocation();
    FCollisionShape Sphere = FCollisionShape::MakeSphere(ExplosionRadius);

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);
    Params.AddIgnoredActor(GetInstigator());

    // 范围重叠检测
    if (GetWorld()->SweepMultiByChannel(HitResults, Center, Center, FQuat::Identity,
        ECC_Pawn, Sphere, Params))
    {
        TSet<AActor*> AffectedActors;

        for (const FHitResult& Hit : HitResults)
        {
            AActor* Target = Hit.GetActor();
            if (!Target || AffectedActors.Contains(Target)) continue;
            AffectedActors.Add(Target);

            // 距离衰减伤害
            float Dist = (Target->GetActorLocation() - Center).Size();
            float Falloff = FMath::Clamp(1.0f - (Dist / ExplosionRadius) * 0.5f, 0.5f, 1.0f);
            float FinalDamage = BaseDamage * Falloff;

            // ① 造成伤害
            if (UHealthComponent* HP = Target->FindComponentByClass<UHealthComponent>())
            {
                HP->TakeDamage(FinalDamage, Element);
            }

            // ② 强元素附着 (2.0U) 触发原神元素反应
            if (UElementComponent* Elem = Target->FindComponentByClass<UElementComponent>())
            {
                Elem->ApplyElement(Element, ElementGauge);
            }

            // ③ 物理击退冲击力
            if (UCharacterMovementComponent* Move = Target->FindComponentByClass<UCharacterMovementComponent>())
            {
                FVector PushDir = (Target->GetActorLocation() - Center).GetSafeNormal();
                PushDir.Z = 0.35f; // 轻微上挑击飞
                Move->AddImpulse(PushDir * 600.f, true);
            }
        }
    }
}
```

#### 4.8.2 投掷管理组件与抛物线预测（UESThrowableComponent）

挂载于玩家角色上，负责**手雷数量/冷却管理、按住 G 绘制精准绿色/红色落点抛物线、松开投掷**：

```cpp
// ESThrowableComponent.h
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UESThrowableComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    // ═══ 投掷物配置 ═══
    UPROPERTY(EditDefaultsOnly, Category="Throwable")
    TSubclassOf<AESThrowableBase> EquippedGrenadeClass; // 当前装备的手雷类型（水/火/冰/雷）

    UPROPERTY(EditDefaultsOnly, Category="Throwable")
    int32 MaxGrenadeCount = 3;                         // 最大携带数量

    UPROPERTY(BlueprintReadOnly, Category="Throwable")
    int32 CurrentGrenadeCount = 3;

    UPROPERTY(EditDefaultsOnly, Category="Throwable")
    float Cooldown = 6.0f;                             // 投掷内置 CD（秒）

    UPROPERTY(BlueprintReadOnly)
    float CooldownRemaining = 0.f;

    UPROPERTY(EditDefaultsOnly, Category="Throwable")
    float ThrowVelocity = 1800.f;                      // 初始投掷初速（cm/s）

    // ═══ 核心方法 ═══
    UFUNCTION(BlueprintCallable)
    void StartAimTrajectory();                         // 按住 G：开启抛物线预测

    UFUNCTION(BlueprintCallable)
    void StopAimAndThrow();                            // 松开 G：投掷手雷

    UFUNCTION(BlueprintCallable)
    void CancelAim();                                  // 右键/ESC 取消投掷

    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
                               FActorComponentTickFunction* ThisTickFunction) override;

    // ═══ 委托 ═══
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGrenadeCountChanged, int32, Current, int32, Max);
    UPROPERTY(BlueprintAssignable) FOnGrenadeCountChanged OnGrenadeCountChanged;

private:
    bool bIsAiming = false;
    UPROPERTY() class USplineComponent* TrajectorySpline;

    void DrawTrajectoryPrediction();
    void SpawnAndLaunchGrenade();
};
```

**抛物线实时物理预测实现（UE5 原生 API）：**

```cpp
// ESThrowableComponent.cpp
void UESThrowableComponent::DrawTrajectoryPrediction()
{
    if (!bIsAiming) return;

    APlayerController* PC = Cast<APlayerController>(Cast<APawn>(GetOwner())->GetController());
    if (!PC || !PC->PlayerCameraManager) return;

    FVector StartPos = GetOwner()->GetActorLocation() + FVector(0, 0, 50);
    FVector LaunchVelocity = PC->PlayerCameraManager->GetActorForwardVector() * ThrowVelocity;

    // UE5 物理投掷物预测参数
    FPredictProjectilePathParams PredictParams;
    PredictParams.StartLocation = StartPos;
    PredictParams.LaunchVelocity = LaunchVelocity;
    PredictParams.bTraceWithCollision = true;
    PredictParams.ProjectileRadius = 8.f;
    PredictParams.MaxSimTime = 3.0f;
    PredictParams.TraceChannel = ECC_WorldStatic;
    PredictParams.ActorsToIgnore.Add(GetOwner());
    PredictParams.DrawDebugType = EDrawDebugTrace::ForOneFrame; // 调试时实时渲染绿色抛物线
    PredictParams.DrawDebugTime = 0.05f;

    FPredictProjectilePathResult PredictResult;
    UGameplayStatics::PredictProjectilePathByTraceChannel(GetWorld(), PredictParams, PredictResult);

    // 落点处显示元素光圈提示（Impact Point）
}
```

---

## 5. 核心战斗系统

### 5.1 伤害公式层

伤害计算是独立的纯函数类，与游戏对象解耦，便于单元测试和策划调参：

```cpp
// DamageCalculator.h — 纯函数，无状态，不持有 UObject 引用
UCLASS()
class UDamageCalculator : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    // ═══ 主伤害公式 ═══
    // 最终伤害 = 基础攻击 × 技能/武器倍率 × (1 + 伤害加成%) × 暴击 × 防御减免 × 抗性减免 × 反应倍率
    UFUNCTION(BlueprintCallable, Category="Damage")
    static float CalculateFinalDamage(const FDamageContext& Context);

    // ═══ 各环节独立计算（方便 debug 和 AI 分析）═══

    // 基础伤害 = BaseATK × SkillMultiplier
    UFUNCTION(BlueprintCallable)
    static float CalcBaseDamage(float BaseATK, float SkillMultiplier);

    // 暴击伤害 = bCrit ? (1 + CritDMG%) : 1.0
    UFUNCTION(BlueprintCallable)
    static float CalcCritMultiplier(float CritRate, float CritDMG, bool& bOutCrit);

    // 防御减免 = AttackerLv / (AttackerLv + DefenderDEF × (1 - DEFReduction%))
    // 参考原神公式：等级差防御减免
    UFUNCTION(BlueprintCallable)
    static float CalcDefenseMultiplier(int32 AttackerLevel, float DefenderDEF,
                                        float DEFReduction = 0.f);

    // 抗性减免：
    //   抗性 < 0%  → 倍率 = 1 - 抗性/2（负抗性减半生效）
    //   0% ≤ 抗性 < 75% → 倍率 = 1 - 抗性
    //   抗性 ≥ 75% → 倍率 = 1 / (4×抗性 + 1)（高抗性递减）
    UFUNCTION(BlueprintCallable)
    static float CalcResistanceMultiplier(float Resistance);

    // 元素反应倍率（增幅反应：蒸发/融化）
    UFUNCTION(BlueprintCallable)
    static float CalcAmplifyingReaction(FGameplayTag Reaction, FGameplayTag TriggerElement,
                                        float ElementalMastery);

    // 元素反应固定伤害（剧变反应：超载/超导/感电/扩散）
    UFUNCTION(BlueprintCallable)
    static float CalcTransformativeReaction(FGameplayTag Reaction, int32 CharacterLevel,
                                             float ElementalMastery);

    // ═══ 模拟计算（不执行，纯预估）═══
    UFUNCTION(BlueprintCallable)
    static FDamagePreview SimulateDamage(const FDamageSimulationInput& Input);
};

// 伤害上下文
USTRUCT(BlueprintType)
struct FDamageContext
{
    GENERATED_BODY()

    UPROPERTY() float BaseATK = 0.f;
    UPROPERTY() float SkillMultiplier = 1.0f;      // 武器/技能倍率
    UPROPERTY() float DamageBonus = 0.f;            // 伤害加成 %（火伤加成、物理加成等）
    UPROPERTY() float CritRate = 0.05f;
    UPROPERTY() float CritDMG = 0.5f;
    UPROPERTY() int32 AttackerLevel = 1;
    UPROPERTY() float DefenderDEF = 500.f;
    UPROPERTY() float DEFReduction = 0.f;           // 减防效果 %
    UPROPERTY() float ElementResistance = 0.1f;     // 敌人对应元素抗性
    UPROPERTY() FGameplayTag DamageElement;          // 伤害元素
    UPROPERTY() FGameplayTag Reaction;               // 触发的反应（空 = 无反应）
    UPROPERTY() FGameplayTag TriggerElement;         // 触发反应的元素
    UPROPERTY() float ElementalMastery = 0.f;       // 攻击者元素精通
    UPROPERTY() float DistanceDropoff = 1.0f;       // 距离衰减倍率
    UPROPERTY() float HeadshotMultiplier = 1.0f;    // 爆头倍率
    UPROPERTY() bool bIsHeadshot = false;
};
```

**伤害公式实现：**

```cpp
float UDamageCalculator::CalculateFinalDamage(const FDamageContext& Ctx)
{
    // Step 1: 基础伤害
    float Base = Ctx.BaseATK * Ctx.SkillMultiplier;

    // Step 2: 伤害加成
    float BonusMult = 1.0f + Ctx.DamageBonus;

    // Step 3: 暴击
    bool bCrit = false;
    float CritMult = CalcCritMultiplier(Ctx.CritRate, Ctx.CritDMG, bCrit);

    // Step 4: 爆头
    float HeadMult = Ctx.bIsHeadshot ? Ctx.HeadshotMultiplier : 1.0f;

    // Step 5: 距离衰减
    float DistMult = Ctx.DistanceDropoff;

    // Step 6: 防御减免
    float DefMult = CalcDefenseMultiplier(Ctx.AttackerLevel, Ctx.DefenderDEF, Ctx.DEFReduction);

    // Step 7: 抗性减免
    float ResMult = CalcResistanceMultiplier(Ctx.ElementResistance);

    // Step 8: 元素反应
    float ReactionMult = 1.0f;
    if (Ctx.Reaction.IsValid())
    {
        // 增幅反应（乘法）
        if (Ctx.Reaction.MatchesTag(FGameplayTag::RequestGameplayTag("Reaction.Vaporize")) ||
            Ctx.Reaction.MatchesTag(FGameplayTag::RequestGameplayTag("Reaction.Melt")))
        {
            ReactionMult = CalcAmplifyingReaction(Ctx.Reaction, Ctx.TriggerElement,
                                                   Ctx.ElementalMastery);
        }
        // 剧变反应在主公式外独立结算（加法伤害）
    }

    float FinalDamage = Base * BonusMult * CritMult * HeadMult * DistMult
                        * DefMult * ResMult * ReactionMult;

    return FMath::Max(0.f, FinalDamage);
}
```

### 5.2 命中反馈系统

FPS 的打击感不来自顿帧（那是动作游戏的），而来自**视觉 + 听觉 + 触觉反馈的组合**：

```cpp
// HitFeedbackComponent.h
UCLASS()
class UHitFeedbackComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    // ═══ 命中标记（HitMarker）═══
    // 准星中心短暂闪烁 "×" 标记，爆头时变红 + 不同音效
    UPROPERTY(EditDefaultsOnly, Category="Feedback|HitMarker")
    USoundCue* HitSound;                      // 普通命中音效（"叮"）

    UPROPERTY(EditDefaultsOnly, Category="Feedback|HitMarker")
    USoundCue* HeadshotSound;                  // 爆头音效（"铛"，更清脆）

    UPROPERTY(EditDefaultsOnly, Category="Feedback|HitMarker")
    USoundCue* KillSound;                      // 击杀确认音效

    UPROPERTY(EditDefaultsOnly, Category="Feedback|HitMarker")
    float HitMarkerDuration = 0.15f;          // HitMarker 显示时长

    // ═══ 飘字伤害数字 ═══
    UPROPERTY(EditDefaultsOnly, Category="Feedback|DamageNumber")
    TSubclassOf<UDamageNumberWidget> DamageNumberClass;

    // ═══ 击中粒子 ═══
    UPROPERTY(EditDefaultsOnly, Category="Feedback|Impact")
    UNiagaraSystem* FleshImpactFX;            // 命中肉体
    UPROPERTY(EditDefaultsOnly, Category="Feedback|Impact")
    UNiagaraSystem* MetalImpactFX;            // 命中金属
    UPROPERTY(EditDefaultsOnly, Category="Feedback|Impact")
    UNiagaraSystem* StoneImpactFX;            // 命中石头

    // ═══ 元素命中特效 ═══
    UPROPERTY(EditDefaultsOnly, Category="Feedback|Element")
    TMap<FGameplayTag, UNiagaraSystem*> ElementImpactFXMap;
    // Element.Pyro → 火花飞溅
    // Element.Cryo → 冰晶碎裂
    // Element.Electro → 电弧闪烁
    // Element.Hydro → 水花飞溅

    // ═══ 受击反馈（敌人侧）═══
    UPROPERTY(EditDefaultsOnly, Category="Feedback|HitReaction")
    float HitStunDuration = 0.15f;            // 受击硬直

    UPROPERTY(EditDefaultsOnly, Category="Feedback|HitReaction")
    float KnockbackForce = 200.f;             // 击退力度

    // ═══ 核心方法 ═══
    UFUNCTION(BlueprintCallable)
    void PlayHitFeedback(const FHitResult& Hit, float Damage, bool bHeadshot,
                         FGameplayTag Element);

    UFUNCTION(BlueprintCallable)
    void PlayKillFeedback(AActor* KilledActor);

private:
    void ShowHitMarker(bool bHeadshot, bool bKill);
    void SpawnDamageNumber(FVector Location, float Damage, bool bCrit,
                           FGameplayTag Element);
    void SpawnImpactFX(const FHitResult& Hit, FGameplayTag Element);
    void ApplyHitReaction(AActor* HitActor, FVector HitDirection, float Damage);
    void PlayHitSound(bool bHeadshot, bool bKill);
};
```

**命中反馈执行流程：**

```cpp
void UHitFeedbackComponent::PlayHitFeedback(const FHitResult& Hit, float Damage,
    bool bHeadshot, FGameplayTag Element)
{
    // 1. HitMarker（屏幕准星位置闪烁）
    ShowHitMarker(bHeadshot, false);

    // 2. 命中音效（空间化 3D 音效）
    PlayHitSound(bHeadshot, false);

    // 3. 飘字伤害数字（世界空间，从命中点向上飘动）
    SpawnDamageNumber(Hit.ImpactPoint, Damage, bHeadshot, Element);

    // 4. 命中粒子（根据元素和表面材质选择）
    SpawnImpactFX(Hit, Element);

    // 5. 敌人受击反馈（硬直 + 击退 + 材质闪白）
    ApplyHitReaction(Hit.GetActor(), Hit.ImpactNormal, Damage);

    // 6. 摄像机微震（轻微，不要太强影响瞄准）
    if (APlayerController* PC = Cast<APlayerController>(
        Cast<APawn>(GetOwner())->GetController()))
    {
        float ShakeIntensity = FMath::Clamp(Damage / 100.f, 0.1f, 0.5f);
        PC->ClientStartCameraShake(HitCameraShakeClass, ShakeIntensity);
    }
}

void UHitFeedbackComponent::ApplyHitReaction(AActor* HitActor,
    FVector HitDirection, float Damage)
{
    if (!HitActor) return;

    // 材质闪白（0.1s）
    if (USkeletalMeshComponent* Mesh = HitActor->FindComponentByClass<USkeletalMeshComponent>())
    {
        for (int32 i = 0; i < Mesh->GetNumMaterials(); i++)
        {
            UMaterialInstanceDynamic* MatInst = Mesh->CreateDynamicMaterialInstance(i);
            if (MatInst)
            {
                MatInst->SetScalarParameterValue("HitFlash", 1.0f);
                // 使用 Timeline 或 Timer 在 0.1s 后恢复为 0
            }
        }
    }

    // 击退（沿子弹方向施加脉冲力）
    if (UCharacterMovementComponent* Movement =
        HitActor->FindComponentByClass<UCharacterMovementComponent>())
    {
        FVector KnockbackDir = -HitDirection;  // 反向 = 子弹飞行方向
        KnockbackDir.Z = 0.1f;                // 轻微上挑
        KnockbackDir.Normalize();
        Movement->AddImpulse(KnockbackDir * KnockbackForce, true);
    }
}
```

### 5.3 技能组件（E 技能 / Q 大招）

每个角色除了枪械射击外，还有独立的元素技能（E）和元素爆发（Q），这是与原神一致的核心设计：

```cpp
// SkillComponent.h
UCLASS()
class USkillComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    // ═══ E 技能配置 ═══
    UPROPERTY(EditDefaultsOnly, Category="Skill|E")
    USkillDataAsset* ElementalSkillData;

    UPROPERTY(BlueprintReadOnly) float SkillCooldownRemaining = 0.f;
    UPROPERTY(BlueprintReadOnly) bool bSkillOnCooldown = false;

    // ═══ Q 大招配置 ═══
    UPROPERTY(EditDefaultsOnly, Category="Skill|Q")
    USkillDataAsset* ElementalBurstData;

    UPROPERTY(BlueprintReadOnly) float BurstCooldownRemaining = 0.f;
    UPROPERTY(BlueprintReadOnly) bool bBurstOnCooldown = false;

    // ═══ 核心方法 ═══
    UFUNCTION(BlueprintCallable)
    bool UseElementalSkill();                  // E 键

    UFUNCTION(BlueprintCallable)
    bool UseElementalBurst();                  // Q 键

    // ═══ 委托 ═══
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSkillCooldown,
        float, Remaining, float, Total);
    UPROPERTY(BlueprintAssignable) FOnSkillCooldown OnSkillCooldownTick;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBurstCooldown,
        float, Remaining, float, Total);
    UPROPERTY(BlueprintAssignable) FOnBurstCooldown OnBurstCooldownTick;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSkillUsed);
    UPROPERTY(BlueprintAssignable) FOnSkillUsed OnSkillUsed;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBurstUsed);
    UPROPERTY(BlueprintAssignable) FOnBurstUsed OnBurstUsed;

    // Tick 中更新 CD
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
                               FActorComponentTickFunction* ThisTickFunction) override;

private:
    void ExecuteSkillEffect(USkillDataAsset* SkillData);
    void StartCooldown(float& CooldownRemaining, bool& bOnCooldown, float Duration);
};

// 技能数据资产
UCLASS()
class USkillDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditDefaultsOnly) FGameplayTag SkillID;
    UPROPERTY(EditDefaultsOnly) FText SkillName;
    UPROPERTY(EditDefaultsOnly) FText Description;
    UPROPERTY(EditDefaultsOnly) UTexture2D* Icon;

    // 技能参数
    UPROPERTY(EditDefaultsOnly) FGameplayTag Element;           // 技能元素
    UPROPERTY(EditDefaultsOnly) float DamageMultiplier = 2.0f;  // 伤害倍率（基于角色ATK）
    UPROPERTY(EditDefaultsOnly) float Cooldown = 8.0f;          // CD 秒数
    UPROPERTY(EditDefaultsOnly) float EnergyCost = 0.f;         // Q 技能能量消耗
    UPROPERTY(EditDefaultsOnly) float ElementGauge = 2.0f;      // 元素附着量

    // 技能类型
    UPROPERTY(EditDefaultsOnly) ESkillType SkillType;
    // Projectile — 发射元素投射物
    // AoE — 范围爆炸
    // Buff — 给自己/队伍加 buff
    // Summon — 召唤物
    // Transform — 改变射击模式（如：Q 期间所有子弹附加额外元素伤害）

    // 范围参数（AoE 类型）
    UPROPERTY(EditDefaultsOnly) float AoERadius = 300.f;

    // 持续时间（Buff/Transform 类型）
    UPROPERTY(EditDefaultsOnly) float Duration = 0.f;

    // 视觉
    UPROPERTY(EditDefaultsOnly) UAnimMontage* CastMontage;
    UPROPERTY(EditDefaultsOnly) UNiagaraSystem* SkillFX;
    UPROPERTY(EditDefaultsOnly) USoundCue* SkillSound;
};
```

**技能示例设计：**

| 角色 | 元素 | 武器 | E 技能 | Q 大招 |
|------|------|------|--------|--------|
| 烈焰突击手 | 🔥 Pyro | 突击步枪 | 发射火焰手雷，落点范围爆炸 | 10秒内子弹附带额外火焰伤害，射速+20% |
| 寒冰狙击手 | 🧊 Cryo | 狙击枪 | 放置冰墙屏障，阻挡敌人并造成冰元素伤害 | 蓄力射出冰晶箭矢，命中后大范围冻结 |
| 雷霆冲锋 | ⚡ Electro | 冲锋枪 | 瞬移到目标方向，经过的敌人受雷元素伤害 | 召唤雷暴领域，领域内敌人持续受感电 |
| 水潮卫士 | 💧 Hydro | 霰弹枪 | 放置水元素回复领域，队友站内回血 | 召唤水盾保护全队，持续8秒 |

---

## 6. 元素反应系统

### 6.1 元素附着组件（含 ICD 机制）

FPS 的射速远高于近战挥砍，因此 ICD（内部冷却）机制需要重新平衡——否则冲锋枪每秒 15 发子弹会瞬间触发大量反应：

```cpp
// ElementComponent.h
UCLASS()
class UElementComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    // ═══ 当前附着的元素（最多 2 层同时附着）═══
    UPROPERTY(BlueprintReadOnly) FGameplayTag AttachedElement1;
    UPROPERTY(BlueprintReadOnly) FGameplayTag AttachedElement2;
    UPROPERTY(BlueprintReadOnly) float ElementGauge1 = 0.f;  // 元素量（自然衰减）
    UPROPERTY(BlueprintReadOnly) float ElementGauge2 = 0.f;

    // ═══ 附着新元素 ═══
    UFUNCTION(BlueprintCallable)
    void ApplyElement(FGameplayTag NewElement, float Gauge);

    // ═══ 查询 ═══
    UFUNCTION(BlueprintCallable)
    bool HasElement(FGameplayTag Element) const;

    UFUNCTION(BlueprintCallable)
    TArray<FGameplayTag> GetAttachedElements() const;

    // ═══ 委托 ═══
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnElementAttached,
        FGameplayTag, Element, float, Gauge, AActor*, Source);
    UPROPERTY(BlueprintAssignable) FOnElementAttached OnElementAttached;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnElementReaction,
        FGameplayTag, Reaction, float, ReactionDamage);
    UPROPERTY(BlueprintAssignable) FOnElementReaction OnElementReaction;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnElementDecayed,
        FGameplayTag, Element);
    UPROPERTY(BlueprintAssignable) FOnElementDecayed OnElementDecayed;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
                               FActorComponentTickFunction* ThisTickFunction) override;

private:
    // ═══ ICD 机制（针对 FPS 高射速重新平衡）═══
    //
    // 原神 ICD：2.5秒 / 3次命中，满足任一才附着
    // FPS 适配：根据武器类型分别配置 ICD
    //   步枪（600RPM）：1.5秒 / 5次命中
    //   冲锋枪（900RPM）：2.0秒 / 8次命中
    //   狙击枪（40RPM）：0秒 / 1次命中（每发必附着）
    //   霰弹枪（多弹丸）：整次射击算 1 次命中，弹丸不独立计数
    //   技能：无 ICD，每次必附着
    //
    struct FICDTracker
    {
        float Timer = 0.f;
        int32 HitCounter = 0;
        float ICDDuration;        // 时间阈值
        int32 ICDHitThreshold;    // 命中次数阈值
    };

    // 每种攻击源有独立的 ICD 追踪器
    TMap<FGameplayTag, FICDTracker> ICDTrackers;

    bool CanApplyElement(FGameplayTag SourceTag);
    void CheckReactions(FGameplayTag NewElement, AActor* Source);
    void DecayElements(float DeltaTime);
    void AttachToSlot(FGameplayTag Element, float Gauge);
};
```

**ICD 配置表（DataAsset 可调）：**

| 攻击源 | ICD 时间 | ICD 命中次数 | 说明 |
|--------|---------|-------------|------|
| 步枪射击 | 1.5s | 5 hits | 中等射速，适中 ICD |
| 冲锋枪射击 | 2.0s | 8 hits | 高射速需要更严的 ICD |
| 狙击枪射击 | 0s | 1 hit | 每发必附着（低射速补偿） |
| 霰弹枪射击 | 1.0s | 2 hits | 整次射击算 1 次，散弹丸不独立 |
| E 技能 | 0s | 1 hit | 必附着（高价值操作） |
| Q 大招 | 0s | 1 hit | 必附着 |
| 持续伤害（DoT） | 1.0s | — | 固定 1 秒间隔 |

### 6.2 反应管理器

```cpp
// ElementReactionManager.h — 全局单例，管理反应规则
UCLASS()
class UElementReactionManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    // ═══ 反应判定 ═══
    UFUNCTION(BlueprintCallable)
    FReactionResult CheckReaction(FGameplayTag ExistingElement, FGameplayTag IncomingElement);

    // ═══ 执行反应效果 ═══
    UFUNCTION(BlueprintCallable)
    void ExecuteReaction(AActor* Target, const FReactionResult& Result,
                         float AttackerATK, float ElementalMastery, int32 AttackerLevel);

    static UElementReactionManager* Get(const UObject* WorldContext);

private:
    // 反应规则表（静态初始化）
    void InitReactionTable();

    struct FReactionRule
    {
        FGameplayTag Element1;
        FGameplayTag Element2;
        FGameplayTag ReactionTag;
        EReactionCategory Category;  // Amplifying / Transformative / Other
        float BaseMultiplier;        // 增幅反应基础倍率
    };
    TArray<FReactionRule> ReactionRules;

    // ═══ 具体反应效果实现 ═══
    void ApplyVaporize(AActor* Target, float Damage);     // 蒸发：伤害 ×1.5/2.0
    void ApplyMelt(AActor* Target, float Damage);         // 融化：伤害 ×1.5/2.0
    void ApplyOverload(AActor* Target, FVector Center,    // 超载：范围爆炸
                       float Damage);
    void ApplySuperconduct(AActor* Target, FVector Center, // 超导：降物抗 + 范围冰伤
                           float Damage);
    void ApplyFreeze(AActor* Target, float Duration);      // 冻结：冰冻
    void ApplyElectroCharged(AActor* Target, float DPS,    // 感电：持续雷伤
                             float Duration);
    void ApplySwirl(AActor* Target, FGameplayTag SwirlElement,  // 扩散：传播元素
                    float Damage, float Radius);
    void ApplyCrystallize(AActor* Target, FGameplayTag ShieldElement,  // 结晶：生成护盾
                          float ShieldHP);
};

USTRUCT(BlueprintType)
struct FReactionResult
{
    GENERATED_BODY()
    UPROPERTY() bool bTriggered = false;
    UPROPERTY() FGameplayTag ReactionTag;
    UPROPERTY() EReactionCategory Category;
    UPROPERTY() float DamageMultiplier = 1.0f;     // 增幅反应倍率
    UPROPERTY() float FixedDamage = 0.f;           // 剧变反应固定伤害
    UPROPERTY() FGameplayTag ConsumedElement;       // 被消耗的元素
};
```

### 6.3 元素反应规则表（完整版）

| 已附着 | 触发元素 | 反应 | 分类 | 效果 |
|--------|---------|------|------|------|
| Hydro | Pyro | 蒸发 | 增幅 | 伤害 ×1.5 |
| Pyro | Hydro | 蒸发 | 增幅 | 伤害 ×2.0 |
| Cryo | Pyro | 融化 | 增幅 | 伤害 ×2.0 |
| Pyro | Cryo | 融化 | 增幅 | 伤害 ×1.5 |
| Electro | Pyro | 超载 | 剧变 | 半径 500cm 火元素爆炸 + 击飞 |
| Pyro | Electro | 超载 | 剧变 | 同上 |
| Electro | Hydro | 感电 | 剧变 | 持续 3s 雷伤 + 传导周围水元素目标 |
| Hydro | Electro | 感电 | 剧变 | 同上 |
| Electro | Cryo | 超导 | 剧变 | 半径 400cm 冰伤 + 降低物理抗性 40% 持续 12s |
| Cryo | Electro | 超导 | 剧变 | 同上 |
| Cryo | Hydro | 冻结 | 特殊 | 冰冻目标 2s（可被碎冰追加伤害） |
| Hydro | Cryo | 冻结 | 特殊 | 同上 |
| 任意 | Anemo | 扩散 | 剧变 | 将已附着元素向半径 600cm 传播 + 元素伤害 |
| 任意 | Geo | 结晶 | 特殊 | 生成对应元素护盾拾取物，护盾量 = f(等级, 精通) |

**元素量消耗机制（和原神一致）：**

```cpp
void UElementReactionManager::ConsumeGauges(UElementComponent* ElemComp,
    const FReactionResult& Result)
{
    if (Result.Category == EReactionCategory::Amplifying)
    {
        // 增幅反应：触发侧消耗比 1:2 或 2:1
        // 例：火打水（蒸发 ×1.5） → 消耗 2 单位火，1 单位水
        // 例：水打火（蒸发 ×2.0） → 消耗 1 单位水，2 单位火
        if (Result.DamageMultiplier >= 2.0f)
        {
            // 强反应（触发元素量 ×0.5，已附着元素量 ×1.0 消耗）
            ElemComp->ElementGauge1 -= Result.TriggerGauge * 0.5f;
            ElemComp->ElementGauge2 -= Result.ExistingGauge * 1.0f;
        }
        else
        {
            // 弱反应（触发元素量 ×1.0，已附着元素量 ×0.5 消耗）
            ElemComp->ElementGauge1 -= Result.TriggerGauge * 1.0f;
            ElemComp->ElementGauge2 -= Result.ExistingGauge * 0.5f;
        }
    }
    else
    {
        // 剧变反应：固定消耗 0.5 单位
        ElemComp->ElementGauge1 -= 0.5f;
    }

    // 清理归零的元素
    if (ElemComp->ElementGauge1 <= 0.f)
    {
        ElemComp->AttachedElement1 = FGameplayTag();
        ElemComp->ElementGauge1 = 0.f;
    }
    if (ElemComp->ElementGauge2 <= 0.f)
    {
        ElemComp->AttachedElement2 = FGameplayTag();
        ElemComp->ElementGauge2 = 0.f;
    }
}
```

### 6.4 FPS 特有的元素反应交互

FPS 模式下元素反应有一些独特的交互设计：

| 场景 | 交互设计 |
|------|---------|
| **超载击飞** | 被超载击飞的敌人成为"飞碟"，其他玩家可以射击空中目标获得额外伤害 |
| **冻结 + 爆头** | 冻结状态的敌人头部判定框不移动，更容易爆头 → 鼓励冰角色先冻再切狙击手爆头 |
| **感电传导** | 感电效果会传导到附近的水元素附着目标 → 对成群的敌人特别有效 |
| **扩散传播** | 风元素扩散可以在敌人群中传播火/冰/雷/水 → 鼓励先附着再扩散的 combo |
| **结晶护盾** | 生成的护盾拾取物掉落在地面，需要走过去拾取 → FPS 中需要注意走位 |
| **超导降抗** | 降低物理抗性 → 对无元素武器（物理子弹）特别有效 → 冰+雷 combo 后切物理输出 |

---

## 7. 双角色小队切换系统（Dual-Character Switch）

### 7.1 为什么采用「双角色 + 自由切枪」而不是「4人切人」？

在第三人称动作游戏（如原神）中，4 人轮流切人体验良好是因为有华丽的入场动作、大招特写和全身动画反馈。但在**第一人称 FPS** 中：
- 频繁 4 人快速切换会导致视野和手臂模型高频闪烁，极易产生眩晕与操作割裂感；
- 射击游戏的节奏要求高频瞄准射击，切枪比切人更平滑、更符合 FPS 直觉。

**设计定案：**
1. **基础输出与元素反应：** 主要依靠**单角色自由切枪（1/2/3键快速换火枪/冰枪/雷枪）**在战斗中极速完成。
2. **战术切换与机制搭配：** 采用 **双角色小队（主战 + 副战 / 输出 + 辅助）**，按下 `Tab` 或 `C` 键即时换人。换人伴随专属角色语音、手臂 Mesh 切换、以及调用另一套独特的 E/Q 技能与独立武器配装。

### 7.2 切人组件（Dual Character Switcher）

```cpp
// TeamSwitchComponent.h — 双角色小队管理
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UTeamSwitchComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    // ═══ 双角色小队（最多 2 人）═══
    UPROPERTY(BlueprintReadOnly, Category="DualTeam")
    TArray<AESCharacterBase*> TeamMembers;     // 固定大小 2

    UPROPERTY(BlueprintReadOnly, Category="DualTeam")
    int32 ActiveIndex = 0;                     // 0: 主角色, 1: 副角色

    UPROPERTY(BlueprintReadOnly, Category="DualTeam")
    AESCharacterBase* ActiveCharacter = nullptr;

    // ═══ 切人参数 ═══
    UPROPERTY(EditDefaultsOnly, Category="Switch")
    float SwitchDuration = 0.2f;               // 切人极速过渡时间（0.2s 干净利落）

    UPROPERTY(EditDefaultsOnly, Category="Switch")
    float SwitchCooldown = 2.0f;               // 双人切换内置 CD

    UPROPERTY(EditDefaultsOnly, Category="Switch")
    bool bInheritVelocity = true;              // 继承当前速度向量（奔跑中切人保持冲刺）

    UPROPERTY(EditDefaultsOnly, Category="Switch")
    bool bInheritAimDirection = true;          // 继承视角方向（准星完全不晃）

    // ═══ 核心方法 ═══
    UFUNCTION(BlueprintCallable)
    bool ToggleCharacter();                    // Tab 键一键对调角色

    UFUNCTION(BlueprintCallable)
    bool SwitchTo(int32 TargetIndex);

    UFUNCTION(BlueprintCallable)
    bool CanSwitch() const;

    // ═══ 委托 ═══
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnTeamSwitch,
        AESCharacterBase*, OldChar, AESCharacterBase*, NewChar, int32, NewIndex);
    UPROPERTY(BlueprintAssignable) FOnTeamSwitch OnTeamSwitch;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSwitchCooldown,
        float, Remaining, float, Total);
    UPROPERTY(BlueprintAssignable) FOnSwitchCooldown OnSwitchCooldownTick;

    // Tick 中更新后台角色
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
                               FActorComponentTickFunction* ThisTickFunction) override;

private:
    float SwitchCooldownRemaining = 0.f;
    bool bSwitchOnCooldown = false;

    void PerformSwitch(AESCharacterBase* OutChar, AESCharacterBase* InChar);
    void TickBackgroundCharacter(float DeltaTime);
};
```

### 7.3 第一人称切人流程（FPS 适配核心）

```
玩家按下 Tab 键
    │
    ▼
CanSwitch() 检查
  ├── 切人 CD 是否就绪？（<= 0）
  ├── 队友是否存活？（HP > 0）
  └── 当前是否在不可切人动作中（换弹/受击硬直）？
    │
    ▼（通过）
PerformSwitch(OldChar, NewChar)
    │
    ├── 1. 保存当前状态：
    │     ├── 玩家视角 ControlRotation (Yaw / Pitch)
    │     ├── 角色实时速度向量 Velocity
    │     └── OldChar 当前武器弹药与后坐力状态
    │
    ├── 2. 退场处理：
    │     ├── 停止射击与换弹
    │     ├── OldChar->SetActorHiddenInGame(true)
    │     ├── 禁用碰撞，进入后台轻量更新
    │     └── 播放退场语音（如："交给你了！"）
    │
    ├── 3. 入场处理：
    │     ├── NewChar 移动到 OldChar 坐标并继承速度
    │     ├── 恢复严格相同的 ControlRotation（准星分毫不差）
    │     ├── NewChar->SetActorHiddenInGame(false)
    │     ├── 启用碰撞并激活其专属第一人称手臂 Mesh (FPArms)
    │     ├── 播放入场语音（如："交给我吧！"）+ 屏幕边缘元素光效一闪
    │     └── 触发拔枪动作 (0.2s)
    │
    ├── 4. 控制权交接：
    │     ├── PlayerController->Possess(NewChar)
    │     └── 重新绑定 Enhanced Input 与 HUD（血条、技能组、武器槽）
    │
    └── 5. 广播事件：
          └── OnTeamSwitch.Broadcast(OldChar, NewChar, NewIndex)
```

**FPS 切人手感代码保障（准星零跳动 + 动量继承）：**

```cpp
void UTeamSwitchComponent::PerformSwitch(AESCharacterBase* OutChar, AESCharacterBase* InChar)
{
    APlayerController* PC = Cast<APlayerController>(OutChar->GetController());
    if (!PC) return;

    FRotator SavedControlRot = PC->GetControlRotation(); // 1. 严格锁住玩家当前准星视角
    FVector SavedVelocity = OutChar->GetCharacterMovement()->Velocity; // 2. 锁住当前移动惯性

    // 坐标与朝向继承
    InChar->SetActorLocation(OutChar->GetActorLocation());
    InChar->SetActorRotation(FRotator(0.f, SavedControlRot.Yaw, 0.f));

    if (bInheritVelocity)
    {
        InChar->GetCharacterMovement()->Velocity = SavedVelocity;
    }

    // 后台化旧角色，激活新角色
    OutChar->EnterBackgroundMode();
    InChar->ExitBackgroundMode();

    // 换控制器拥有者
    PC->UnPossess();
    PC->Possess(InChar);

    // 恢复视角（确保第一人称准星完全无跳动感）
    PC->SetControlRotation(SavedControlRot);

    // 触发新角色的武器就位动画与语音
    InChar->PlayWeaponEquipAnimation();
    InChar->PlaySwitchInVoice();

    ActiveCharacter = InChar;
    ActiveIndex = (ActiveIndex == 0) ? 1 : 0;

    bSwitchOnCooldown = true;
    SwitchCooldownRemaining = SwitchCooldown;

    OnTeamSwitch.Broadcast(OutChar, InChar, ActiveIndex);
    UESEventBus::Get(this)->OnCharacterSwitched.Broadcast(
        (ActiveIndex == 1) ? 0 : 1, ActiveIndex);
}
```
    // 在 FPS 中，切人时准星跳动会严重影响手感
    // 必须保证 NewChar 的摄像机朝向 = OldChar 的摄像机朝向

    APlayerController* PC = Cast<APlayerController>(OutChar->GetController());
    FRotator ControlRotation = PC->GetControlRotation();  // 保存当前视角

    // 位置继承
    InChar->SetActorLocation(OutChar->GetActorLocation());
    InChar->SetActorRotation(FRotator(0, ControlRotation.Yaw, 0));  // 只继承 Yaw

    // 速度继承
    if (bInheritVelocity)
    {
        InChar->GetCharacterMovement()->Velocity = OutChar->GetCharacterMovement()->Velocity;
    }

    // 退场
    OutChar->EnterBackgroundMode();

    // 入场
    InChar->ExitBackgroundMode();

    // Possess 新角色
    PC->UnPossess();
    PC->Possess(InChar);

    // ══ 关键：恢复摄像机旋转 ══
    PC->SetControlRotation(ControlRotation);  // 准星不跳

    // 武器掏出动画（期间不能射击，但可以移动和瞄准）
    InChar->PlayWeaponEquipAnimation();

    // 更新引用
    ActiveCharacter = InChar;
    ActiveIndex = TeamMembers.IndexOfByKey(InChar);

    // 开始切人 CD
    bSwitchOnCooldown = true;
    SwitchCooldownRemaining = SwitchCooldown;

    // 广播
    OnTeamSwitch.Broadcast(OutChar, InChar, ActiveIndex);
    UESEventBus::Get(this)->OnCharacterSwitched.Broadcast(
        TeamMembers.IndexOfByKey(OutChar), ActiveIndex);
}
```

### 7.3 后台角色 Tick

后台角色（未被操控的 3 人）需要继续更新部分逻辑：

```cpp
void UTeamSwitchComponent::TickBackgroundCharacters(float DeltaTime)
{
    for (int32 i = 0; i < TeamMembers.Num(); i++)
    {
        if (i == ActiveIndex) continue;  // 跳过当前操控角色

        AESCharacterBase* Char = TeamMembers[i];
        if (!Char || Char->IsDead()) continue;

        // ✅ 继续 Tick 的内容：
        Char->GetSkillComponent()->TickCooldowns(DeltaTime);   // 技能 CD 持续冷却
        Char->GetEnergyComponent()->TickRecharge(DeltaTime);   // 能量自然充能
        Char->GetHealthComponent()->TickRegeneration(DeltaTime); // HP 回复（如有 buff）

        // ❌ 不 Tick 的内容：
        // - 移动 / 碰撞
        // - 动画
        // - 渲染
        // - 输入处理
        // - AI 感知
    }
}
```

---

## 8. 资源与状态系统

### 8.1 生命组件

```cpp
// HealthComponent.h
UCLASS()
class UHealthComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category="Health") float MaxHealth = 10000.f;
    UPROPERTY(BlueprintReadOnly) float CurrentHealth;
    UPROPERTY(BlueprintReadOnly) bool bIsDead = false;

    // ═══ 伤害与治疗 ═══
    UFUNCTION(BlueprintCallable)
    void TakeDamage(float Amount, FGameplayTag DamageElement);

    UFUNCTION(BlueprintCallable)
    void Heal(float Amount);

    // ═══ HP 再生（buff 触发）═══
    UPROPERTY(EditDefaultsOnly) float RegenRate = 0.f;           // 每秒回复量
    UPROPERTY(EditDefaultsOnly) float RegenDelay = 5.0f;         // 受伤后延迟回复
    void TickRegeneration(float DeltaTime);

    // ═══ 委托 ═══
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnHealthChanged,
        float, Current, float, Max, bool, bIsDamage);
    UPROPERTY(BlueprintAssignable) FOnHealthChanged OnHealthChanged;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeath);
    UPROPERTY(BlueprintAssignable) FOnDeath OnDeath;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLowHealth,
        float, Current, float, Max);  // HP < 20% 时触发
    UPROPERTY(BlueprintAssignable) FOnLowHealth OnLowHealth;

private:
    float TimeSinceLastDamage = 0.f;
};
```

### 8.2 护盾组件

```cpp
// ShieldComponent.h — 独立于 HP 的护盾层（结晶反应生成）
UCLASS()
class UShieldComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly) float CurrentShield = 0.f;
    UPROPERTY(BlueprintReadOnly) float MaxShield = 0.f;
    UPROPERTY(BlueprintReadOnly) FGameplayTag ShieldElement;     // 护盾元素（对应元素伤害吸收 250%）

    UFUNCTION(BlueprintCallable)
    void AddShield(float Amount, FGameplayTag Element, float Duration);

    // 返回穿透护盾后的剩余伤害
    UFUNCTION(BlueprintCallable)
    float AbsorbDamage(float Damage, FGameplayTag DamageElement);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnShieldChanged,
        float, Current, float, Max);
    UPROPERTY(BlueprintAssignable) FOnShieldChanged OnShieldChanged;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShieldBroken);
    UPROPERTY(BlueprintAssignable) FOnShieldBroken OnShieldBroken;

private:
    FTimerHandle ShieldDecayHandle;
    void DecayShield();
};
```

**伤害流向：伤害 → 护盾吸收 → 剩余穿透到 HP**

```cpp
void UHealthComponent::TakeDamage(float Amount, FGameplayTag DamageElement)
{
    if (bIsDead) return;

    float RemainingDamage = Amount;

    // 1. 先走护盾
    if (UShieldComponent* Shield = GetOwner()->FindComponentByClass<UShieldComponent>())
    {
        RemainingDamage = Shield->AbsorbDamage(RemainingDamage, DamageElement);
    }

    // 2. 剩余伤害扣 HP
    if (RemainingDamage > 0.f)
    {
        CurrentHealth = FMath::Max(0.f, CurrentHealth - RemainingDamage);
        TimeSinceLastDamage = 0.f;
        OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, true);

        // 低血量警告
        if (CurrentHealth / MaxHealth < 0.2f)
        {
            OnLowHealth.Broadcast(CurrentHealth, MaxHealth);
        }

        // 死亡
        if (CurrentHealth <= 0.f)
        {
            bIsDead = true;
            OnDeath.Broadcast();
            UESEventBus::Get(this)->OnCharacterDeath.Broadcast(
                Cast<AESCharacterBase>(GetOwner()));
        }
    }
}
```

### 8.3 体力组件

```cpp
// StaminaComponent.h
UCLASS()
class UStaminaComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere) float MaxStamina = 240.f;
    UPROPERTY(BlueprintReadOnly) float CurrentStamina = 240.f;
    UPROPERTY(EditAnywhere) float RecoveryRate = 30.f;        // 每秒恢复
    UPROPERTY(EditAnywhere) float RecoveryDelay = 1.5f;       // 停止消耗后延迟恢复
    UPROPERTY(EditAnywhere) float SprintCostPerSecond = 18.f; // 冲刺每秒消耗
    UPROPERTY(EditAnywhere) float DashCost = 18.f;            // 闪避消耗

    UFUNCTION(BlueprintCallable) bool ConsumeStamina(float Amount);
    UFUNCTION(BlueprintCallable) bool HasEnoughStamina(float Amount) const;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStaminaChanged,
        float, Current, float, Max);
    UPROPERTY(BlueprintAssignable) FOnStaminaChanged OnStaminaChanged;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStaminaExhausted);
    UPROPERTY(BlueprintAssignable) FOnStaminaExhausted OnStaminaExhausted;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
                               FActorComponentTickFunction* ThisTickFunction) override;

private:
    float TimeSinceLastConsume = 0.f;
};
```

### 8.4 能量组件（元素充能）

```cpp
// EnergyComponent.h — Q 大招能量系统
UCLASS()
class UEnergyComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere) float MaxEnergy = 60.f;   // Q 技能所需能量
    UPROPERTY(BlueprintReadOnly) float CurrentEnergy = 0.f;
    UPROPERTY(EditAnywhere) float EnergyRechargeRate = 1.0f;  // 能量充能效率倍率

    // 获取能量（击败敌人、触发元素反应、拾取元素微粒）
    UFUNCTION(BlueprintCallable)
    void AddEnergy(float Amount);

    // 消耗能量（释放 Q）
    UFUNCTION(BlueprintCallable)
    bool ConsumeEnergy(float Amount);

    // 被动充能（后台也生效）
    void TickRecharge(float DeltaTime);

    // 同元素微粒加成（原神机制：同元素角色获取同元素微粒时 ×3 能量）
    UFUNCTION(BlueprintCallable)
    void CollectParticle(FGameplayTag ParticleElement, float BaseEnergy);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEnergyChanged,
        float, Current, float, Max);
    UPROPERTY(BlueprintAssignable) FOnEnergyChanged OnEnergyChanged;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEnergyFull);
};

---

## 9. 敌人 AI 系统

### 9.1 AI 架构概览

敌人 AI 使用 UE5 的 **行为树 (Behavior Tree) + AI Perception + EQS (Environment Query System)** 三件套。不同类型敌人共享同一个 AI Controller 基类，通过不同的行为树资产实现差异化行为。

```
┌─────────────────────────────────────────────────┐
│              ESAIController                      │
│  ┌────────────────┐  ┌────────────────────────┐  │
│  │ AI Perception  │  │  Blackboard Component  │  │
│  │ ├─ Sight       │  │  ├─ TargetActor        │  │
│  │ ├─ Hearing     │  │  ├─ LastKnownLocation  │  │
│  │ └─ Damage      │  │  ├─ CoverLocation      │  │
│  └────────┬───────┘  │  ├─ DistanceToTarget   │  │
│           │          │  ├─ SelfElement         │  │
│           ▼          │  └─ AIState             │  │
│  ┌────────────────┐  └────────────────────────┘  │
│  │ Behavior Tree  │                              │
│  │ (per enemy)    │                              │
│  └────────────────┘                              │
└─────────────────────────────────────────────────┘
```

### 9.2 AI 控制器基类

```cpp
// ESAIController.h
UCLASS()
class AESAIController : public AAIController
{
    GENERATED_BODY()
public:
    UPROPERTY(VisibleAnywhere) UAIPerceptionComponent* PerceptionComp;

    UPROPERTY(EditDefaultsOnly, Category="AI|Perception")
    float SightRadius = 2000.f;
    UPROPERTY(EditDefaultsOnly, Category="AI|Perception")
    float SightAngle = 90.f;
    UPROPERTY(EditDefaultsOnly, Category="AI|Perception")
    float HearingRange = 3000.f;        // 枪声感知范围
    UPROPERTY(EditDefaultsOnly, Category="AI|Perception")
    float LoseSightDuration = 5.0f;

    UPROPERTY(EditDefaultsOnly, Category="AI")
    UBehaviorTree* BehaviorTreeAsset;

protected:
    virtual void BeginPlay() override;
    virtual void OnPossess(APawn* InPawn) override;

    UFUNCTION()
    void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
};
```

### 9.3 敌人类型设计

```cpp
// EnemyBase.h
UCLASS()
class AEnemyBase : public ACharacter
{
    GENERATED_BODY()
public:
    UPROPERTY(VisibleAnywhere) UHealthComponent* HealthComp;
    UPROPERTY(VisibleAnywhere) UShieldComponent* ShieldComp;
    UPROPERTY(VisibleAnywhere) UElementComponent* ElementComp;

    UPROPERTY(EditDefaultsOnly) UEnemyDataAsset* EnemyData;
    UPROPERTY(BlueprintReadOnly) TMap<FGameplayTag, float> Resistances;

    UFUNCTION() void OnDeath();
    void SpawnEnergyParticles();

protected:
    virtual void BeginPlay() override;
};
```

**各敌人类型行为差异：**

| 类型 | 武器 | 行为特征 | 特殊机制 |
|------|------|---------|---------|
| **近战兵** | 大剑/爪击 | 直线冲锋，近距离挥砍 | 冲锋前有蓄力动作（可打断） |
| **远程射手** | 步枪 | 保持中距离射击，被逼近会后退 | 使用掩体系统 |
| **狙击手** | 狙击枪 | 远距离站位，瞄准线预警 | 红色激光瞄准线（给玩家反应时间） |
| **盾兵** | 盾牌+单手枪 | 正面架盾缓慢推进 | 正面免疫子弹，需要绕后/元素破盾 |
| **自爆兵** | 炸弹 | 高速冲向玩家，近距离自爆 | 爆炸附带元素伤害 |
| **精英** | 多种 | 综合行为 + 元素技能 | 有 E 技能（元素攻击），更高属性 |
| **Boss** | 多阶段 | 阶段转换 + 多种攻击模式 | 独立行为树，多阶段血量门槛 |

### 9.4 行为树任务节点

```cpp
// BTTask_FindCover.h — 使用 EQS 查询附近掩体
// 条件：掩体能挡住玩家射线 + 距离适中 + 未被其他 AI 占用
UCLASS()
class UBTTask_FindCover : public UBTTaskNode
{
    GENERATED_BODY()
public:
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp,
                                            uint8* NodeMemory) override;
};

// BTTask_FlankPlayer.h — EQS 计算玩家侧翼位置
// 条件：与玩家朝向夹角 > 60° + 有掩体可达
UCLASS()
class UBTTask_FlankPlayer : public UBTTaskNode
{
    GENERATED_BODY()
public:
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp,
                                            uint8* NodeMemory) override;
};

// BTTask_ElementalAttack.h — 精英/Boss 使用元素技能
UCLASS()
class UBTTask_ElementalAttack : public UBTTaskNode
{
    GENERATED_BODY()
public:
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp,
                                            uint8* NodeMemory) override;
};
```

### 9.5 掩体系统

```cpp
// CoverPointComponent.h
UCLASS()
class UCoverPointComponent : public USceneComponent
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere) bool bIsOccupied = false;
    UPROPERTY(EditAnywhere) float CoverHeight = 100.f;
    UPROPERTY(EditAnywhere) ECoverType CoverType;  // Full / Half / Destructible

    UFUNCTION(BlueprintCallable)
    bool IsValidAgainst(FVector ThreatLocation) const;
};
```

### 9.6 Boss 战设计框架

```cpp
// EnemyBoss.h — 多阶段 Boss
UCLASS()
class AEnemyBoss : public AEnemyBase
{
    GENERATED_BODY()
public:
    UPROPERTY(EditDefaultsOnly) TArray<FBossPhase> Phases;
    UPROPERTY(BlueprintReadOnly) int32 CurrentPhaseIndex = 0;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPhaseChange,
        int32, OldPhase, int32, NewPhase);
    UPROPERTY(BlueprintAssignable) FOnPhaseChange OnPhaseChange;

private:
    void CheckPhaseTransition();
    void EnterPhase(int32 PhaseIndex);
};

USTRUCT(BlueprintType)
struct FBossPhase
{
    GENERATED_BODY()
    UPROPERTY(EditDefaultsOnly) float HPThreshold = 0.5f;
    UPROPERTY(EditDefaultsOnly) UBehaviorTree* PhaseBehaviorTree;
    UPROPERTY(EditDefaultsOnly) UAnimMontage* TransitionMontage;
    UPROPERTY(EditDefaultsOnly) float DamageMultiplier = 1.0f;
    UPROPERTY(EditDefaultsOnly) float SpeedMultiplier = 1.0f;
};
```

---

## 10. UI / HUD 系统

### 10.1 HUD 架构

**核心规则：所有 UI 通过 Delegate 驱动更新，严禁在 Tick 中拉取数据。**

```
┌─────────────────────────────── FPS HUD ──────────────────────────────┐
│                                                                       │
│  [1🔥 主战: 凯亚 ████████] [Tab: 换人] [2🧊 辅助: 甘雨 ████████]       │
│                                                                       │
│                         ┌─ KILL信息 ─┐  ← 右上角击杀流                │
│                                                                       │
│                         ╔═══╗                                         │
│                    ╔════╬═╗═╬════╗  ← 动态准星 + 抛物线落点预警       │
│                    ╚════╬═╝═╬════╝                                     │
│                         ╚═══╝                                         │
│                   🔥 飘字 1,234  💧 蒸发! 2,468                        │
│                                                                       │
│  体力 [======]                    E [CD 3.2s]  Q [████] 45/60         │
│                                                                       │
│  [1: 🔥突击步枪 30/150]  [2: 💧战术霰弹 6/30]  [G: 🧊极寒霜雷 x2]     │
└───────────────────────────────────────────────────────────────────────┘
```

### 10.2 准星系统

```cpp
// CrosshairWidget.h
UCLASS()
class UCrosshairWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable) void UpdateSpread(float CurrentSpread);
    UFUNCTION(BlueprintCallable) void ShowHitMarker(bool bHeadshot, bool bKill);
    UFUNCTION(BlueprintCallable) void SetElementColor(FGameplayTag Element);
    UFUNCTION(BlueprintCallable) void EnterScopeMode(UTexture2D* ScopeTexture);
    UFUNCTION(BlueprintCallable) void ExitScopeMode();

private:
    UPROPERTY(meta=(BindWidget)) UImage* TopLine;
    UPROPERTY(meta=(BindWidget)) UImage* BottomLine;
    UPROPERTY(meta=(BindWidget)) UImage* LeftLine;
    UPROPERTY(meta=(BindWidget)) UImage* RightLine;
    UPROPERTY(meta=(BindWidget)) UImage* CenterDot;
    UPROPERTY(meta=(BindWidget)) UImage* HitMarkerImage;
};
```

### 10.3 UI 数据绑定模式（切人与切枪联动）

所有 UI Widget 遵循统一的绑定模式：

```cpp
// 1. 切枪时更新武器槽高亮与弹药 UI
void UESHUD::OnWeaponSwitched(AESWeaponBase* OldWeapon, AESWeaponBase* NewWeapon, int32 NewSlot)
{
    AmmoWidget->BindToWeapon(NewWeapon);
    WeaponSlotWidget->HighlightSlot(NewSlot);
    CrosshairWidget->SetElementColor(NewWeapon->Element);
}

// 2. 切人时重新绑定整套角色 HUD（双人头像、技能组、武器槽）
void UESHUD::OnTeamSwitch(AESCharacterBase* OldChar, AESCharacterBase* NewChar, int32 NewIndex)
{
    SkillCDWidget->BindToSkill(NewChar->GetSkillComponent());
    StaminaBar->BindToStamina(NewChar->GetStaminaComponent());
    TeamPortraitBar->SetActiveIndex(NewIndex);

    // 绑定新角色的武器管理器
    if (auto* WeaponComp = NewChar->GetWeaponComponent())
    {
        OnWeaponSwitched(nullptr, WeaponComp->GetCurrentWeapon(), WeaponComp->CurrentWeaponIndex);
    }
}
```

---

## 11. 场景与关卡管理

### 11.1 世界结构

```
                    ┌─────────────────┐
                    │   枢纽大厅 Hub   │
                    │  (持久关卡)      │
                    └────────┬────────┘
              ┌──────────────┼──────────────┐
        ┌─────▼─────┐ ┌─────▼─────┐ ┌─────▼─────┐
        │ 战斗竞技场 │ │  波次防御  │ │  Boss 战  │
        │ (歼灭模式) │ │ (生存模式) │ │ (阶段战)  │
        └───────────┘ └───────────┘ └───────────┘
```

### 11.2 关卡管理器

```cpp
// ESLevelManager.h
UCLASS()
class UESLevelManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable) void LoadCombatLevel(ULevelDataAsset* LevelData);
    UFUNCTION(BlueprintCallable) void ReturnToHub();
    UFUNCTION(BlueprintCallable) void StartLevel();
    UFUNCTION(BlueprintCallable) void CompleteLevel();
    UFUNCTION(BlueprintCallable) void FailLevel();

    void AsyncLoadLevel(FName LevelName, FOnLevelLoaded OnComplete);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLevelResult,
        bool, bSuccess, FLevelReward, Reward);
    UPROPERTY(BlueprintAssignable) FOnLevelResult OnLevelResult;
};
```

### 11.3 波次系统

```cpp
// WaveSystem.h
UCLASS()
class UWaveSystem : public UActorComponent
{
    GENERATED_BODY()
public:
    UPROPERTY(EditDefaultsOnly) TArray<FWaveConfig> Waves;
    UPROPERTY(BlueprintReadOnly) int32 CurrentWaveIndex = 0;
    UPROPERTY(BlueprintReadOnly) int32 AliveEnemyCount = 0;

    UFUNCTION(BlueprintCallable) void StartNextWave();
    UFUNCTION(BlueprintCallable) bool IsAllWavesCleared() const;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWaveProgress,
        int32, CurrentWave, int32, TotalWaves);
    UPROPERTY(BlueprintAssignable) FOnWaveProgress OnWaveProgress;

private:
    void SpawnWave(const FWaveConfig& Config);
    void OnEnemyDied(AActor* Enemy, AActor* Killer);
};

USTRUCT(BlueprintType)
struct FWaveConfig
{
    GENERATED_BODY()
    UPROPERTY(EditDefaultsOnly) TArray<FEnemySpawnEntry> Enemies;
    UPROPERTY(EditDefaultsOnly) float DelayBeforeWave = 3.0f;
    UPROPERTY(EditDefaultsOnly) bool bBossWave = false;
};

USTRUCT(BlueprintType)
struct FEnemySpawnEntry
{
    GENERATED_BODY()
    UPROPERTY(EditDefaultsOnly) TSubclassOf<AEnemyBase> EnemyClass;
    UPROPERTY(EditDefaultsOnly) int32 Count = 1;
    UPROPERTY(EditDefaultsOnly) FGameplayTag SpawnPointTag;
    UPROPERTY(EditDefaultsOnly) float SpawnDelay = 0.f;
};
```

### 11.4 关卡传送门 & 关卡数据

```cpp
// LevelPortal.h — Hub 中的关卡入口
UCLASS()
class ALevelPortal : public AActor
{
    GENERATED_BODY()
public:
    UPROPERTY(EditInstanceOnly) ULevelDataAsset* TargetLevel;
    UPROPERTY(VisibleAnywhere) UBoxComponent* TriggerBox;
    UPROPERTY(VisibleAnywhere) UWidgetComponent* InfoWidget;
    // 显示：关卡名称、推荐等级、敌人元素预览、奖励预览
};

// LevelDataAsset.h
UCLASS()
class ULevelDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditDefaultsOnly) FGameplayTag LevelID;
    UPROPERTY(EditDefaultsOnly) FText LevelName;
    UPROPERTY(EditDefaultsOnly) FName MapName;
    UPROPERTY(EditDefaultsOnly) int32 RecommendedLevel = 1;
    UPROPERTY(EditDefaultsOnly) TArray<FGameplayTag> EnemyElements;
    UPROPERTY(EditDefaultsOnly) TArray<FWaveConfig> Waves;
    UPROPERTY(EditDefaultsOnly) float TimeLimit = 300.f;
    UPROPERTY(EditDefaultsOnly) FLevelReward Reward;
};
```

---

## 12. 背包与装备系统

### 12.1 背包组件

```cpp
// InventoryComponent.h
UCLASS()
class UInventoryComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UPROPERTY() TArray<FInventorySlot> Slots;
    UPROPERTY(EditAnywhere) int32 MaxSlots = 200;

    UFUNCTION(BlueprintCallable) bool AddItem(FGameplayTag ItemID, int32 Count = 1);
    UFUNCTION(BlueprintCallable) bool RemoveItem(FGameplayTag ItemID, int32 Count = 1);
    UFUNCTION(BlueprintCallable) int32 GetItemCount(FGameplayTag ItemID) const;
    UFUNCTION(BlueprintCallable) TArray<FInventorySlot> GetItemsByType(EItemType Type) const;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryChanged,
        FGameplayTag, ItemID, int32, NewCount);
    UPROPERTY(BlueprintAssignable) FOnInventoryChanged OnInventoryChanged;
};
```

### 12.2 装备系统

每个角色有固定装备槽位：1 武器 + 5 圣遗物（花/羽/沙/杯/冠）。

```cpp
USTRUCT(BlueprintType)
struct FCharacterEquipment
{
    GENERATED_BODY()
    UPROPERTY() FGameplayTag WeaponID;
    UPROPERTY() FGameplayTag ArtifactFlower;     // 生之花
    UPROPERTY() FGameplayTag ArtifactPlume;      // 死之羽
    UPROPERTY() FGameplayTag ArtifactSands;      // 时之沙
    UPROPERTY() FGameplayTag ArtifactGoblet;     // 空之杯
    UPROPERTY() FGameplayTag ArtifactCirclet;    // 理之冠
};
```

---

## 13. 性能优化方案

### 13.1 对象池系统

FPS 每秒产生大量临时对象（子弹特效、飘字），频繁 Spawn/Destroy 会导致 GC 卡顿：

```cpp
// ObjectPoolSubsystem.h
UCLASS()
class UObjectPoolSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    template<typename T>
    T* SpawnFromPool(TSubclassOf<T> Class, const FTransform& Transform);

    template<typename T>
    void ReturnToPool(T* Object);

    UPROPERTY(EditDefaultsOnly) TMap<TSubclassOf<AActor>, int32> PoolSizes;
    // AESProjectile → 100, ADamageNumberWidget → 50, ANiagaraActor → 200

    void WarmupPools();  // 关卡加载时预创建

private:
    TMap<UClass*, TArray<AActor*>> AvailablePool;
    TMap<UClass*, TArray<AActor*>> ActivePool;
};
```

### 13.2 内存优化策略

| 策略 | 实现方式 | 适用场景 |
|------|---------|---------|
| **对象池** | `ObjectPoolSubsystem` | 子弹、特效、飘字、音效 |
| **流式加载** | `ULevelStreamingDynamic` | 关卡切换 |
| **按需加载** | `TSoftObjectPtr` + `AsyncLoad` | 角色蓝图、武器模型 |
| **纹理 Streaming** | UE5 Virtual Texture | 大尺寸纹理 |
| **LOD** | 自动 LOD + HLOD | 远距离模型简化 |

### 13.3 渲染优化策略

| 策略 | 实现方式 | 预期收益 |
|------|---------|---------|
| **遮挡裁剪** | Hardware Occlusion Queries | 减少 30%+ DrawCall |
| **距离裁剪** | `CullDistanceVolume` | 远处小物件不渲染 |
| **实例化渲染** | `UInstancedStaticMeshComponent` | 弹壳、散落物 |
| **粒子池化** | Niagara `AutoRelease` | 粒子系统复用 |
| **阴影优化** | 小怪用 Capsule Shadow | 减少阴影 DrawCall |

### 13.4 CPU 优化

```cpp
// AI Tick 分帧：不是每个 AI 每帧都 Tick
void AESAIController::Tick(float DeltaTime)
{
    FrameCounter++;
    if (FrameCounter % 3 != (AIIndex % 3)) return;  // 交错执行
    Super::Tick(DeltaTime * 3.f);  // 补偿 DeltaTime
}

// 感知系统频率降低
SightConfig->SetMaxAge(0.5f);  // 每 0.5 秒更新一次（而非每帧）
```

---

## 14. 网络架构预留（PvP 扩展）

> **当前版本为纯 PvE 单机，但代码架构预留网络同步扩展点。**

### 14.1 网络同步预留点

| 模块 | 预留设计 | 说明 |
|------|---------|------|
| **移动** | `CharacterMovementComponent` 内置同步 | UE5 默认支持 |
| **射击** | 服务器验证伤害 (`ServerRPC`) | 防止客户端作弊 |
| **元素附着** | `Replicated` 属性标记 | 服务器权威 |
| **血量** | `Replicated` + `OnRep` 回调 | 服务器计算伤害 |
| **切人** | `ServerRPC` → `MulticastRPC` 广播 | 所有客户端同步 |

### 14.2 PvP 元素反应平衡问题

> 元素反应在 PvP 中需要大幅调整——这也是推迟 PvP 的主要原因：
> - **冻结 2s** 对 PvP 致命 → 缩短到 0.5s 或改为减速
> - **超载击飞** 影响操控 → 加入抗性机制
> - **蒸发 ×2.0** 可能一发秒杀 → 伤害上限
> - **感电传导** 对团战影响太大 → 限制传导数量
>
> 解决方案：PvP 模式使用独立的 `PvP_ReactionTable`，与 PvE 表分离。

---

## 15. 代码量预估

| 模块 | 文件数 | 行数估计 | 说明 |
|------|-------|---------|------|
| **Core** | 14 | ~700 | GameInstance / GameMode / Controller / EventBus |
| **Character** | 12 | ~1200 | 3C 系统（摄像机/移动/切人） |
| **Weapon** | 18 | ~1800 | 武器基类 + 5种武器 + 后坐力 |
| **Combat** | 10 | ~1000 | 伤害计算 / 命中反馈 / 技能 |
| **Element** | 20 | ~1500 | 元素附着 / 反应管理 / 7种反应 |
| **Resource** | 8 | ~600 | HP / Shield / Stamina / Energy |
| **AI** | 28 | ~2000 | 控制器 / 5种敌人 / 行为树节点 |
| **UI** | 22 | ~1600 | HUD / 准星 / 飘字 / 头像栏 |
| **Level** | 12 | ~800 | 关卡管理 / 波次 / 传送门 |
| **Inventory** | 8 | ~500 | 背包 / 装备 |
| **Optimization** | 8 | ~500 | 对象池 / LOD |
| **合计** | **~160 个文件** | **~12,200 行 C++** | |

---

## 16. 开发时间线

| 周 | 模块 | 关键产出 | JD 能力对应 |
|----|------|---------|------------|
| 1 | 项目搭建 + 3C 基础 | UE5 项目、Enhanced Input、三模式摄像机、基础移动 | 角色 3C |
| 2 | 3C 完善 + 武器系统 | 冲刺/闪避/滑铲、武器基类、Hitscan 射击、后坐力 | 角色 3C + 战斗逻辑 |
| 3 | 武器类型 + 弹药系统 | 5 种武器实现、换弹、散布系统、弹药管理 | 战斗逻辑 |
| 4 | 战斗系统 + 命中反馈 | 伤害公式、HitMarker、飘字、受击反馈、爆头 | 战斗逻辑 |
| 5 | 元素反应系统 | 元素附着（含 ICD）、反应管理器、7 种反应效果 | 战斗逻辑（核心） |
| 6 | 队伍切换 + 技能系统 | 4 人切换、E/Q 技能、后台 Tick | 角色 3C + 战斗逻辑 |
| 7 | 资源系统 + UI 系统 | HP/Shield/Stamina/Energy + HUD 全套 | UI 系统 |
| 8 | 敌人 AI（基础） | 近战兵/远程射手/狙击手、行为树、感知系统 | AI 系统 |
| 9 | 敌人 AI（进阶） | 盾兵/精英/Boss、掩体系统、EQS 侧翼包抄 | AI 系统 |
| 10 | 场景管理 | Hub 大厅、关卡传送门、波次系统、异步加载 | 场景管理 |
| 11 | 背包装备 + 性能优化 | 背包/装备系统、对象池、内存/渲染优化 | 性能优化 |
| 12 | 联调 + 打磨 | 端到端测试、平衡性调参、Bug 修复、体验打磨 | 全栈 |

---

## 17. 验证方案

### 17.1 模块级验证

| 模块 | 验证内容 | 通过标准 |
|------|---------|---------|
| **3C** | WASD 移动、鼠标视角、FPS/TPS/肩射切换 | 三种视角平滑过渡，无穿模 |
| **射击** | 步枪连射、散布扩张、ADS 收束 | 散布随连射递增，ADS 后收束 |
| **后坐力** | 压枪模式、停火回正 | 后坐力模式固定可学习，停火后准星回正 |
| **武器切换** | 5 种武器手感差异 | 步枪/霰弹/狙击/冲锋/榴弹各有特色 |
| **伤害** | 距离衰减、爆头倍率、防御减免 | 公式输出与预期一致（单元测试） |
| **元素** | 火打水蒸发、冰打雷超导 | 反应正确触发 + ICD 正常工作 |
| **切人** | 按 1/2/3/4 切人 | 准星不跳、速度继承、武器掏出 |
| **AI** | 敌人发现→追击→攻击→死亡 | 状态机正常流转 |
| **UI** | HP 变化→血条更新 | 无 Tick 轮询，纯委托驱动 |
| **性能** | 30 个敌人同屏 | 60fps 稳定（对象池启用） |

### 17.2 端到端验证

1. **完整关卡流程：** Hub 选队伍 → 传送门进入 → 3 波敌人 + Boss → 通关 → 返回 Hub
2. **元素 Combo 验证：** 冰角色冻结 → 切狙击手爆头 → 切火角色蒸发 → DPS 验证
3. **体力管理：** 冲刺消耗体力 → 体力耗尽自动停冲 → 延迟后恢复
4. **全队阵亡：** 4 个角色全部死亡 → 显示失败 UI → 返回 Hub

### 17.3 性能验证指标

| 指标 | 目标值 | 测试场景 |
|------|-------|---------|
| FPS | ≥ 60fps | 30 个敌人同屏 + 元素反应特效 |
| DrawCall | < 3000 | 最复杂关卡场景 |
| 内存占用 | < 4GB | 完整游戏运行 |
| 关卡加载 | < 3s | 异步加载战斗关卡 |
| GC 卡顿 | < 2ms | 对象池启用后 |
| 射击响应延迟 | < 1 帧 | 点击到命中反馈 |

