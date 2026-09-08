# 🚀 Lumi-Spark 项目开发路线图与进度看板 (Project Roadmap)

本文档记录了 **Lumi-Spark** 项目的全局开发规划、实时完成进度、2~4 人联机网络架构方案以及地脉遗迹随机地图生成（PCG）技术方案。

---

## 📊 实时开发进度看板 (Progress Overview)

| 阶段里程碑 | 包含核心模块 | 状态 | 当前完成度 |
| :--- | :--- | :---: | :---: |
| **阶段 1：3C 核心框架与基础层** | `LSCameraComponent`, `LSMovementComponent`, `LSCharacterBase`, `LSPlayerController`, `LSTypes`, `LSEventBus`, `GameMode` | **已完成实机验证** | **100% ✅** |
| **阶段 2：武器与投掷物系统** | `LSWeaponBase`, `LSWeaponComponent`, `LSHitscanTrace`, `LSElementProjectile`(手雷), 后坐力系统 | **准备开始** | 0% |
| **阶段 3：高等元素论与反应引擎** | `LSElementComponent`(1U/2U/4U/ICD), `LSElementReactionManager`, 16 种反应结算 | **待开始** | 0% |
| **阶段 4：双角色即时切换与技能** | `LSTeamSwitchComponent`, `LSSkillComponent`, `LSPlayerState`, 独立 CD 体系 | **待开始** | 0% |
| **阶段 5：2~4人联机合作网络架构** | Listen Server, Server RPC, 属性复制, 客户端预测, 动态怪物缩放, 倒地互救 | **架构规划中** | 0% |
| **阶段 6：地脉遗迹随机地图生成 (PCG)** | 模块化房间拼装 (Dungeon Generator), 地脉紊乱词条, 宝箱与撤离点分布 | **方案规划中** | 0% |
| **阶段 7：敌人 AI、UI 与完整闭环** | 行为树、仇恨表、Boss 机制、伤害飘字、准星 HitMarker、结算面板 | **待开始** | 0% |

---

## 🎯 阶段 1 详细交付清单 (Phase 1 Checklist)

- [x] **摄像机组件 (`LSCameraComponent`)**：第一人称/第三人称/过肩瞄准三模式平滑插值、球体防穿墙检测、FPS 隐藏躯干且保留地面阴影。
- [x] **增强移动组件 (`LSMovementComponent`)**：重写 `GetMaxSpeed()` 统一状态机、FTimerManager 驱动冲刺/闪避无敌帧/真实摩擦力滑铲。
- [x] **角色基类 (`LSCharacterBase`)**：`SetDefaultSubobjectClass` 替换底层移动组件、挂载第一人称手臂、Yaw 水平旋转隔离。
- [x] **原生 GameplayTags 体系 (`LSTypes`)**：编译期 16 种全元素反应、月/星特化机制、草原核/月笼实体标签、1U/2U/4U 量级、伤害上下文结构体。
- [x] **全局事件总线 (`LSEventBus`)**：基于 `UGameInstanceSubsystem` 的跨关卡零耦合发布-订阅中心。
- [x] **玩家控制器 (`LSPlayerController`)**：战斗模式与 UI 菜单模式 IMC 切换、开镜灵敏度 0.6x 平滑衰减、16 项动作通道前置绑定。
- [x] **GameMode 配置 (`Lumi_SparkGameMode`)**：指定默认 Controller 与 DefaultPawn。
- [ ] **【当前下一步】编辑器内实机测试**：配置 Enhanced Input 资产，挂载小白人模型，在关卡中试跑 3C 手感！

---

## 🌐 阶段 5 扩展：2~4人联机合作架构规划 (Multiplayer Co-op)

### 1. 网络角色与权威划分 (Network Authority)
- **Server 权威端 (Authority)**：
  - 伤害计算、暴击判定、元素量附着衰减（1U/2U/4U）、反应触发、怪物仇恨与刷新、关卡撤离判定。
- **Client 表现端 (Autonomous / Simulated Proxy)**：
  - 本地瞬时枪口火光、后坐力抬枪手感、客户端移动预测（Client Prediction）。
  - 通过 `Server RPC` 发送开火与技能请求；通过 `NetMulticast` / `Client RPC` 接收反应大招特效与受击跳字。

### 2. 动态怪物数值与机制缩放 (Dynamic Scaling)
- **血量/护盾系数**：
  - 1人：$100\%$ | 2人：$160\%$ | 3人：$230\%$ | 4人：$300\%$
- **复合元素破盾机制**：
  - 精英怪拥有双层元素盾（如冰甲包裹雷核），鼓励队友之间分别用火与草进行连携破盾。
- **倒地互救机制 (Downed & Revive)**：
  - 玩家生命归 0 进入 `State.Downed` 倒地状态，队友按住 F 键 3 秒拉起；全员倒地判定 Raid Wipe 撤离失败。
- **多目标仇恨积分表 (Threat Table)**：
  - 根据各玩家单位时间内的反应伤害量、救援动作动态切换仇恨目标。

---

## 🗺️ 阶段 6 扩展：地脉遗迹随机地图生成方案 (PCG Dungeon)

结合“地脉肉鸽搜打撤（Roguelite Extraction）”玩法，地图采用**模块化房间拼接（Modular Dungeon Graph）**：

```text
       [起点入口 / 整备区]
               │
       ┌───────┴───────┐
       ▼               ▼
  [战斗房间 A]    [地脉宝箱房 (水)]
  (冰史莱姆群)         │
       │       ┌───────┘
       ▼       ▼
    [精英战斗房 B (火雷复合盾)]
               │
       ┌───────┴───────┐
       ▼               ▼
[传奇圣遗物藏宝库]   [Boss 竞技场]
       │               │
       └───────┬───────┘
               ▼
       [地脉紊乱撤离传送门]
```

### 1. 核心技术选型
- **Level Streaming Instance (关卡流式动态实例化)**：
  - 预先制作 10~15 个不同类型的地脉房间模块（战斗房、机关房、宝箱房、陷阱房、Boss房）。
  - 在运行时通过 C++ 图算法（MST 最小生成树 + Delaunay 三角剖分）随机拼接走廊与房间。
- **随机地脉紊乱环境词条 (Ley Line Disorders)**：
  - 每次进入遗迹随机附加全局词条（如：“全场雷暴降临，感电伤害 +150%”、“极寒领域，冲刺体力消耗增加但暴击率 +20%”）。
- **动态战利品与撤离点分布**：
  - 房间深处的宝箱根据反应挑战评分掉落不同阶级的圣遗物与武器配件；
  - 倒计时结束前随机激活 1~2 个撤离传送门，制造撤离博弈点。

---

## 📅 下一步行动指引 (Next Action)

**当前最佳节奏**：
1. **先进入 UE 编辑器**，按照我们梳理的 5 个步骤完成第一阶段的实机验证（体验第一/第三人称切换、开镜、冲刺、滑铲、闪避）。
2. 实机手感调优完成后，正式启动 **阶段 2：武器与抛物线元素手雷系统** 的 C++ 编写！
