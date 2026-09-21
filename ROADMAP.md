# 🚀 Lumi-Spark 项目开发路线图与进度看板 (Project Roadmap)

本文档记录了 **Lumi-Spark** 项目的全局开发规划与实时完成进度。
对照基准：[Lumi-Spark-design.md](Lumi-Spark-design.md) 全 17 章 · 3395 行，设计文档中每一个系统全部列入。

---

## 📊 实时开发进度看板 (Progress Overview)

| 阶段里程碑 | 包含核心模块 | 状态 | 完成度 |
| :--- | :--- | :---: | :---: |
| **阶段 1：3C 核心框架与基础层** | Camera, Movement, Character, Tags, EventBus, Controller, GameMode | **已完成** | **100% ✅** |
| **阶段 2：武器与投掷物 + 网络化** | WeaponBase, WeaponComponent, Recoil, GrenadeBase, DamageCalc, HUD | **已完成** | **100% ✅** |
| **阶段 3：高等元素论与反应引擎** | ElementComponent, 16种反应, DendroCore, ICD, DamagePopWidget | **已完成** | **100% ✅** |
| **阶段 4：三人小队切换与技能** | TeamSwitchComponent, SkillComponent, TargetDummy, 阵亡顺切 | **已完成** | **100% ✅** |
| **阶段 5：2~4人联机合作架构** | GameState, 动态缩放, 破盾, 仇恨表, 倒地互救, 交互接口 | **已完成** | **100% ✅** |
| **阶段 6：数据资产化与资源组件解耦** | DataAsset 体系, Health/Stamina/Energy 组件, 武器派生类, 投掷管理器 | 待开始 | 0% |
| **阶段 7：敌人 AI 与行为树系统** | AIController, EnemyBase, 7类敌人, 掩体, BT 节点, Boss | 待开始 | 0% |
| **阶段 8：驱动核心与装备系统** | 6 槽驱动核心, 驱动盘, 套装判定, 属性汇总管线 | 待开始 | 0% |
| **阶段 9：搜打撤循环与 Meta-Tree** | 侵蚀系统, 背包, 安全箱, 跑尸, 异体刃, 撤离, 天赋树, 经济 | 待开始 | 0% |
| **阶段 10：完整 UI/HUD 与打击反馈** | 准星, 头像栏, 击杀流, 敌人血条, 体力条, HitFeedback, 装备UI | 待开始 | 0% |
| **阶段 11：战术关卡与动态物资生态** | LevelManager, WaveSystem, 宝箱, 传送门, 地脉紊乱词条, NavMesh | 待开始 | 0% |
| **阶段 12：性能优化与最终打磨** | 对象池, AI 分帧, LOD/裁剪, 音频, 任务系统, 压测验证 | 待开始 | 0% |

---

## 🎯 阶段 1 详细交付清单 (Phase 1 — 100% ✅)

- [x] **摄像机组件 (`LSCameraComponent`)**：第一人称/第三人称/过肩瞄准三模式平滑插值、球体防穿墙检测、FPS 隐藏躯干且保留地面阴影。
- [x] **增强移动组件 (`LSMovementComponent`)**：重写 `GetMaxSpeed()` 统一状态机、FTimerManager 驱动冲刺/闪避无敌帧/真实摩擦力滑铲。
- [x] **角色基类 (`LSCharacterBase`)**：`SetDefaultSubobjectClass` 替换底层移动组件、挂载第一人称手臂、Yaw 水平旋转隔离。
- [x] **原生 GameplayTags 体系 (`LSTypes`)**：编译期 16 种全元素反应、月/星特化机制、草原核/月笼实体标签、1U/2U/4U 量级、伤害上下文结构体。
- [x] **全局事件总线 (`LSEventBus`)**：基于 `UGameInstanceSubsystem` 的跨关卡零耦合发布-订阅中心。
- [x] **玩家控制器 (`LSPlayerController`)**：战斗模式与 UI 菜单模式 IMC 切换、开镜灵敏度 0.6x 平滑衰减、16 项动作通道前置绑定。
- [x] **GameMode 配置 (`Lumi_SparkGameMode`)**：指定默认 Controller 与 DefaultPawn。

---

## 🎯 阶段 2 详细交付清单 (Phase 2 — 100% ✅)

- [x] **武器基类与射击 (`LSWeaponBase`)**：双段视差矫正 Hitscan 射线、线性距离衰减、爆头弱点判定、弹药扣除与权威同步、动态准星散布比率。
- [x] **双武器槽位管理器 (`LSWeaponComponent`)**：主副武器槽位、手部 (`HandSocket`) 与背部 (`HolsterSocket`) 挂载、快速切枪、切枪广播委托。
- [x] **程序化后坐力系统 (`LSRecoilComponent`)**：模式弹道 (Pattern Recoil)、随机扰动、按需使能 Tick 的平滑视角回正机制。
- [x] **元素物理投掷物系统 (`ALSGrenadeBase`)**：抛物线弹跳、防穿墙视线遮挡检测、径向伤害衰减、向心吸附黑洞机制、六大元素手雷派生类。
- [x] **战斗伤害计算器 (`LSDamageCalculator`)**：纯静态七乘区无状态数学库、暴击率/暴伤、等级防御减免、抗性分段函数、剧变反应基数拟合。
- [x] **网络联机前置集成**：武器与手雷 `bReplicates` 属性复制、`OnRep_CurrentWeapon` 多端同步、`Server_Fire` 客户端预测与服务端裁决、`Client_HitConfirm` 命中确认闭环。
- [x] **战斗 HUD 中枢 (`LSHUDWidget`)**：监听武器切换与弹药广播、零耦合监听总线命中事件触发 HitMarker、暴露动态准星散布比例。

---

## 🎯 阶段 3 详细交付清单 (Phase 3 — 100% ✅)

- [x] **元素附着与衰减组件 (`LSElementComponent`)**：附着池管理、1U/2U/4U 线性衰减、按需开启 Tick 休眠。
- [x] **16 种元素反应状态机与消耗矩阵**：增幅(蒸发/融化)、剧变(超载/超导/感电/冻结/碎冰)、扩散、结晶。
- [x] **草系生态与草原核 (`ALSDendroCore`)**：原绽放催生刚体种子、烈绽放火系引爆、超绽放雷系追踪飞弹。
- [x] **ICD 附着内置冷却**：2.5 秒 / 3 次命中高射速节流。
- [x] **战斗伤害跳字 (`ULSDamagePopWidget`)**：7 大元素色相、暴击放大感叹号、反应名称浮标与随机径向喷泉散布。

---

## 🎯 阶段 4 详细交付清单 (Phase 4 — 100% ✅)

- [x] **三人小队即时轮换中枢 (`ULSTeamSwitchComponent`)**：顺切、逆切与 1/2/3 直切、视角绝对锁死、移动速度无缝继承、后台休眠状态机。
- [x] **小队自动拉起与在场阵亡顺切**：控制器 `OnPossess` 权威拉起待命队友、阵亡自动顺切救场、全员覆灭锁定输入。
- [x] **E/Q 战术技能与后台独立 CD/充能 (`ULSSkillComponent`)**：独立走表、后台静默流逝 CD 与充能。
- [x] **战斗 HUD 动态重绑 (`ULSHUDWidget`)**：切人时解绑旧角色委托并订阅新角色、主动推流策略。
- [x] **实机打靶木桩 (`ALSTargetDummy`)**：双层碰撞弱点、16 种元素反应验证。

---

## 🎯 阶段 5 详细交付清单 (Phase 5 — 100% ✅)

- [x] **Listen Server 动态怪物数值缩放 (`ALSGameState` & `ALumi_SparkGameMode`)**：1~4人血量护盾动态缩放系数。
- [x] **通用长按交互接口 (`ILSInteractableInterface`)**：瞬时/蓄力双分流模型。
- [x] **复合多层元素破盾 (`ULSShieldComponent`)**：元素克制 2.0x 破盾、溢出伤害折算、破盾瘫痪大硬直。
- [x] **多目标动态仇恨积分表 (`ULSThreatComponent`)**：伤害与反应加权、救援激增 300 点/秒。
- [x] **倒地匍匐与拉起互救**：45s 流血倒计时、长按 [F] 3 秒拉起、距离超出自动打断。
- [x] **HUD 交互推流与团灭表现**：交互浮窗、长按进度条、倒地红屏、Raid Wipe 通告。

---

## 🔶 阶段 6：数据资产化与资源组件解耦

> **目标**：将硬编码数值全部抽离为 DataAsset 驱动，解耦资源组件，派生具体武器子类。
> **对应设计文档**：§4 武器系统、§5 战斗系统、§8 资源与状态系统

### 6.1 数据资产体系
- [ ] **武器数据资产 (`ULSWeaponDataAsset`)**：射速/伤害/散布/后坐力曲线/弹匣/音效粒子软引用
- [ ] **角色数据资产 (`ULSCharacterDataAsset`)**：角色基础属性、待命蓝图引用、E/Q 技能配置
- [ ] **技能数据资产 (`ULSSkillDataAsset`)**：技能倍率/CD/能量消耗/施法蒙太奇/粒子

### 6.2 独立资源组件（从 Character 解耦）
- [ ] **独立生命组件 (`ULSHealthComponent`)**：脱战 5s 延迟回血、低血 20% 告警委托 `OnLowHealth`
- [ ] **体力组件 (`ULSStaminaComponent`)**：240 上限、冲刺 18/s、闪避 18/次、1.5s 延迟回体 30/s、耗尽停止冲刺
- [ ] **元素能量组件 (`ULSEnergyComponent`)**：60 点 Q 能量池、同色微粒 3.0x 加成、后台被动微量充能

### 6.3 武器特化派生类（5 种）
- [ ] **步枪 (`ALSRifle`)**：全自动/半自动模式切换、标准弹道
- [ ] **冲锋枪 (`ALSSMG`)**：高射速低伤害、近距优势散布曲线
- [ ] **霰弹枪 (`ALSShotgun`)**：8 弹丸独立锥形散布与独立射线检测
- [ ] **狙击枪 (`ALSSniperRifle`)**：蓄力增伤、3.0x 爆头倍率、开镜极窄 FOV
- [ ] **榴弹发射器 (`ALSLauncher`)**：抛物线投射物实体、碰触/延时爆炸

### 6.4 投掷物管理器增强
- [ ] **投掷管理组件 (`ULSThrowableComponent`)**：手雷数量管理、按住 G 实时抛物线预测、Spline 落点指示器
- [ ] **手雷残留元素领域 (`SpawnResidualField`)**：爆炸后地面 3.5s 元素领域（火海 DoT / 水雾湿润 / 冰冻地面）
- [ ] **手雷物理击退冲击**：对未霸体敌人施加 `AddImpulse` 微挑击飞

---

## 🔶 阶段 7：敌人 AI 与行为树系统

> **目标**：让游戏从"打靶场"变成"真游戏"，实现有战术深度的 AI 对手。
> **对应设计文档**：§9 敌人 AI 系统

### 7.1 AI 基础框架
- [ ] **AI 控制器基类 (`ALSAIController`)**：`UAIPerceptionComponent` 视觉 2000cm/90° + 听觉 3000cm、丢失视野 5s 倒计时
- [ ] **敌人基类 (`ALSEnemyBase`)**：血量/抗性表/等级/微粒掉落、挂载 ElementComponent + ShieldComponent + ThreatComponent
- [ ] **敌人数据资产 (`ULSEnemyDataAsset`)**：基础属性、行为树引用、掉落表

### 7.2 掩体与战术寻路
- [ ] **掩体感知组件 (`ULSCoverPointComponent`)**：全高/半高/可破坏掩体标记、威胁朝向遮挡计算
- [ ] **BT 任务：寻找掩体 (`UBTTask_FindCover`)**：EQS 查询最优掩体点
- [ ] **BT 任务：侧翼包抄 (`UBTTask_FlankPlayer`)**：EQS 侧翼查询，夹角 >60°
- [ ] **BT 任务：元素攻击 (`UBTTask_ElementalAttack`)**：精英怪释放元素技能
- [ ] **BT 装饰器：元素检查 (`UBTDecorator_CheckElement`)**：检测目标元素附着状态

### 7.3 七类敌人行为特征
- [ ] **近战杂兵 (`ALSEnemy_Melee`)**：直线冲锋蓄力、近距离挥砍打断
- [ ] **远程步枪兵 (`ALSEnemy_Ranged`)**：掩体对射、被逼近时战术后撤
- [ ] **红外狙击手 (`ALSEnemy_Sniper`)**：远距狙击、红色瞄准激光预警线
- [ ] **防弹盾兵 (`ALSEnemy_Shielder`)**：正面大盾免疫子弹，逼迫绕后/元素破盾
- [ ] **自爆兵 (`ALSEnemy_Bomber`)**：高移速冲刺、近身自爆附带元素爆炸
- [ ] **元素精英怪 (`ALSEnemy_Elite`)**：综合行为、携带复合盾、释放元素技能
- [ ] **多阶段 Boss (`ALSEnemyBoss` + `FLSBossPhase`)**：血量门槛转场、转场无敌蒙太奇、独立行为树、狂暴加成

---

## 🔶 阶段 8：驱动核心与装备系统

> **目标**：实现"换人不换装"的全队共享装备体系，构建 Build 配装的核心驱动力。
> **对应设计文档**：§12 驱动核心、局外研发与搜打撤装备系统

### 8.1 驱动核心数据结构
- [ ] **驱动盘结构体 (`FLSDriveDisc`)**：主词条（固定）+ 4 条随机副词条 + 品质等级（绿/蓝/紫/金）
- [ ] **6 槽位装载 (`FLSDriveCoreLoadout`)**：1~3 号位基础攻防、4~6 号位流派特化
- [ ] **驱动盘数据资产 (`ULSDriveDiscDataAsset`)**：套装词条池、属性区间、掉落权重

### 8.2 套装系统
- [ ] **套装激活判定**：4+2 件套或 2+2+2 件套激活效果
- [ ] **套装效果引擎**：双暴增幅、精通反应加成、元素伤害提升、充能/缩短 ICD 等

### 8.3 属性计算管线
- [ ] **角色最终属性汇总**：基础属性 + 武器加成 + 驱动核心 6 槽叠加 → 输入 `LSDamageCalculator`

---

## 🔶 阶段 9：搜打撤循环与 Meta-Tree 养成

> **目标**：实现完整的"局内搜打撤 + 局外长线养成"闭环。
> **对应设计文档**：§1 核心玩法循环、§12 搜打撤系统

### 9.1 局内搜打撤机制
- [ ] **地脉侵蚀系统 (`ULSCorrosionComponent`)**：局内侵蚀计量表随时间上涨、抗侵蚀过滤器消耗滤芯
- [ ] **异体刃背后处决**：潜行至敌人背后 [F] 处决，削 75% 血量并破盾
- [ ] **战利品背包组件 (`ULSBackpackComponent`)**：16 格可扩容至 24 格（Meta-Tree 解锁）
- [ ] **安全箱组件**：2×2 格保底存储（可升 2×3），生死 100% 带回
- [ ] **撤离传送门 (`ALSExtractionPortal`)**：5s 防守倒计时、区域内全员确认撤离

### 9.2 死亡惩罚与跑尸
- [ ] **梯度惩罚系统**：成功微磨损 / 普通失败掉落+安全箱保底 / 高危全丢但掉率翻倍
- [ ] **跑尸机制 (`ALSCorpseMarker`)**：阵亡位置生成遗物标记，下局可前往捡回

### 9.3 局外 Meta-Tree 科技树
- [ ] **Meta-Tree 子系统 (`ULSMetaTreeSubsystem`)**：碎矿/零件废料永久点亮节点
- [ ] **装备分支**：异体刃强化（处决 → 75% 削血、隐身 2s）
- [ ] **生存分支**：开局 200 能量护盾、侵蚀减缓 25%、自带解毒针
- [ ] **搜刮分支**：背包扩容 16→24 格、安全箱 2×2→2×3、跑尸保留率提升

### 9.4 经济与掉落
- [ ] **黑市商店**：出售 T-1 紫装兜底
- [ ] **四大"大红物品"**：36 格军工大背包蓝图、安全箱扩充密钥、原初重构洗词条晶片、传说级原型枪
- [ ] **局内三选一祝福**：通关波次弹出肉鸽三选一，智能加权队伍元素流派

---

## 🔶 阶段 10：完整 UI/HUD 与打击反馈

> **目标**：从"能看到数字"进化为"爽快的战斗手感"，补全所有 UI 子组件与装备界面。
> **对应设计文档**：§5 命中反馈、§10 UI/HUD 系统

### 10.1 打击反馈中枢
- [ ] **命中反馈组件 (`ULSHitFeedbackComponent`)**：
  - 音效分流：普通命中 "叮" / 爆头 "铛" / 击杀确认
  - 受击材质闪白：动态材质 `HitFlash` 参数 0.1s 闪烁
  - 子弹击退力 (`KnockbackForce`)
  - 客户端开火微震：`ClientStartCameraShake`

### 10.2 专用 UI 子组件
- [ ] **动态准星 (`ULSCrosshairWidget`)**：4 方向散布刻度随射击/移动张合、武器元素变色、狙击开镜黑边遮罩
- [ ] **三人小队头像栏 (`ULSTeamPortraitBar`)**：前台角色高亮、后台待命血条/元素/大招就绪光圈
- [ ] **击杀信息流 (`ULSKillFeedWidget`)**：右上角战术击杀滚动流
- [ ] **敌人头顶血条 (`ULSEnemyHealthBarWidget`)**：世界空间 3D 血条 + 元素附着图标
- [ ] **体力条 (`ULSStaminaBarWidget`)**：弧形或条状体力 UI、耗尽闪烁告警

### 10.3 装备与养成 UI
- [ ] **驱动核心装配面板 (`WBP_DriveCore`)**：6 槽拖拽装配、词条预览、套装高亮
- [ ] **驱动盘详情面板**：主副词条展示、强化升级、洗词条交互
- [ ] **Meta-Tree 界面 (`WBP_MetaTree`)**：科技树可视化
- [ ] **撤离结算面板 (`WBP_ExtractionResult`)**：战损报告、获取物品、未鉴定物开盲盒
- [ ] **三选一祝福弹窗 (`WBP_BlessingSelection`)**

### 10.4 现有 UI 增强
- [ ] 升级 `ULSHUDWidget`：集成上述所有子组件
- [ ] 升级 `ULSDamagePopWidget`：对象池化实例复用

---

## 🔶 阶段 11：战术关卡与动态物资生态

> **目标**：搭建可玩的战术关卡流程，实现搜打撤的"搜"和"打"场景体验。
> **对应设计文档**：§11 场景与关卡管理

### 11.1 关卡管理框架
- [ ] **关卡管理子系统 (`ULSLevelManager`)**：Hub 大厅与 Combat 关卡切换、异步流式加载、结算广播
- [ ] **双 GameMode 架构**：`ALSHubGameMode`（枢纽大厅）+ `ALSCombatGameMode`（战斗关卡）
- [ ] **关卡数据资产 (`ULSLevelDataAsset`)**：推荐等级、元素弱点、波次配置、掉落表

### 11.2 波次与刷怪
- [ ] **波次系统 (`ULSWaveSystem`)**：波次倒计时推进、Tag 匹配生成点寻址、清怪进度广播、Boss 波次判定
- [ ] **刷怪管理器 (`ALSSpawnManager`)**：复合生成点组、随机与加权抽选

### 11.3 动态物资宝箱
- [ ] **物资候选锚点 (`ALSLootSpawnPoint`)**：关卡预置 15~20 个候选点
- [ ] **物资箱生成器 (`ALSLootSpawner`)**：局内权威随机点亮 6~8 个宝箱（普通/元素地脉/军备）
- [ ] **物资箱实体 (`ALSLootChest`)**：长按 [F] 1.5s 开箱、材质溶解、爆出弹药/配件

### 11.4 战术关卡搭建
- [ ] **手工战术关卡**：起点整备区 → 前哨阵地 → 偏殿废墟 → 地脉核心遗迹 → 极危密室 → 防守撤离点
- [ ] **关卡传送门 (`ALSLevelPortal`)**：3D 浮动 UI 看板（关卡名/推荐等级/弱点/掉落预览）
- [ ] **随机地脉紊乱词条 (Ley Line Disorders)**：每局 1~2 项全局词条

### 11.5 NavMesh
- [ ] **静态烘焙 Recast NavMesh**：保证所有掩体与交火区域的 AI 寻路无 Bug

---

## 🔶 阶段 12：性能优化与最终打磨

> **目标**：满足性能硬指标（≥60fps / DrawCall<3000 / 内存<4GB / GC<2ms），音频与任务收尾。
> **对应设计文档**：§13 性能优化、§15 任务系统、§17 验证方案

### 12.1 通用对象池
- [ ] **对象池子系统 (`ULSObjectPoolSubsystem`)**：预热/借还高频 Actor（投射物、飘字、Niagara 粒子）
- [ ] **投射物专用池 (`ProjectilePool`)**：消除高频射击 Spawn/Destroy 的 GC 卡顿

### 12.2 AI 与渲染优化
- [ ] **AI Tick 分帧交错**：`FrameCounter % 3 != (AIIndex % 3)`，多怪同屏 CPU 降压
- [ ] **LOD 管理 (`ULSLODManager`)**：远景自动降级
- [ ] **裁剪优化 (`ULSCullingOptimizer`)**：视锥外与遮挡物后裁剪
- [ ] **实例化批渲染 (`UInstancedStaticMeshComponent`)**：弹壳、矿石散落物合批

### 12.3 音频系统
- [ ] **武器音效**：开火声 / 换弹声 / 空仓挂机声
- [ ] **元素反应音效**：16 种反应爆发专属音效
- [ ] **打击反馈音效**：命中 "叮"、爆头 "铛"、击杀确认
- [ ] **角色语音**：切人入场/退场语音

### 12.4 任务系统
- [ ] **任务子系统 (`ULSQuestManager`)**：局内战斗事件驱动目标判定
- [ ] **任务数据 (`ULSQuestData`, `FLSQuestTypes`)**：击杀/搜集/存活等目标模板

### 12.5 验证与压测
- [ ] **模块级验证用例**：每个系统独立 TargetDummy 验证
- [ ] **端到端战术闭环测试**：完整一局搜打撤流程走通
- [ ] **性能硬指标达标**：60fps / DrawCall<3000 / 内存<4GB / GC<2ms

---

## 🗺️ Content 资产配置清单（贯穿阶段 6~12 并行推进）

### 角色蓝图 (`Content/Characters/`)
- [ ] `BP_Character_Pyro`（火）/ `BP_Character_Hydro`（水）/ `BP_Character_Electro`（雷）/ `BP_Character_Cryo`（冰）+ 动画资产

### 武器蓝图 (`Content/Weapons/`)
- [ ] `BP_Rifle_Pyro` / `BP_Sniper_Cryo` / `BP_Shotgun_Electro` / `BP_SMG_Hydro` / `BP_Launcher`

### 敌人蓝图 (`Content/Enemies/`)
- [ ] 7 种敌人蓝图 + 行为树(BT) + 黑板(BB) + EQS 资产

### 特效 (`Content/FX/`)
- [ ] 枪口火花 `NS_MuzzleFlash`、撞击特效 `NS_BulletImpact`、元素拖尾 `NS_ElementalTrail`
- [ ] 16 种元素反应粒子 `NS_Reaction_*`
- [ ] 手雷残留领域特效（火海/水雾/电弧/冰面）

### 关卡 (`Content/Maps/`)
- [ ] `Map_Hub`（枢纽大厅 + 整备区 + 研发终端 + 传送门）
- [ ] `Map_Arena_01`（波次战斗场景）
- [ ] `Map_Boss_01`（Boss 多阶段场景）
- [ ] 搜打撤手工战术关卡

### UI 蓝图 (`Content/UI/`)
- [ ] `WBP_HUD` / `WBP_Crosshair` / `WBP_TeamPortraitBar` / `WBP_KillFeed`
- [ ] `WBP_MetaTree` / `WBP_DriveCore` / `WBP_ExtractionResult` / `WBP_BlessingSelection`

### 数据资产 (`Content/DataAssets/`)
- [ ] `DA_Characters/` / `DA_Weapons/` / `DA_Skills/` / `DA_Enemies/` / `DA_Levels/` / `DA_DriveDiscs/`

### Enhanced Input (`Content/Input/`)
- [ ] 18 个 InputAction 资产 + `IMC_Default` + `IMC_UIMode` 映射上下文

### 音频 (`Content/Audio/`)
- [ ] `SFX_Weapons/` / `SFX_Feedback/` / `SFX_Elements/` / `SFX_Reactions/` / `SFX_Voice/`
