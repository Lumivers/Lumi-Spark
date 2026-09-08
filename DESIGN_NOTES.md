# Lumi-Spark 项目架构与系统设计笔记 (DESIGN_NOTES.md)

> 本文档用于集中沉淀 Lumi-Spark（虚幻引擎 5.4 C++）所有底层核心系统、物理手感参数、战斗计算公式与高等元素论架构设计。
> **维护规则**：纯增量记录，新模块统一在文末末尾追加，严禁全量覆盖。

---

## 目录
1. [已完成核心框架与 3C 架构概述](#1-已完成核心框架与-3c-架构概述)
2. [模块一：元素物理投掷物系统 (ALSGrenadeBase)](#2-模块一元素物理投掷物系统-alsgrenadebase)
3. [模块进阶：草雷与风雷的设计价值与机制扩展](#3-模块进阶草雷与风雷的设计价值与机制扩展)
4. [后续待推进 C++ 模块路线图](#4-后续待推进-c-模块路线图)

---

## 1. 已完成核心框架与 3C 架构概述

### 1.1 移动与相机系统
- **LSCameraComponent**：
  - 纳管 FPS（第一人称机瞄）、TPS（第三人称越肩）、ADS（精确瞄准）三视角平滑插值切换。
  - 球体射线检测防穿墙（Camera Collision Sweep），在狭窄地形自动拉近 ArmLength。
- **LSMovementComponent**：
  - 基于状态机集中管控：走/跑/冲刺 (Sprinting)/滑铲 (Sliding)/闪避 (Dashing)。
  - 闪避赋予 `TAG_State_Invincible` 无敌帧标签，重写 `GetMaxSpeed()` 实现动态变速。
- **LSCharacterBase**：
  - 装配相机、移动、双武器插槽与第一人称独立手臂网格体（FPArmsMesh）。

### 1.2 武器与射击中枢
- **LSWeaponBase**：
  - 双段视差矫正 Hitscan（相机向前射线确定落点 -> 枪口向落点发射主射线，彻底解决近距离准星视差）。
  - 线性距离伤害衰减、`head` 骨骼爆头倍率、每发散布累加与帧间恢复。
- **LSWeaponComponent**：
  - 主武器 (`MainWeapon`)、副武器 (`SubWeapon`)、投掷物 (`Throwable`) 槽位纳管。
  - 骨骼插槽动态挂载与切枪委托广播 `OnWeaponChanged`。
- **LSRecoilComponent**：
  - 程序化弹道模式（Pattern Recoil）与按需 Tick 视角平滑回正。

### 1.3 核心框架与解耦通信
- **LSTypes**：Native GameplayTags 统一命名空间 `LSTags`（7大元素 + 16种反应 + 状态标签）与统一伤害上下文结构体 `FLSDamageContext`。
- **LSEventBus (`ULSEventBus`)**：
  - 继承自 `UGameInstanceSubsystem`，全局静态单例获取接口 `ULSEventBus::Get(WorldContext)`。
  - 战斗事件流 `OnDamageDealt`、`OnEnemyKilled`；元素事件流 `OnElementApplied`、`OnElementReactionTriggered`。
- **ULSHUDWidget**：C++ HUD 中枢基类，监听事件总线触发准星动态扩散与受击跳字反馈。

---

## 2. 模块一：元素物理投掷物系统 (ALSGrenadeBase)

### 2.1 架构定位与职责
- **源码文件**：`Source/Lumi_Spark/Weapon/LSGrenadeBase.{h,cpp}`
- **武器槽位**：对应 `ELSWeaponSlot::Throwable`。
- **战术定位**：全队【高额 AOE 物理爆破】与【高等元素论大范围先手铺场】。

### 2.2 物理手感与弹道关键参数
| 参数项 | 设定值 | 底层原理解析 |
| :--- | :--- | :--- |
| **InitialSpeed** | `1600.0f` | 平抛与高抛手感适中，适配掩体射击作战距离。 |
| **Bounciness** | `0.35f` | 适度弹性，触地弹跳 1~2 次后快速稳定贴地，杜绝橡胶球无限反弹。 |
| **Friction** | `0.6f` | 地面滚动阻尼，落地后不易滑动滑出预期轰炸区。 |
| **StopThreshold** | `40.0f` | 低于 40cm/s 强制终止物理模拟，大幅节省 Chaos 物理开销。 |

### 2.3 掩体防穿墙算法 (Line of Sight Raycast)
- 废弃单纯重叠半径伤害（避免隔墙炸死怪物的虚幻常见 Bug）。
- 在 `OverlapMultiByObjectType` 抓取候选 Actor 后，从手雷中心向目标质心发射 `ECC_Visibility` 视线射线，只有连线通畅（绿色 Debug 线）才施加伤害与元素；被几何掩体遮挡（红色 Debug 线）则完全阻隔。

### 2.4 伤害与元素集成
- **伤害类型**：`LSTags::TAG_Damage_Type_Explosion`。
- **元素附着量级**：手雷全系标配 `ELSElementGauge::Heavy (2U)`（衰减期长达 12s），提供极高反应底元素存量，后续可供多发即时子弹连续触发增幅/剧变反应。
- **解耦广播**：统一由 `ULSEventBus` 派发 `OnDamageDealt` 与 `OnElementApplied`。

---

## 3. 模块进阶：草雷与风雷的设计价值与机制扩展

### 3.1 草雷 (ALSGrenade_Dendro) 的核心价值
- **战术定位**：草系种子生态引擎、高额剧变反应铺场源。
- **原理解析**：
  - 枪械多为点射/连发 Hitscan，若只靠枪械单发点射挂草，面对成群杂兵效率极低。
  - 草雷赋予 **2U 强草**，与后续水系枪械扫射配合，可瞬间大面积催生 3~5 颗草原核（`TAG_Entity_DendroCore`）。
  - 随后切雷枪点爆触发**超绽放**（自动索敌导弹），或切火枪触发**烈绽放**（二次范围爆轰）。
  - 对单兵精英怪则可形成原激化（Quicken）底，让雷/草系射击输出呈指数级飙升。

### 3.2 风雷 (ALSGrenade_Anemo) 的核心价值
- **战术定位**：向心引力涡流控场（Black Hole Vortex）、全场群体元素扩散（Swirl）。
- **原理解析**：
  - 传统手雷均为向外击退（Outward Impulse），容易将怪物炸散，反而增加枪械瞄准难度。
  - **风雷引入负冲量向心吸力 (`bInwardPull = true`)**：爆炸瞬间产生逆向冲量矢量，将波及范围内的杂兵强制向中心聚拢，并微量浮空。
  - 触发**扩散反应（Swirl）**：若吸附的怪物中已有火/水/雷/冰附着，风爆会将该元素强行溅射给被吸进来的所有敌人，实现全屏多段反应连锁。

---

## 4. 后续待推进 C++ 模块路线图

1. **战斗伤害计算与抗性公式 (`LSDamageCalculator`)**：[当前推进]
   - 纯 C++ 静态数学库，纳管：基础倍率区、攻击/防御力区（等级减伤）、双暴区、元素增幅区（融化 2.0x/1.5x、蒸发 2.0x/1.5x）、元素精通增伤区、抗性穿透区（40% 超导物理减抗）。
2. **高等元素论附着与反应引擎 (`LSElementComponent`)**：[下一阶段]
   - 元素附着衰减公式（`1U/2U/4U` 真实消耗与剩余衰减计时器）。
   - 16 种元素反应状态机、元素共存规则（水雷共存感电、冰草共存冻结草核）与草种子实体生命周期。

---

## 4. 模块二：战斗伤害计算与护甲抗性公式 (LSDamageCalculator)

### 4.1 架构定位与设计哲学
- **源码文件**：`Source/Lumi_Spark/Combat/LSDamageCalculator.{h,cpp}`
- **技术选型**：继承自 `UBlueprintFunctionLibrary` 的纯静态无状态数学引擎（Stateless Pure C++ Formula Engine）。
- **设计优势**：
  - 零实例化与常驻内存开销，枪械 (`LSWeaponBase`)、手雷 (`LSGrenadeBase`)、元素反应 (`LSElementComponent`) 均可使用 `ULSDamageCalculator::CalculateDamage(...)` 单行完成全局计算。
  - 数据与算法解耦：通过 `FLSAttackerStats` 与 `FLSDefenderStats` 承载攻击方与防守方的等级、双暴、精通与抗性，计算完成后自动回填 `FLSDamageContext`。

### 4.2 六大核心乘区数学模型
$$\text{FinalDamage} = \text{BaseDamage} \times (1 + \text{DmgBonus}) \times \text{CritMultiplier} \times \text{ReactionMultiplier} \times \text{DefFactor} \times \text{ResFactor}$$

| 乘区 | 计算公式 / 逻辑 | 典型数值特性 |
| :--- | :--- | :--- |
| **1. 基础与增伤区** | $\text{BaseDamage} \times (1 + \text{DamageBonus})$ | 枪械面板或手雷标称伤害，乘以对应属性伤害加成杯。 |
| **2. 双暴区 (Crit)** | 命中弱点 (Headshot) 必暴；或随机判定 $\text{Rand} < \text{CritRate}$，暴击伤害为 $(1 + \text{CritDamage})$ | 50% 暴击率 / 100% 暴伤时，平均期望输出倍率为 1.5x。 |
| **3. 增幅反应区 (Amp)** | 顺向 (水打火/火打冰) 2.0x，反向 (火打水/冰打火) 1.5x；受精通加成 $1 + \frac{2.78 \times \text{EM}}{\text{EM} + 1400}$ | 精通对增幅反应为边际收益递减曲线。 |
| **4. 防御力区 (Def)** | $\frac{\text{AtkLv} + 100}{(\text{AtkLv} + 100) + (\text{DefLv} + 100) \times (1 - \text{DefShred}) \times (1 - \text{DefIgnore})}$ | 同级对决未减防时标准承伤比为 50%；等级压制会影响承伤。 |
| **5. 抗性区 (Res)** | 分段函数：<br>• $\text{Res} < 0$: $1 - \frac{\text{Res}}{2}$<br>• $0 \le \text{Res} < 0.75$: $1 - \text{Res}$<br>• $\text{Res} \ge 0.75$: $\frac{1}{1 + 4 \times \text{Res}}$ | 负抗性收益折半（超导 -40% 物理抗性若将抗性打至负值，实际增益平滑过渡）。 |
| **6. 剧变反应区 (Trans)** | $\text{LevelBaseDamage} \times \text{ReactionCoeff} \times (1 + \frac{16 \times \text{EM}}{\text{EM} + 2000}) \times \text{ResFactor}$ | **完全无视敌方防御力 (Def)**，仅由角色等级基数、剧变倍率与目标元素抗性决定。 |

