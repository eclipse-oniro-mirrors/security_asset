---
# phase_status: 提案状态机（Clarifying → AwaitingApproval → Approved / ChangesRequested / Blocked）
phase_status: approved
approval:
  status: approved
  approver: "yin yaode"
  evidence: "对话中明确批准 Define"
  approved_at: "2026-11-13"
  baseline_digest: "待运行 ohos-sdd approval-digest 生成"
target_release:
  id: "7.1"
  status: proposed
# change_type: 变更类型（new-feature/enhancement/optimization/bugfix/refactor/deprecation）
change_type: "enhancement"
proposal_id: "20260916754428"
author: "yin yaode"
date: "2026-11-13"
---

# 需求文档

> 主文档只保存需求输入和已确认基线。澄清过程仅在 complex/critical，或出现澄清阻断、争议决策、跨 SIG/多仓协调时写入条件附录。

## 需求输入

### 基本信息

| 字段 | 内容 |
|------|------|
| 需求ID | SR20260916754428（proposal_id 已迁移为 20260916754428） |
| 需求名称 | Asset 适配用户卸载保留数据 |
| 来源 | 需求方描述（BMS 卸载保留数据特性联动） |
| 提出人 | yin yaode |
| 目标发行版本 | OpenHarmony 7.1（参照仓内先例 20260905541319 口径） |
| 候选 Profile | none（系统侧事件驱动行为适配，待 Owner 确认） |
| 优先级 | P3 |
| Define 阶段状态 | Approved |

### 原始描述

**输入 1：** 应用卸载事件（`usual.event.PACKAGE_REMOVED`）

**处理 1：** Asset 根据卸载事件中保留数据字段（want 中的 keepdata）是否设置，选择删除还是保留关键资产。

特别的，群组数据库之前做过多个 Hap 共享同一个群组时，所有该群组的 Hap 卸载了才会删除群组数据；在这里要注意，选择了保留数据的，等该群组 Hap 都卸载了，这些数据还是需要保留（记录一个标记位）；等下次该 Hap 再卸载时如果 keepdata 没传，则按照老的逻辑处理。

**输出 1：** NA

**输入 2：** 清除数据事件（相当于卸载保留之后，要清理没清理的时候的卸载事件）

**处理 2：** Asset 收到清除数据事件（`usual.event.PACKAGE_UNINSTALLED_DATA_CLEARED`），执行关键资产清理操作（开发者卸载保留的数据不删除）。

特别的，群组数据还是按照之前卸载的逻辑进行清理：如果同 developer 下有应用没卸载，则数据要保留（清除掉标记位），否则清理。

**输出 2：** NA

### 背景证据

| 证据类型 | 链接/路径 | 说明 |
|----------|-----------|------|
| 既有卸载链路 | `services/asset/core_service/src/common_event/start_event.rs:135`（`PACKAGE_REMOVED`/`SANDBOX_PACKAGE_REMOVED` 按需拉起并解析 want） | 事件入口与 want 字段解析（userId/appId/appIndex/bundleName/developerId/groupIds）现状 |
| 既有删除逻辑 | `services/asset/core_service/src/common_event/listener.rs:221`（`delete_data_by_owner`）、`:265`（`on_package_removed`） | 现行为：卸载即删（DE+CE 库，排除 TrustedAccount 同步数据），无数据残留时删除 HUKS owner/group 密钥 |
| 群组卸载判定 | `listener.rs:190` 调用 `GetUninstallGroups`（`services/asset/os_dependency/src/bms_wrapper.cpp:335`） | "所有该群组 Hap 卸载才删群组数据"的现行判定依赖 BMS 查询 |
| SA 事件订阅 | `sa_profile/8100.json:14-15` | 新事件 `PACKAGE_UNINSTALLED_DATA_CLEARED` 需在此注册按需拉起 |
| 云同步联动 | `listener.rs:290`（插件 `OnPackageClear`） | 卸载时通知插件清理；清理事件已确认不联动插件 |
| 既有字段 | `services/asset/db_operator/src/types.rs`（`IS_PERSISTENT`，DataType::Bool） | "开发者卸载保留"判定依据：库内 is_persistent=true 的记录在清理事件中豁免删除 |
| 既有 BMS 能力 | `services/asset/os_dependency/src/bms_wrapper.cpp:335`（`GetUninstallGroups` → `GetAllBundleInfoByDeveloperId`） | developer 粒度存活查询现成可用，支撑清理事件群组规则 |

## 需求基线

### 基线信息

| 字段 | 内容 |
|------|------|
| 基线版本 | v1.0 |
| 基线日期 | 2026-11-13 |
| Owner | yin yaode |
| 复杂度 | 复杂（事件语义、群组判定、HUKS 密钥联动、标记位持久化、多版本兼容） |
| Profile | none |
| 目标发行版本 | OpenHarmony 7.1（P3） |
| 版本状态 | proposed |
| 基线结论 | 通过（Define 已批准，2026-11-13） |

### 问题陈述

现行卸载链路（`on_package_removed`）对关键资产**一律删除**：DE/CE 库数据清空（仅 TrustedAccount 云数据保留），并在无残留时删除 HUKS owner/group 密钥。BMS 引入"卸载保留数据"特性后，用户卸载应用可选择保留数据并期望重装后关键资产可用；若 Asset 不适配，卸载即删将直接破坏该特性在关键资产（密码、令牌类）场景的用户预期。

本变更为卸载链路引入 keepdata 分支（保留 + 标记位）与 `PACKAGE_UNINSTALLED_DATA_CLEARED` 清理链路（延迟清理），并保持不带 keepdata 的卸载行为与基线完全一致。

### 目标与成功指标

| 目标 | 成功指标 | 验证方式 |
|------|----------|----------|
| 1. keepdata 卸载保留：卸载时带 keepdata，关键资产（库数据 + HUKS 密钥）完整保留，重装后可查询 | 保留后重装同 bundle/appIndex，query 返回原 secret；HUKS 密钥未删除 | 端到端：写入→带 keepdata 卸载→重装→查询比对 |
| 2. 延迟清理：清理事件到达时按规则清理保留数据，群组按"同 developer 是否仍有应用在装"判定 | 清理后数据/标记位不可见；同 developer 有存活应用时数据保留且标记清除 | 端到端：保留→触发清理事件→分群组场景断言 |
| 3. 基线兼容：不带 keepdata 的卸载行为与现版本一致 | 不带 keepdata 卸载全链路（含群组、密钥、插件通知）与基线无差异 | 回归：现网卸载用例集 |
| 4. 保留数据安全不降级：保留期间数据仍受原加密与隔离保护 | 保留数据仅原 owner 可访问，密钥/加密体系未变 | 权限/隔离用例回归 |

### 用户故事与 AC

| Story ID | 用户故事 | 优先级 |
|----------|----------|--------|
| US-1 | 作为设备用户，我卸载应用时选择保留数据，以便重装后关键资产无需重新录入 | P3 |
| US-2 | 作为设备用户，我清理卸载残留数据时，当初保留的关键资产应被一并清理 | P3 |
| US-3 | 作为使用同 developer 多应用的用户，群组关键资产按"该群组 Hap 全部卸载且选择保留才保留、同 developer 仍有应用在装则不清理"的规则处理 | P3 |

| AC编号 | 验收标准 | 类型 | 关联Story |
|--------|----------|------|-----------|
| AC-1 | WHEN `PACKAGE_REMOVED` 且 want 中 keepdata 为 true THEN Asset 不删除该 owner 的关键资产（DE/CE 库数据与对应 HUKS 密钥），并记录"卸载保留"标记位 | 正常 | US-1 |
| AC-2 | WHEN `PACKAGE_REMOVED` 且 keepdata 缺失或为 false（含 SANDBOX 变体） THEN 删除行为与现版本基线一致（含群组按 `GetUninstallGroups` 判定、密钥清理、插件通知） | 兼容 | US-1/US-3 |
| AC-3 | WHEN 群组内全部 Hap 均卸载且其中任一 Hap 卸载时带 keepdata=true THEN 群组数据与群组 HUKS 密钥保留并记录标记位；重装后再次卸载且不带 keepdata THEN 按老逻辑清理 | 边界 | US-3 |
| AC-4 | WHEN 应用曾以 keepdata=true 卸载保留、重装后再次卸载触发 `PACKAGE_UNINSTALLED_DATA_CLEARED`（want 携带 bundleName/developerId/groupIds）且该 owner 数据带保留标记 THEN 清理其保留的关键资产，但库内 `IS_PERSISTENT=true` 的记录（开发者声明卸载保留）不删除 | 正常 | US-2 |
| AC-5 | WHEN 清理事件到达且同 developer 下仍有应用未卸载 THEN 群组数据保留并清除标记位；同 developer 应用均已不在 THEN 清理群组数据 | 边界 | US-3 |
| AC-6 | WHEN 保留期间第三方进程访问该 owner 关键资产 THEN 仍被权限体系拒绝 | 异常 | US-1 |

> 注：AC 为需求基线级验收标准，spec.md 将按可观察结果细化并补充事件参数缺失、标记位损坏等异常边界。

### 范围边界

**包含：**
- `PACKAGE_REMOVED`（含 SANDBOX 变体）want 中 keepdata 字段的解析与保留分支。
- 新事件 `PACKAGE_UNINSTALLED_DATA_CLEARED` 的订阅（sa_profile）、解析与清理链路。
- 卸载保留标记位的记录、查询、清除（owner 级与群组级）。
- keepdata 与群组卸载判定（`GetUninstallGroups`）的交互逻辑。
- HUKS owner/group 密钥在保留场景的保留策略（不删除）。
- 清理事件中的群组清理规则（同 developer 存活应用判定）。
- 清理事件中对 `IS_PERSISTENT=true` 记录（开发者声明卸载保留）的豁免。

**不包含：**
- BMS/设置侧卸载保留特性的 UI、事件定义与下发（本变更只消费事件）。
- `interface_sdk-js` 对外 API 变更（本变更为系统内部行为，待确认 Q-9）。
- TrustedAccount（SyncType 云同步）数据在保留/清理路径的口径变更（已确认沿用现状）。
- 云同步插件在清理事件的联动行为（已确认不联动）。
- CE 库 db_key 体系改动（保留场景沿用现有 CE 密钥管理）。

### Agent Scope Guard

> 只约束需求探索和上下文收集阶段的可访问仓库、模块与知识源；发现需要越界时必须停止并重新确认基线。

| 维度 | 允许范围/限制 | 越界触发条件 | 处理规则 |
|------|---------------|-------------|----------|
| 允许检索仓库 | `security_asset`（本仓） | 需要访问 BMS 等其他仓源码 | 以事件字段文档/需求方输入为准，不读取外部仓 |
| 允许模块/目录 | `services/asset/core_service/src/common_event/`、`services/asset/core_service/src/lib.rs`、`services/asset/os_dependency/`（bms_wrapper）、`services/asset/db_operator/`、`services/asset/crypto_manager/`、`frameworks/asset/os_dependency/file/`、`sa_profile/`、`test/asset/` | 需要扩大模块/目录 | 更新范围并经 Owner 确认后继续 |
| 禁止访问/修改 | 云同步插件仓闭源内容；既有不带 keepdata 卸载链路的对外行为（只可读取作为基线） | 命中禁止项 | 标记 Blocked，需改动时先更新本表并经 Owner 确认 |
| 外部网络/私有知识源 | 不涉及网络访问 | 需要外部知识源 | 请求人工授权 |

### 影响范围

| 子系统 | 仓库 | 模块/路径 | 当前职责 | 影响类型 | Owner |
|--------|------|-----------|----------|----------|-------|
| Asset 服务 | `security_asset` | `services/asset/core_service/src/common_event/start_event.rs` | 按需事件路由与 want 解析 | 修改（新增清理事件分支 + keepdata 解析） | 待确认 |
| Asset 服务 | `security_asset` | `services/asset/core_service/src/common_event/listener.rs` | 卸载删除链路编排 | 修改（保留分支 + 清理链路 + 标记位） | 待确认 |
| Asset 服务 | `security_asset` | `services/asset/os_dependency/src/bms_wrapper.cpp` | BMS FFI（GetUninstallGroups 等） | 修改（如需"同 developer 存活应用"查询则新增 FFI，待设计定） | 待确认 |
| Asset 服务 | `security_asset` | `services/asset/crypto_manager/`（HUKS 密钥删除链路调用侧） | 卸载时密钥清理 | 修改（保留场景跳过删除） | 待确认 |
| Asset 服务 | `security_asset` | 标记位存储：`db_operator/src/keepdata_cache.rs`（独立 keepdata.cache 文件） | 无（新增） | 新增 | yin yaode |
| Asset 服务 | `security_asset` | `sa_profile/8100.json` | SA 按需事件订阅 | 修改（注册 PACKAGE_UNINSTALLED_DATA_CLEARED） | 待确认 |
| 测试 | `security_asset` | `test/asset/unittest/module_test/` | 模块测试 | 新增（事件与标记位用例） | 待确认 |

### API 变更项清单

> 涉及 API 变更时必填。本变更预计无对外 API 变更（纯系统内部事件驱动行为），待确认。

| API 名称 | 变更类型 | 开放范围 | 概要说明 |
|----------|----------|----------|----------|
| 无（待确认） | — | — | 无 public/system API 变更；无新增 d.ts 声明 |

### 设备与形态差异（1+8 设备差异规格）

| 设备/差异项 | 是否存在差异 | 差异说明 |
|-------------|--------------|----------|
| phone/tablet/pc/2in1/wearable/tv/car | 否 | 与 default 共用基线 |
| default（其他设备） | 否 | 纯系统侧事件驱动行为，不依赖设备品类 |
| 功能差异（非品类划分） | 否 | 无按设备形态的差异 |

### 不涉及项确认

| 维度 | 是否涉及 | 依据 | 若涉及，进入哪个下游文档 |
|------|---------|------|--------------------------|
| 性能 | 否 | 事件低频（卸载/清理触发），无热路径改动 | — |
| 安全/权限 | 是 | HUKS 密钥保留策略、保留数据隔离边界需安全审视 | design.md / spec.md |
| 兼容性 | 是 | 不带 keepdata 行为需与基线一致；标记位持久化需考虑版本升级/回退 | spec.md |
| API/SDK | 否（待确认） | 推测无对外 API 变更，需与需求方确认 | — |
| IPC/跨进程 | 否 | 复用既有 SA 按需拉起与事件 want 机制 | — |
| 构建/组件 | 是 | sa_profile 事件订阅配置变更 | design.md |
| 国际化/无障碍 | 否 | 无用户可见文案 | — |
| 数据迁移 | 是 | 标记位为新增持久化状态，需定义升级/重装下的迁移与清理 | design.md |

### 依赖、风险与外部约束

| 依赖/风险 | 类型 | 影响 | 缓解措施/Owner | 状态 |
|-----------|------|------|----------------|------|
| keepdata 字段与清理事件 want 参数定义（BMS 侧） | 外部依赖 | 高 | 需求方/BMS 提供，澄清 Q-1/Q-3 | 待澄清 |
| HUKS 密钥保留与数据可解密性 | 技术 | 高 | 密钥误删将导致保留数据不可解密（重装后解密失败）；设计需明确 owner/group 密钥保留条件 | 待设计 |
| "同 developer 存活应用"判定能力 | 技术 | 中 | 现有 `GetUninstallGroups` 语义是否覆盖，或需新增 BMS FFI | 待澄清 |
| 标记位损坏/残留的容错 | 技术 | 中 | 标记位丢失导致行为回退老逻辑（可接受）；残留导致清理遗漏（需清理事件兜底） | 待设计 |
| 群组场景组合复杂度 | 技术 | 中 | keepdata×群组×重装×清理事件的组合用例矩阵 | spec.md 用例展开 |

### 资源开销审视

| 维度 | 状态 | 信号/依据 | 确认人/理由/范围 |
|------|------|-----------|-----------------|
| 性能 | not-applicable | 事件低频冷路径 | — |
| 内存(RAM) | not-applicable | 无新增常驻内存 | — |
| 存储(ROM) | review-required | 新增标记位持久化与事件处理代码；保留数据本身不增加 ROM | 设计阶段评估镜像增量与标记位体量 |

### 依据与上下文引用

| 来源 | 查询/读取内容 | 基线采用的结论 | 可信度 | 证据路径/链接 |
|------|---------------|----------------|--------|---------------|
| 本仓代码基线 | `start_event.rs:98-127`（handle_package_removed）、`listener.rs:142-263`（删除链路）、`bms_wrapper.cpp:335` | 现卸载行为：DE+CE 全删（排除 TrustedAccount）、群组按 GetUninstallGroups 判定、无残留删 HUKS 密钥、通知插件 | 高 | 源码路径 |
| 本仓代码基线 | `listener.rs:144`（reverse_condition=TrustedAccount） | 现行卸载不删除可信账号云数据 | 高 | 源码路径 |
| 本仓配置 | `sa_profile/8100.json` | 新事件需注册按需拉起 | 高 | 配置路径 |
| 需求方描述 | 本 proposal「原始描述」 | keepdata 语义、清理事件、群组标记位规则 | 中（细节待澄清） | 需求输入 |

### 变更控制与下游条件

| 变更类型 | 触发条件 | 处理规则 |
|----------|----------|----------|
| 范围新增 | 新增用户故事或仓/模块 | 重新评估复杂度和影响范围，重新确认基线 |
| AC 变更 | 修改可观察行为 | 重新审批基线和 Spec |
| API 变更 | 新增/修改 Public/System API | 触发设计审批 |
| 非功能指标变更 | 阈值变化 | 重新确认验证计划 |
| 目标版本变更 | 交付版本调整 | 更新 proposal.target_release |

- [x] 所有 P1 用户故事有可测试、可度量的 AC（优先级待确认）
- [x] 范围内外、影响范围已按代码基线落位
- [ ] 需求元数据（ID/Owner/目标版本/优先级）待确认
- [x] 基线结论为"草案"，进入澄清阶段

## 附录：澄清与决策记录（条件启用）

> 本变更复杂度=复杂（群组判定 × 密钥联动 × 标记位持久化 × 事件语义），触发本附录。

### 触发与待澄清问题

| 项目 | 内容 | 验证方式/依据 | 状态 |
|------|------|---------------|------|
| 初始假设 | `PACKAGE_REMOVED` 仍以 SA 按需拉起，want 中 keepdata 为 bool 字段；SANDBOX 变体不带 keepdata、不处理，走老逻辑 | 需求方确认（2026-11-13 已确认） | 已验证 |
| 初始假设 | `PACKAGE_UNINSTALLED_DATA_CLEARED` 同样按需拉起，需在 `sa_profile/8100.json` 注册 | 需求方确认 | 待验证 |
| 初始假设 | "开发者卸载保留"由库内 IS_PERSISTENT 字段判定（true=开发者声明卸载也保留），与用户 keepdata 保留相互独立 | 需求方确认（2026-11-13 已确认） | 已验证 |
| 初始假设 | 标记位独立文件持久化（记录 bundleName 与 developerId），仅 Asset 内部使用，不暴露 API | 需求方确认（2026-11-13 已确认） | 已验证 |
| 初始假设 | 保留场景 HUKS owner/group 密钥不删除（否则保留数据不可解密） | 需求方确认（2026-11-13 已确认密钥保留） | 已验证 |
| 复杂度触发 | 复杂（事件语义 + 群组组合 + 密钥联动 + 持久化兼容） | 初始分级判断 | 已确认 |

| 编号 | 问题 | 为什么需要澄清 | 状态 |
|------|------|---------------|------|
| Q-1 | keepdata 在 want 中的确切参数名、取值格式（布尔/字符串）与缺失语义；SANDBOX 变体是否同样携带 | 参数解析是入口分支的唯一依据 | 已澄清（2026-11-13：bool；缺失/false 走老逻辑；SANDBOX 不管） |
| Q-2 | "开发者卸载保留的数据不删除"的确切语义 | 直接决定清理事件的过滤规则，AC-4 不可验证 | 已澄清（2026-11-13：以库内 is_persistent 字段判定，true 表示卸载也保留，清理事件中豁免） |
| Q-3 | `PACKAGE_UNINSTALLED_DATA_CLEARED` 的 want 参数集与触发场景 | 决定清理链路如何定位 owner 与群组 | 已澄清（2026-11-13：want 携带 bundleName/developerId/groupIds；触发=先以 keepdata=true 卸载保留，再重装再卸载时触发，语义等同卸载事件） |
| Q-4 | 保留场景 HUKS 密钥是否保留（现链路无残留即删密钥） | 密钥删除=保留数据不可解密 | 已澄清（2026-11-13：确认密钥保留） |
| Q-5 | 标记位存储介质的约束 | 设计决策输入 | 已澄清（2026-11-13：独立文件，记录 bundleName 与 developerId） |
| Q-6 | 重装衔接：标记位生命周期（"等下次该 hap 再卸载"的衔接） | 标记位状态机闭环 | 已澄清（2026-11-13：重装后再卸载通过清除数据事件通知，标记由清理事件消费） |
| Q-7 | TrustedAccount 云数据在保留/清理路径的口径 | 避免与云同步数据语义冲突 | 已澄清（2026-11-13：不管，沿用现状） |
| Q-8 | 云同步插件是否需要感知清理事件 | 插件联动范围 | 已澄清（2026-11-13：不联动） |
| Q-9 | 需求元数据：正式需求编号、Owner、目标发行版本、优先级 | 流程登记 | 已澄清（需求号 SR20260916754428；目标版本 OpenHarmony 7.1 / 优先级 P3；Owner：yin yaode） |
| Q-10 | 清理事件仅有 bundleName，如何定位保留的 owner 库（appId/appIndex 缺失，clone 应用多 app_index） | 清理链路定位依据 | 已澄清（2026-11-13：标记文件记录 appId、appIndex、developerId、groupIds；developerId 与 groupIds 仅在 appIndex==0 时记录，清理按标记定位） |
| Q-11 | 群组混合 keepdata（部分 HAP 带保留、最后卸载的不带）时群组数据去留 | 群组规则边界 | 已澄清（2026-11-13：群组内任一 HAP 卸载时带过 keepdata → 全部卸载后群组保留并记录标记位） |
| Q-12 | "同 developer 下仍有应用在装"的判定能力 | 清理事件群组规则依赖 | 已澄清（2026-11-13：复用现有实现——`GetUninstallGroups` 内部 `GetAllBundleInfoByDeveloperId`（bms_wrapper.cpp:335/348）可查 developer 下应用存活状态，无需新增 FFI，细节 design 阶段确认） |
| Q-13 | 重装后第二次卸载仍带 keepdata 的行为 | 状态机边界 | 默认假设（未异议即生效）：再次保留，标记刷新、不清理数据 |

### 方案取舍（草案）

| 方案 | 核心思路 | 优势 | 风险/代价 | 结论 |
|------|----------|------|-----------|------|
| A-1 | 标记位独立持久化（文件记录 appId/appIndex/developerId/groupIds，developerId 与 groupIds 仅 appIndex==0 时记录），保留分支短路既有删除链路 | 增量小，不动库 schema；记录定位信息可直接反查 owner 库 | 标记与库数据一致性需自行维护 | 已确认（2026-11-13 需求方选定并细化记录内容） |
| A-2 | 标记位入库（每库新增表/列，或集中标记表） | 与库数据事务一致 | 侵入所有 Hap/Group/Native 库 schema，升级迁移成本高 | 备选 |

### 澄清结论

- [x] Q-1 ~ Q-8、Q-10 ~ Q-12 已关闭（2026-11-13 需求方澄清）；Q-9 部分关闭（需求号已提供，Owner/目标版本/优先级待补）；Q-13 默认假设生效
- [x] 范围、兼容性、依赖风险已形成基线结论（HUKS 密钥保留、IS_PERSISTENT 豁免、标记位独立文件记录 appId/appIndex/developerId/groupIds、群组混合 keepdata=任一带即保留、developer 存活判定复用 GetUninstallGroups 链路）
- [x] 方案取舍已确认（A-1 独立文件方案）

**结论：** 基线候选完成（phase_status = awaiting_approval；待需求方/Owner 明确批准 Define 后进入 Specify。批准时填写 approval 各字段，并运行 `ohos-sdd approval-digest` 生成 baseline_digest）
