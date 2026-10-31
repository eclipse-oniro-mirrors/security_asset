---
# phase_status: 提案状态机（Clarifying → AwaitingApproval → Approved / ChangesRequested / Blocked）
phase_status: approved
approval:
  status: approved
  approver: "Lin Shen"
  evidence: "功能设计评审"
  approved_at: "2026-10-24"
  baseline_digest: "48e4c5d21eda88551dd1178c7e06d2f448dc1900c9418d91a8840efaf0e3935e"
target_release:
  id: "7.1"
  status: proposed
# change_type: 变更类型（new-feature/enhancement/optimization/bugfix/refactor/deprecation）
change_type: "new-feature"
proposal_id: "20260905541319"
author: "Lin Shen"
date: "2026-10-24"
---

# 需求文档

> 主文档只保存需求输入和已确认基线。澄清过程仅在 complex/critical，或出现澄清阻断、争议决策、跨 SIG/多仓协调时写入条件附录。

## 需求输入

### 基本信息

| 字段 | 内容 |
|------|------|
| 需求ID | REQ-20260905541319（proposal_id） |
| 需求名称 | 任务级身份凭证支撑 Agent 长时/定时任务在锁屏下执行 |
| 来源 | 用户反馈（智能体长时/定时任务锁屏执行中断） |
| 提出人 | Lin Shen |
| 目标发行版本 | OpenHarmony 7.1（proposal.target_release.id） |
| 候选 Profile | none（系统侧安全能力，不命中子系统 profile；待 Owner 确认） |
| 优先级 | P1 |
| Define 阶段状态 | Approved |

### 原始描述

**原始问题：** 智能体（Agent）正成为用户委托任务执行的入口，但现有的授权模型只覆盖“用户在场时的一次性授权”（如工具权限申请、锁屏解锁），承载不了任务在**跨会话、跨锁屏状态**下的生命周期。因此当 Agent 被用户授权执行**长时任务**（数分钟至数小时）或**定时任务**（到点触发）时，任务执行到一半会因锁屏或会话结束被中断，定时任务无法在锁屏状态下被拉起。

**用户核心声量：** 智能体当前**不支持长时、定时任务在锁屏下执行**。用户已授权的任务因设备锁屏而中断，需要用户保持解锁或反复确认才能跑完，这与“用户本人授权后任务应可靠执行”的预期冲突，是用户反馈的主要来源。

| 用户类型 | 当前痛点 | 影响 |
|----------|----------|------|
| 终端用户（设备机主） | 已授权的长时/定时任务因锁屏或会话结束被中断，需保持解锁或反复确认 | 任务执行成功率与用户体验下降，与“授权后可靠执行”预期冲突 |
| 系统侧任务经办方（SA/插件仓，闭源） | 缺少可跨会话、可在锁屏态校验的任务级授权载体 | 无法承载长时/定时任务的生命周期授权 |

**期望结果：** 用户本人完成 IAM 机主认证（锁屏解锁/生物识别）后，授权 Agent 执行的长时、定时任务可在锁屏态下持续执行或到点拉起；未完成足够强度认证（ATL 不足）的执行被管控/拦截。结果层面不预设实现方案。

### 背景证据

| 证据类型 | 链接/路径 | 说明 |
|----------|-----------|------|
| 用户反馈 | 贾维斯EWP（智能体长时/定时任务锁屏执行中断反馈） | 智能体长时/定时任务锁屏执行中断是本次需求的主要声量来源 |
| 既有实现 | 本仓 `services/secure_access_fence/core_service/`（代码基线 `7e110901` 既有 ticket 链路） | 基线已有 requestToolPermissions/ticket 链路与双 challenge HMAC 模式，是本变更复用与扩展的对象；task identity 能力在基线不存在，全部为本次新增 |
| 接口声明 MR | `interface_sdk-js` MR 36745 | SystemAPI 变更 + 任务有效性校验 InnerApi 的声明载体 |

## 需求基线

### 基线信息

| 字段 | 内容 |
|------|------|
| 基线版本 | v1.0 |
| 基线日期 | 2026-10-24 |
| Owner | Lin Shen |
| 确认人 | Lin Shen |
| 确认依据 | 功能设计评审 |
| 复杂度 | 复杂 |
| Profile | none（待 Owner 确认） |
| 目标发行版本 | OpenHarmony 7.1（引用 proposal.target_release） |
| 版本状态 | proposed |
| 基线结论 | 通过 |

### 问题、目标与成功指标

**问题陈述：** 现有授权模型只覆盖“用户在场时的一次性授权”，承载不了任务在跨会话、跨锁屏状态下的生命周期；已授权长时/定时任务因锁屏或会话结束被中断。本次变更在安全侧引入任务级身份凭证，将授权从“会话”解耦到“任务”，并用“指令风险等级 × IAM 机主认证安全等级（ATL）”在锁屏执行时做管控/拦截——即“谁授权的”（用户本人，经 IAM 认证）+“授权到什么程度”（任务凭证 + 风险等级与 ATL 的管控矩阵）共同决定任务能否在锁屏下执行。

| 目标 | 成功指标 | 验证方式 |
|------|----------|----------|
| 1. 用户本人授权 Agent 执行长时、定时任务：通过任务级身份凭证将授权与“会话”解耦，凭证跨会话有效，用户授权后无需持续在场 | 用户授权→退出/重建会话→任务凭证仍可被校验通过；未重新认证的任务不被非法放行 | 端到端：授权→跨会话校验→执行，含负面用例（无认证/认证过期） |
| 2. 授权后长时、定时任务可在锁屏下执行：凭证绑定用户本人 IAM 机主认证结果，执行侧按“指令风险等级 × ATL”完成管控/拦截，已授权任务锁屏态下可持续执行或到点拉起 | 设备锁屏后长时任务运行不被中断且按授权范围完成；定时触发时刻处于锁屏态仍被拉起并按授权执行 | 端到端：授权→锁屏→长时任务全程可观测完成；预授权→锁屏→到点触发→确认拉起与执行结果 |
| 3. 安全底线不放松：任何锁屏执行都以“用户本人完成足够强度认证（ATL 满足风险等级要求）”为前提；未达标执行被拦截，凭证可验证、可过期、可撤销 | ATL 未达要求的执行被拦截或降级，不产生任务副作用；篡改、过期、撤销的凭证均校验失败，撤销后立即失效 | 矩阵用例遍历（风险等级 × ATL 象限）+ 校验路径注入用例 + 撤销后置校验 |

### 用户故事与 AC

| Story ID | 用户故事 | 优先级 |
|----------|----------|--------|
| US-1 | 作为设备机主，我想要锁屏后已授权的长时任务继续执行，以便任务不中断、无需保持在场 | P1 |
| US-2 | 作为设备机主，我想要锁屏态下定时任务到点拉起执行，以便满足“授权后自动执行”的预期 | P1 |
| US-3 | 作为设备机主/任务经办方，我想要未达机主认证安全等级（ATL）的高危任务被执行侧管控/拦截，以便高敏感任务只在用户本人完成足够强度认证后才执行 | P1 |

| AC编号 | 验收标准 | 类型 | 关联Story |
|--------|----------|------|-----------|
| AC-1 | WHEN 用户完成机主认证并授权长时任务，随后退出或重建会话 THEN 任务凭证仍可被校验通过，任务可继续按授权范围执行 | 正常 | US-1 |
| AC-2 | WHEN 设备处于锁屏态且该长时任务已被授权 THEN 任务运行不被中断，可全程完成且无副作用 | 边界 | US-1 |
| AC-3 | WHEN 定时任务已预授权且触发时刻设备处于锁屏态 THEN 任务被拉起并按授权执行，无需用户在场确认 | 正常 | US-2 |
| AC-4 | WHEN 任务风险等级要求的 ATL 高于机主认证返回的 ATL THEN 执行被拦截或降级，不产生任务副作用；ATL 达标时正常放行 | 异常 | US-3 |
| AC-5 | WHEN 任务凭证被篡改、已过期或已撤销 THEN 凭证校验失败，撤销后立即失效 | 异常 | US-3 |

> 注：上述 AC 为需求基线级验收标准，spec.md 将按三层接口边界（public/system/inner API 可观察结果）细化展开并补充错误码与已知边界。

### 范围边界

**包含：**
- 任务级身份凭证（Task Identity）能力底座：凭证签发/撤销/校验/消费，引入任务级身份将授权与“会话”解耦，凭证跨会话有效。
- 锁屏执行管控：执行侧按“指令风险等级 × IAM 机主认证安全等级（ATL）”拦截/降级或放行；凭证绑定用户本人认证结果。
- 新增/变更 SystemAPI 与 1 个任务有效性校验 InnerApi（供系统侧调用）。

**不包含：**
- 智能体/上层业务侧的任务编排、定时调度、UI 交互（本次只提供安全侧能力底座）。
- 改变既有 `requestToolPermissions` / ticket 链路的对外行为与错误码语义（Task Identity 为增量能力，复用既有双 challenge HMAC 模式）。
- IAM(useriam) 认证机制本身的实现与 ATL 判定逻辑改动（仅消费 `VerifyAuthToken` 返回值）。
- 新增额外 public API 入口（除上述 SystemAPI 与任务有效性校验 InnerApi 外）。
- 跨设备、多用户间任务凭证迁移或协同。

| 阶段（如适用） | 范围 | 不做 | 进入条件 |
|----------------|------|------|----------|
| 骨架/完整 | 凭证生命周期（签发/撤销/校验/消费）+ 锁屏执行 ATL 管控 + API 声明变更 | 上层编排/调度/UI、IAM 认证机制、ticket 链路行为改动 | 需求基线确认后进入 Specify |

### Agent Scope Guard

> 只约束需求探索和上下文收集阶段的可访问仓库、模块与知识源；发现需要越界时必须停止并重新确认基线；精确代码文件范围由 execution-plan.md / task.md 持有。若需要突破边界，先更新本 proposal 并获得确认。

| 维度 | 允许范围/限制 | 越界触发条件 | 处理规则 |
|------|---------------|-------------|----------|
| 允许检索仓库 | `security_asset`（主仓）、`interface_sdk-js`（声明仓，仅 API 声明变更） | 需要访问未列仓库 | 停止探索，补充影响分析并重新确认基线 |
| 允许模块/目录 | 主仓：`services/secure_access_fence/core_service/src/task_identity/`、`lib.rs`/`stub.rs`/`wrapper.rs` 相关调用链、`interfaces/` 下既有 Inner API 定义；声明仓：对应 Kit 的 `.d.ts` 与 API 版本配置 | 需要扩大模块/目录 | 更新范围与影响范围，经 Owner 确认后继续 |
| 禁止访问/修改 | 既有 `requestToolPermissions`/ticket 链路的对外行为与错误码语义；IAM(useriam) 认证机制；闭源插件仓相关内容（开源文档中不得表述） | 命中禁止项 | 标记 Blocked，不读取、不修改；需改动时先更新本表并经 Owner 确认 |
| 外部网络/私有知识源 | 不涉及网络访问；新增对 `useriam_framework`（`UserAccessCtrlClient::VerifyAuthToken`）的编译/运行依赖，需引入 CXX 桥与 `bundle.json`/`BUILD.gn` 依赖声明（自提交 cd731e5b 引入）；无新增外部服务。`interface_sdk-js` 声明仓（知识源授权）：`https://gitcode.com/openharmony/interface_sdk-js`（API/SDK 确认=是，spec 阶段 API 规格设计直接复用此仓） | 依赖变更面超出设计范围 / 需要新增权限或未授权来源 | 请求人工授权；未授权前不得使用，先更新 Agent Scope Guard 后重新确认基线 |

### 影响范围

| 子系统 | 仓库 | 模块/路径 | 当前职责 | 影响类型 | Owner |
|--------|------|-----------|----------|----------|-------|
| 安全访问围栏（SAF） | `security_asset` | `services/secure_access_fence/core_service/src/task_identity/` | 凭证签发/撤销/校验/消费 | 新增 | 待确认 |
| 安全访问围栏（SAF） | `security_asset` | `services/secure_access_fence/core_service/src/lib.rs` | SAFService 入口、verify_ticket_impl 编排 | 修改 | 待确认 |
| 安全访问围栏（SAF） | `security_asset` | `services/secure_access_fence/core_service/src/stub.rs` + `wrapper.rs` | IPC 分发、CXX 桥 | 修改 | 待确认 |
| 安全访问围栏（SAF） | `security_asset` | `frameworks/secure_access_fence/inner_api/ipc/` | u8 序列化等既有 IPC 载荷 | 修改 | 待确认 |
| 安全访问围栏（SAF） | `security_asset` | `interfaces/inner_kits/secure_access_fence/` | SystemAPI 声明 + 新 InnerApi 声明 | 修改 | 待确认 |
| 安全访问围栏（SAF） | `security_asset` | `sa_profile/`、`BUILD.gn`、`bundle.json`、SELinux 相关配置 | 组件构建与权限基础 | 修改 | 待确认 |
| 声明/接口 | `interface_sdk-js` | 对应 Kit 的 `.d.ts`（SystemAPI 变更 + 1 个任务有效性校验 InnerApi，见 MR 36745） | API 声明合并与发布 | 修改 | 待确认 |

### API 变更项清单

> 涉及 API 变更时必填，作为 design.md 与 spec.md 的共享锚点。

| API 名称 | 变更类型 | 开放范围 | 概要说明 |
|----------|----------|----------|----------|
| 任务级身份管理 SystemAPI（具体签名以 `interface_sdk-js` MR 36745 现状态为准） | 新增 | System | 供系统应用侧任务经办方生成/撤销任务级身份凭证，供 Agent 长时/定时任务使用 |
| 任务有效性校验 InnerApi（1 个） | 新增 | InnerAPI | 供系统侧内部调用（SA/插件仓，闭源），校验任务凭证当前是否有效（未过期、未撤销、校验通过），作为锁屏执行放行的条件 |

### 设备与形态差异（1+8 设备差异规格）

> 逐项判断设备形态是否存在能力、交互、资源或约束差异；无差异也必须填写"否"并说明共用基线。`default（其他设备）` 用于表中未单列的设备形态；跨设备但不按品类划分的差异填写最后一行。

| 设备/差异项 | 是否存在差异 | 差异说明 |
|-------------|--------------|----------|
| phone | 否 | 与 default 共用基线 |
| tablet | 否 | 与 default 共用基线 |
| pc/2in1 | 否 | 与 default 共用基线 |
| wearable | 否 | 与 default 共用基线 |
| tv | 否 | 与 default 共用基线 |
| car | 否 | 与 default 共用基线 |
| default（其他设备） | 否 | 任务级身份凭证、锁屏执行与 ATL 管控为系统侧安全能力，不依赖特定设备品类；各品类共用同一条实现基线 |
| 功能差异（非品类划分） | 否 | 无按设备形态的差异 |

### 不涉及项确认

> 逐维度主动判断"涉及/不涉及"并填写依据；不允许留空；标"不涉及"必须写明理由。**API/SDK 维度特殊**：无论是否可以从需求描述中推断，都必须先与开发者确认是否涉及 API/SDK，不得自行填写。

| 维度 | 是否涉及 | 依据 | 若涉及，进入哪个下游文档 |
|------|---------|------|--------------------------|
| 性能 | 是 | 凭证校验/锁屏拉起在冷路径，见下方「资源开销审视」性能维度 review-required | design.md / spec.md |
| 安全/权限 | 是 | 机主认证 ATL、凭证防篡改、锁屏执行管控均属安全敏感变更；涉及既有权限复用与联动 | design.md / spec.md |
| 兼容性 | 是 | 复用既有 ticket/HMAC 模式，需保持既有票证链路对外行为与错误码语义不变；新增 API 不与既有 API 冲突 | spec.md |
| API/SDK | 是 | 已与开发者确认涉及 API 变更：`interface_sdk-js` SystemAPI 变更 + 1 个任务有效性校验 InnerApi（MR 36745）；声明仓 `https://gitcode.com/openharmony/interface_sdk-js` | design.md / spec.md |
| IPC/跨进程 | 是 | 凭证一经签发即随任务 IPC 传递并校验；SA 服务与调用方跨进程交互（复用既存 IPC 路径） | design.md |
| 构建/组件 | 是 | `BUILD.gn`/`bundle.json` 涉及依赖调整（新增 `user_auth_framework` 组件依赖与 SELinux `binder_call`）；声明仓同步合入 | design.md |
| 国际化/无障碍 | 否 | 纯系统侧安全能力，无用户可见文案或可访问性交互面 | — |
| 数据迁移 | 否 | 无既有持久化数据格式变更；新凭证沿用既有 `taskAttributes` 磁盘结构 | — |

### 依赖、风险与外部约束

| 依赖/风险 | 类型 | 影响 | 缓解措施/Owner | 状态 |
|-----------|------|------|----------------|------|
| `useriam_framework` `UserAccessCtrlClient::VerifyAuthToken`（经 CXX 桥 `iam_verify_wrapper`） | 编译/运行 | 高 | 随本变更引入；`bundle.json`/`BUILD.gn` 加 `user_auth_framework` 组件依赖 + SELinux `binder_call` 权限（并配套安全设计） | 待确认 |
| `interface_sdk-js` MR 36745 声明合并与发布 | 交付 | 中 | 随本变更同步提交，spec.md「API 规格定义」以 MR 现状态固化 | 待确认 |
| 权限声明（复用既有 `ohos.permission.QUERY_TOOL_PERMISSIONS` / `INTERACT_ACROSS_LOCAL_ACCOUNTS` 等） | 运行 | 低 | 复用既有权限，不新增 | 已确认 |
| 指令风险等级分级口径与来源未定义 | 技术 | 高 | 在 spec/design 阶段明确（由谁定义、如何随指令下发），否则"风险等级 × ATL"管控矩阵无法落成精确规则 | 待确认 |
| 锁屏执行冷路径时延（凭证校验含 IAM/桥调用或本地 HMAC） | 性能 | 中 | 设计阶段给出校验路径时延预算与证据，纳入锁屏拉起冷路径评估 | 待确认 |
| 跨 SIG 协同（IAM(useriam)、SELinux、interface_sdk-js） | 外部 | 中 | 与相关安全 SIG 对齐，记录跨 SIG 确认结论 | 待确认 |

### 资源开销审视

> 三维状态：`required` / `review-required` / `not-applicable` / `waived`。存储(ROM) 主语：交付进 OS 版本镜像的产物 + 运行后 data 分区下的大体量持久化。

| 维度 | 状态 | 信号/依据 | 确认人/理由/范围 |
|------|------|-----------|-----------------|
| 性能 | review-required | 凭证校验进入锁屏执行冷路径；每次校验含 IAM/桥调用（高危）或纯本地 HMAC（低危），需评估对锁屏拉起时延的影响 | 设计阶段给出校验路径时延预算与证据 |
| 内存(RAM) | not-applicable | 凭证存储沿用既有 LRU 32 上限（~24KB），无新增可持续内存占用 | 若新增锁屏执行缓存，回退 review-required |
| 存储(ROM) | review-required | 新增约 2000 行 Rust 代码（折算约 100KB 镜像体积）；磁盘 task 信息按**单 agent 最多缓存 10MB** 的存储基线约束 | 独立 ABU/冒烟验证镜像增量 ≤100KB，并验证单 agent 10MB 磁盘缓存边界不超限 |

### 依据与上下文引用

| 来源 | 查询/读取内容 | 基线采用的结论 | 可信度 | 证据路径/链接 |
|------|---------------|----------------|--------|---------------|
| 本仓既有代码基线 | 代码基线 `7e110901`（`services/secure_access_fence/core_service/src/ticket_operation/`、`frameworks/secure_access_fence/inner_api/ipc/`） | 基线已有双 challenge HMAC ticket 链路与错误码 `TICKET_OPERATION` 前缀规范；task identity 相关能力（含 `TASK_IDENTITY_NOT_FOUND 0x71005` 等错误码、LRU 32 存储）为本次新增 | 高 | 代码基线 `7e110901` + `spec.md` 事实表 |
| 本仓既有实现 | `services/secure_access_fence/core_service/src/`（lib.rs/stub.rs/wrapper.rs、inner_api/ipc） | SAFService 入口、IPC 分发、CXX 桥作为集成点 | 高 | 源码路径（spec/design 阶段用 文件:行 固化） |
| 接口声明 MR | `interface_sdk-js` MR 36745（SystemAPI + 1 个任务有效性校验 InnerApi） | API 签名/权限/错误码以 MR 现状态为准，spec.md「API 规格定义」固化 | 中（以 MR 现状态为准） | MR 36745 |
| 用户反馈 | 贾维斯EWP | "智能体不支持长时/定时任务锁屏执行"为本次需求来源 | 中 | 贾维斯EWP（文本记录） |

### 变更控制与下游条件

| 变更类型 | 触发条件 | 处理规则 |
|----------|----------|----------|
| 范围新增 | 新增用户故事或仓/模块 | 重新评估复杂度和影响范围，重新确认基线 |
| AC 变更 | 修改可观察行为或错误码 | 重新审批基线和 Spec |
| API 变更 | 新增/修改 Public/System API | 触发设计审批 |
| Agent 探索范围变更 | 需要访问未列仓库/模块、禁止项或新知识源 | 停止探索，更新 Agent Scope Guard 后重新确认基线 |
| 非功能指标变更 | 性能/安全/兼容性阈值变化 | 重新确认验证计划 |
| 目标版本变更 | 交付版本调整 | 更新 proposal.target_release |

- [x] 所有 P0/P1 用户故事有可测试、可度量的 AC
- [x] 范围内外、影响范围、API 变化和不涉及项已确认（API 声明仓 `interface_sdk-js` 已确认）
- [x] `proposal.target_release` 已确认（OpenHarmony 7.1）
- [ ] `manifest.profile` 已确认或明确 none（当前为 none，待 Owner 确认）
- [x] Agent Scope Guard 已基线化，未复制 Plan/Task 的实现文件清单
- [x] 基线结论为"通过"，确认人 Lin Shen、确认依据"功能设计评审"已完整
- [ ] 条件附录触发时，待澄清项和争议决策已关闭（开放问题待澄清）

## 附录：澄清与决策记录（条件启用）

> 本变更复杂度=复杂 且涉及跨 SIG/多仓协调（IAM、SELinux、interface_sdk-js），触发本附录；澄清过程与结论同步记录于对话与 `evidence/checks/check-proposal.md`。

### 触发与待澄清问题

| 项目 | 内容 | 验证方式/依据 | 状态 |
|------|------|---------------|------|
| 初始假设 | 复用既有 `requestToolPermissions` 双 challenge HMAC 模式派生任务凭证，与 IAM `VerifyAuthToken` 返回值(challenge + ATL)语义一致，可直接消费 | 源码核对（代码基线 `7e110901` ticket 链路） | 待验证 |
| 初始假设 | 锁屏执行仅需要"凭证有效 + ATL 达标的放行/拦截"，不引入新的锁屏判定机制；锁屏状态检测沿用现有系统能力 | 源码 + 系统能力 review | 待验证 |
| 初始假设 | 新增的"任务有效性校验 InnerApi"为系统侧内部接口，调用方为本仓 SA/插件仓（闭源），不在开源侧展开插件仓细节 | 内部确认 | 已确认 |
| 初始假设 | 机主认证（锁屏解锁/生物识别）与指令风险等级的映射关系（风险等级分级口径）由安全侧基线提供，本变更只在执行侧消费判定结果 | 安全侧基线确认 | 待验证 |
| 复杂度触发 | 复杂（跨 NAPI/Inner API/IPC/SA 服务/IAM 桥多层；安全关键路径；涉及凭证生命周期与管控矩阵） | 初始分级判断 | 已确认 |

| 编号 | 问题 | 为什么需要澄清 | 状态 |
|------|------|---------------|------|
| Q-1 | 指令风险等级的分级口径与来源（由谁定义、如何随指令下发） | 不闭环则"风险等级 × ATL"管控矩阵无法落成精确规则，目标 2 与 AC-4 不可验证 | 已澄清（来源见下） |
| Q-2 | `interface_sdk-js` MR 36745 中 SystemAPI 与 InnerApi 的具体签名、权限与错误码 | 需以该 MR 当前状态为准在 spec.md「API 规格定义」中固化 | 待澄清 |
| Q-3 | 锁屏执行是否涉及 ATL 阈值/豁免策略（如高风险长时任务锁屏后是否需要更高 ATL） | 影响管控矩阵边界与 AC-4 判定规则 | 待澄清 |

### 讨论与方案决策

| 日期 | 参与人 | 讨论主题 | 结论 | 后续动作 |
|------|--------|----------|------|----------|
| 2026-10-24 | Lin Shen（Owner） | Q-1 风险等级分级口径来源 | 已澄清：以 useriam AuthTrustLevel（`#authtrustlevel8`）作为机主认证安全等级分级口径，随指令下发，执行侧消费判定结果 | 在 spec/design 阶段固化精确映射 |
| —（Q-2 待 MR 确认后回填） | Owner + 开发者 | Q-2 API 签名/权限/错误码 | 待澄清 | 以 MR 36745 现状态为准固化 |
| —（Q-3 待安全基线确认后回填） | 安全基线 + Owner | Q-3 锁屏 ATL 阈值/豁免 | 待澄清 | 设计阶段与安全基线确认 |

| 方案 | 核心思路 | 优势 | 风险/代价 | 结论 |
|------|----------|------|-----------|------|
| A-1 | 复用既有双 challenge HMAC 模式派生任务凭证 | 增量改动，兼容既有 ticket 链路 | HMAC 派生需与 IAM challenge 语义一致 | 推荐（基线假设） |
| A-2 | 独立新建凭证体系（不复用 ticket/HMAC） | 与既有链路完全解耦 | 迁移/兼容成本高，引入重复凭证路径 | 备选（除非 A-1 验证失败） |

### 检索与影响分析

| 来源 | 查询/读取内容 | 关键发现 | 可信度 | 用于 | 命中/未查询原因 |
|------|---------------|----------|--------|------|-----------------|
| 本仓既有代码基线 | 代码基线 `7e110901`：`services/secure_access_fence/core_service/src/`（ticket_operation）、`frameworks/secure_access_fence/inner_api/ipc/` | 基线已有双 challenge HMAC、IPC 序列化、CXX 桥；task identity 能力（含 LRU 32 存储、IAM 桥）为本次新增，可在基线基础上扩展 | 高 | 范围/API/设计/测试 | 命中 |
| 接口声明 MR | `interface_sdk-js` MR 36745 | SystemAPI + 1 个任务有效性校验 InnerApi | 中 | API/范围 | 命中（待现状态） |
| IAM(useriam) | `UserAccessCtrlClient::VerifyAuthToken` | 返回 challenge + ATL，可消费 | 中 | 设计 | 命中（依赖已申请） |
| IAM(useriam) ATL 分级口径 | useriam `AuthTrustLevel` 文档（`#authtrustlevel8`，`js-apis-useriam-userauth`） | 机主认证安全等级分级口径来源，`AuthTrustLevel` 定义 ATL 枚举，作为"指令风险等级 × ATL"管控矩阵的 ATL 轴依据 | 高 | 范围/设计 | 命中（来源已确认） |

| 维度 | 澄清结论 | 确认人 | 状态 |
|------|----------|--------|------|
| 子系统与仓 | `security_asset`（主仓）+ `interface_sdk-js`（声明仓，`https://gitcode.com/openharmony/interface_sdk-js`）；SAF 子系统范围 | Lin Shen | 已确认 |
| API 与权限 | SystemAPI 变更 + 1 个任务有效性校验 InnerApi；复用既有权限不新增 | Lin Shen | 已确认 |
| 兼容性与非功能 | 保持既有 ticket 链路行为与错误码语义不变；性能 review-required | Lin Shen | 已确认 |
| 依赖与风险 | IAM VerifyAuthToken 新增依赖 + SELinux binder_call；风险分级口径以 useriam `AuthTrustLevel` 为准 | Lin Shen | 已确认 |

### 澄清结论

- [x] Q-1 已关闭（分级口径来源 = useriam `AuthTrustLevel`）；Q-2/Q-3 留待 spec/design 阶段关闭
- [x] 范围、API、兼容性、非功能需求、依赖和风险已形成基线结论
- [x] complex/critical 已记录方案取舍（A-1 推荐 / A-2 备选）和确认证据（功能设计评审，Lin Shen 确认）
- [x] P0/P1 AC 覆盖正常、异常和边界路径

**结论：** 通过（基线候选已完成，需求方/Owner Lin Shen 确认，待最终批准 Define）

> 基线内容形成后，将 `phase_status` 置为 `awaiting_approval` 并停止。只有需求方/Owner/SIG 明确批准 Define，且 `approval` 中的 status、approver、evidence、approved_at、baseline_digest 均已填写并与当前「需求基线」一致后，才能将 `phase_status` 置为 `approved` 并进入 Specify。回答澄清问题、表示没有补充、`ReadyForReview` 或 AI 自评均不等于批准。

> 基线候选内容固定后，运行 `ohos-sdd approval-digest <change>`，将输出的 64 位摘要填写到 `approval.baseline_digest`。该命令与 validator 共用同一摘要算法。需求基线发生任何实质变更后，必须将 `phase_status` 改回 `awaiting_approval`、重新生成摘要并取得批准。

> `需求输入` 的"Define 阶段状态"是面向人工阅读的镜像字段，必须与 frontmatter 的 `phase_status` 保持一致（状态名称可使用 `Clarifying`、`AwaitingApproval`、`Approved`、`ChangesRequested`、`Blocked` 的展示写法）。