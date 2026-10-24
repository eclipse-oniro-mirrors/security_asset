# 特性规格

> 固化用户可见行为和验收标准。复杂交互、状态机、异常流程可补充 Gherkin 场景。

> **不应包含：** 内部实现流程、中间件调用链路、框架层类名/方法名、内部性能指标。这些内容属于 design.md。
> **THEN 边界：** 每条 AC 的 `THEN` 必须能通过终端用户行为，或 Public / System / InnerAPI 三层接口边界的返回值、回调、错误码、事件或可查询结果直接判定。

## 概述

| 属性 | 值 |
|------|-----|
| 特性名称 | 任务级身份凭证支撑 Agent 长时/定时任务在锁屏下执行 |
| 特性编号 | FEAT-20260905541319 |
| 所属 Epic | 无 |
| 优先级 | P1 |
| 目标版本 | OpenHarmony 7.1（引用 proposal.target_release） |
| SIG 归属 | 安全（IAM/interface_sdk-js 协同；SELinux 配置由 `selinux_adapter` 仓完成，不在本仓交付） |
| 状态 | Approved |
| 复杂度 | 复杂 |

## 本次变更范围（Delta）

> 本 spec 以代码基线 `7e1109015c5941f014d9cf0421f528351a3b9c07`（!521 merge master into master）为基础。基线已具备 ticket 授权链路（`requestToolPermissions` / `grantToolPermissionsByUser` / `verifyTicket` / `requestToolTicket` / 远程控制）；**任务级身份凭证（Task Identity）能力在基线中不存在，全部为本次新增**。对外 System/InnerAPI 行为与错误码语义以本 spec 为准。

| 类型 | 内容 | 说明 |
|------|------|------|
| ADDED | **任务级身份凭证能力**：任务身份生成（含机主认证 authToken 校验）/ 挑战生成 / 批量撤销 / 任务有效性检索，凭证跨会话有效、可过期、可撤销、可校验 | 供系统应用侧任务经办方为 Agent 长时/定时任务签发任务级授权凭证；授权从"会话"解耦到"任务" |
| ADDED | **锁屏执行管控**：任务凭证携带机主认证 ATL 结果与"允许锁屏执行"声明；锁屏态下发授权/执行时，按任务 ATL 对应的**锁屏拒绝权限策略**判定放行或拦截（ATL3/ATL4 为有效豁免强度，拒绝列表当前为空），未达标返回锁屏错误码 | 用户本人完成机主认证（ATL 达标）后，已授权长时/定时任务可在锁屏下执行或到点拉起 |
| MODIFIED | 既有 `requestToolPermissions` 授权链路扩展：携带 `taskIdentity` 时校验任务有效性与锁屏豁免放行；既有 `verifyTicket` 执行链路集成任务凭证有效性校验（一次性任务凭证执行后失效） | 对外既有行为与错误码语义保持兼容（未携带 taskIdentity 时行为完全不变） |
| REMOVED | 无（保持既有 `requestToolPermissions` / `verifyTicket` / ticket 链路的对外行为与错误码语义不变） | 兼容性约束，见「兼容性声明」 |

## 输入文档

| 文档 | 路径 | 状态 |
|------|------|------|
| Requirement | `.codespec/changes/20260905541319/proposal.md` | Approved |
| API 声明（权威） | `D:\@ohos.abilityToolAccessCtrl.d.ts` | 签名与错误码事实来源 |
| 代码基线 | `7e1109015c5941f014d9cf0421f528351a3b9c07`（!521 merge master into master） | 基线事实基准；task identity 能力在基线不存在 |

> 需求基线、不涉及项、受影响子系统与仓库详见 proposal.md，本文档不重复摘录。

## 核心术语定义

> 本术语表是本 spec 的 AC、规则、验证映射统一引用的事实来源，正文不重复定义。

### ATL（AuthTrustLevel，机主认证结果信任等级）

> ATL 是 IAM(useriam) `AuthTrustLevel` 枚举返回的机主认证安全等级，随机主认证（锁屏解锁/生物识别）结果返回，经 `UserAccessCtrlClient::VerifyAuthToken` 校验收敛。本特性将其作为「指令风险等级 × ATL」锁屏管控矩阵的判定轴与**锁屏豁免强度依据**（见 R-7）；任务凭证签发（高危路径）时绑定该 ATL 值到 `taskAttributes.atl`。

| ATL | 数值 | 定义与安全含义 | 典型适用场景 |
|-----|------|----------------|--------------|
| ATL1 | 10000 | 认证结果的信任等级级别1：表示该认证方案能够识别用户个体，具备基本的活体检测能力。安全强度较低，认证结果可能存在一定风险。 | 业务风控、一般个人数据查询、非敏感信息访问等低安全场景。建议配合其他安全措施使用。 |
| ATL2 | 20000 | 认证结果的信任等级级别2：表示该认证方案能够精确识别用户个体，具备标准的活体检测能力。安全强度中等，可有效防御简单的伪造攻击。 | 维持设备解锁状态、应用登录、一般敏感操作确认等中等安全场景。 |
| ATL3 | 30000 | 认证结果的信任等级级别3：表示该认证方案能够精确识别用户个体，具备较强的活体检测能力。安全强度较高，可有效防御照片、视频等常见伪造攻击。 | 设备解锁、重要敏感操作确认、企业级应用登录等较高安全场景。 |
| ATL4 | 40000 | 认证结果的信任等级级别4：表示该认证方案能够高精度识别用户个体，具备很强的活体检测能力。安全强度最高，可有效防御各类高级伪造攻击。 | 小额支付、金融交易、高敏感数据访问等高安全场景。仅少数高安全认证方案可支持此等级。 |

> **本特性 ATL 口径：** 锁屏豁免有效机主认证强度 = **ATL3(30000)/ATL4(40000)**（C-5 已确认最终规格，当前两等级对应拒绝列表为空 → 放行）；`ATL<=0`（如低危路径未携 authToken，`taskAttributes.atl=0`）不构成豁免（R-7 拦截）。

## 用户故事

### US-1: 用户本人授权 Agent 执行长时/定时任务（任务级身份凭证签发与撤销）

**作为** 系统开发者（任务经办方，SA/插件仓系统应用调用方）,
**我想要** 为 Agent 长时/定时任务生成任务级身份凭证（含挑战、机主认证绑定、过期与撤销机制）,
**以便** 授权从"会话"解耦到"任务"，用户授权后无需持续在场.

**正常流程步骤（供测试脚本编排）：**

1. 系统应用持 `ohos.permission.QUERY_TOOL_PERMISSIONS` 调用 `generateTaskChallenge(taskIdentityInfo)`（taskId 非空、executeType=INSTANT/DELAYED）。
2. 高危路径：用户在 10 分钟（boot-time）内完成 IAM 机主认证，系统应用携带 authToken 调用 `generateTaskIdentity(taskIdentityInfo)`；低危路径：直接调用 `generateTaskIdentity` 且不携带 authToken。
3. 断言 resolve 的 `TaskIdentity{taskAttributes, identity, challenge}`：identity/challenge 均为非空 base64；高危路径 taskAttributes 内含 atl。
4. 调用 `revokeTaskIdentity([taskId])` 断言 resolve（幂等）。
5. 断言撤销后对该 taskId 的校验/检索返回 24010002（内部 NOT_FOUND）。

**验收标准（AC）：**

| AC编号 | 验收标准 | 可观察表面 | 类型 |
|--------|----------|--------------|------|
| AC-1.1 | WHEN 系统应用持 `ohos.permission.QUERY_TOOL_PERMISSIONS` 调用 `generateTaskChallenge(taskIdentityInfo)` 且 taskId 非空、executeType=INSTANT 或 DELAYED、allowScreenLockExecution 为布尔值 THEN Promise resolve 为 32 字节 `Uint8Array` challenge | System API 返回值 | 正常 |
| AC-1.2 | WHEN 系统应用调用 `generateTaskIdentity(taskIdentityInfo)` 且未携带 authToken（低危路径） THEN Promise resolve 为 `TaskIdentity{taskAttributes, identity, challenge}`，其中 identity/challenge 为非空 base64 值 | System API 返回值 | 正常 |
| AC-1.3 | WHEN 系统应用调用 `generateTaskIdentity(taskIdentityInfo)` 且携带 authToken（高危路径），IAM `VerifyAuthToken` 返回的挑战值与**待认证挑战值（pending challenge）**不匹配 THEN reject BusinessError 24010002（内部 `TASK_IDENTITY_MISMATCH`） | System API 错误码 | 异常 |
| AC-1.4 | WHEN 高危路径未先生成挑战（待认证挑战值 pending challenge 不存在或超时） THEN reject BusinessError 24010002（内部 `TASK_IDENTITY_NOT_FOUND`） | System API 错误码 | 异常 |
| AC-1.5 | WHEN 系统应用调用 `revokeTaskIdentity(taskIds)`（1~100 个、非空、无路径穿越字符） THEN Promise resolve；随后对该 taskId 的校验返回非成功（内部 `TASK_IDENTITY_NOT_FOUND` → 对外 24010002） | System API 返回值/错误码 | 正常 |
| AC-1.6 | WHEN 调用方未持 `QUERY_TOOL_PERMISSIONS` THEN reject 201；WHEN 非系统应用调用 system API THEN reject 202 | System API 错误码 | 异常 |
| AC-1.7 | WHEN taskId 为空、executeType 非 INSTANT/DELAYED、authToken 超 4096 字节、allowableDuration 非法 THEN reject 24010000 | System API 错误码 | 边界 |

### US-2: 链上鉴权——requestToolPermissions 携带任务凭证时锁屏下放行

**作为** 系统开发者 / 设备机主,
**我想要** 发起工具权限授权（ticket 生成）时，携带任务身份凭证且机主已完成足够强度认证（ATL）时，锁屏态不再被拦截,
**以便** 用户本人授权后的长时/定时任务可在锁屏下执行.

> **锁屏豁免条件（本 US 核心）：** 当 `requestToolPermissions` 携带 `taskIdentity` 且其 `taskAttributes` 声明 `allowScreenLockExecution=true`、携带有效 `authToken`（高危路径签发）时，按任务凭证中 **ATL**（分级定义见「核心术语定义」）对应的**锁屏拒绝权限策略**判定放行/拦截。**ATL3(30000)/ATL4(40000) 是有效的可豁免机主认证强度，当前两者对应的拒绝权限列表为空**——此时请求的权限不在拒绝列表即放行；`policyFound=false`（该 ATL 无锁屏策略）或 ATL<=0 时一律不放行。

**正常流程步骤（供测试脚本编排）：**

1. 高危路径签发任务凭证（ATL3(30000)/ATL4(40000)、allowScreenLockExecution=true），并持有该 `TaskIdentity`。
2. 使设备进入锁屏态。
3. 调用 `requestToolPermissions(permissionQuery)`，`permissionQuery.taskIdentity` 携带该凭证，请求的权限不在该 ATL 拒绝权限列表（当前为空）。
4. 断言调用成功：不返回 24010006，正常进入授权结果与 ticket 生成流程。
5. 断言消息内 taskIdentity 具备放行条件（见 AC-2.1~2.3 矩阵）。

| AC编号 | 验收标准 | 可观察表面 | 类型 |
|--------|----------|--------------|------|
| AC-2.1 | WHEN 调用 `requestToolPermissions(permissionQuery)`，`permissionQuery.taskIdentity` 携带合法任务凭证（`allowScreenLockExecution=true` + 有效 authToken + ATL=30000 或 40000），设备处于锁屏态，且请求的权限不在该 ATL 的锁屏拒绝权限列表中（当前拒绝列表为空） THEN 调用不返回锁屏错误（`OPERATION_FAILED_UNDER_SCREEN_LOCK`/24010006），正常进入授权结果与 ticket 生成流程 | System API 返回值/错误码 | 正常 |
| AC-2.2 | WHEN 上述条件满足但设备处于锁屏态且 `taskIdentity` 未携带（空）、或 `allowScreenLockExecution=false`、或 authToken 无效、或 ATL<=0、或该 ATL 无锁屏策略（policyFound=false） THEN 调用返回 24010006，不生成 ticket | System API 错误码 | 异常 |
| AC-2.3 | WHEN `requestToolPermissions` 携带任务凭证但所请求权限命中该 ATL 的锁屏拒绝权限列表 WHEN 该列表非空 THEN 调用返回 24010006，不生成 ticket；WHEN 列表为空 THEN 放行（与 AC-2.1 一致） | System API 错误码 | 边界 |
| AC-2.4 | WHEN 未携带 taskIdentity 的既有调用（基线行为） THEN 保持基线锁屏拦截语义不变：需解锁的 CLI/权限在锁屏态返回 24010006，无需解锁的正常放行 <!-- ext-ok: 此处"解锁/未解锁"指设备屏幕锁定状态（needUnlockScreen 终端用户可观察语义），非内部加锁/互斥锁 --> | System API 错误码 | 兼容 |

### US-3: 锁屏态下任务执行/到点拉起（verifyTicket 链上校验任务有效性）

**作为** 设备机主 / 系统开发者,
**我想要** 已授权任务在执行校验（verifyTicket 链路，含 cliManager.execTool 执行入口）时校验任务有效性与锁屏豁免,
**以便** 预授权且 ATL 达标的任务在锁屏态下可继续执行或到点拉起.

> **verifyTicket 链上的可观察语义：** 任务执行侧通过 `verifyTicket`（System/InnerAPI 接口）校验已授权任务的凭证有效性。携带任务凭证的校验通过意味着：凭证未过期、未撤销、签名校验一致、调用方 token 匹配；**一次性（INSTANT）任务凭证校验并执行后不可再次通过该校验，延时（DELAYED）任务凭证可重复通过校验**。任务凭证无效时整个校验失败，任务不执行。

**正常流程步骤（供测试脚本编排）：**

1. 已授权任务（DELAYED 或 INSTANT，锁屏态/解锁态）执行调用 `verifyTicket`，消息携带 taskIdentity。
2. 断言校验通过（返回 CliInfo 等正常结果）；`INSTANT` 凭证校验并执行后，再次调用同一凭证断言校验失败（24010002）。
3. 对 `DELAYED` 凭证重复调用同一凭证断言可重复通过校验。
4. 撤销/过期/篡改凭证后调用断言校验失败（24010002）。
5. 消息声明 `needUnlockScreen=true` 且设备锁屏、凭证 ATL 未达标时，断言返回 24010006。

| AC编号 | 验收标准 | 可观察表面 | 类型 |
|--------|----------|--------------|------|
| AC-3.1 | WHEN 已授权任务执行调用 `verifyTicket` 且消息携带 `taskIdentity`，凭证有效（未过期、未撤销、签名校验通过、callerToken 匹配） THEN 校验通过；**一次性（INSTANT）任务凭证在校验通过并执行后不可再次通过该校验**，延时（DELAYED）任务凭证可重复通过校验 | System API 返回值 | 正常 |
| AC-3.2 | WHEN `verifyTicket` 携带的 `taskIdentity` 无效（已撤销/已过期/篡改/或一次性任务已执行过） THEN 校验返回失败（对外 24010002），任务不执行 | System API 错误码 | 异常 |
| AC-3.3 | WHEN 任务消息声明 `needUnlockScreen=true`（需要解锁执行）且设备锁屏，且凭证内 ATL 不足以豁免（taskAttributes 未声明 allowScreenLockExecution 或 ATL 不达标） THEN 校验返回 24010006（`ScreenIsLocked`） | System API 错误码 | 异常 |
| AC-3.4 | WHEN `verifyTicket` 消息携带 taskIdentity 但 callerToken 为空或为 0 THEN 校验返回参数错误（24010000，内部 `InvalidArgument`） | System API 错误码 | 边界 |
| AC-3.5 | WHEN `taskExpireTime=-1`（仅 DELAYED 永不过期）且任务未撤销，锁屏态且 ATL 达标 THEN 到点拉起时校验通过（时间检查不干扰） | System API 返回值 | 边界 |

### US-4: 任务有效性检索（InnerAPI 门禁）与未达认证强度管控

**作为** 设备机主 / 系统开发者,
**我想要** 系统侧可用任务有效性校验接口确认凭证是否有效（未过期、未撤销、校验通过），并确保未达认证强度的执行被拦截,
**以便** 凭证可验证、可过期、可撤销，安全底线不放松.

| AC编号 | 验收标准 | 可观察表面 | 类型 |
|--------|----------|--------------|------|
| AC-4.1 | WHEN 系统 SA 调用 InnerAPI `RetrieveAuthTokenByTaskId(callerToken, taskId)`，凭证未过期、HMAC 校验通过、callerToken 可解析 os_account_id THEN 返回该任务关联的 authToken 字节 | InnerAPI 返回值 | 正常 |
| AC-4.2 | WHEN 调用 `RetrieveAuthTokenByTaskId` 的调用方非系统 SA、或未持 `QUERY_TOOL_PERMISSIONS`、或未持 `INTERACT_ACROSS_LOCAL_ACCOUNTS` THEN 返回权限错误 201 | InnerAPI 错误码 | 异常 |
| AC-4.3 | WHEN 凭证已过期、已撤销或一次性（INSTANT）任务已执行 THEN `RetrieveAuthTokenByTaskId` 校验返回非成功（对外 24010002） | InnerAPI 错误码 | 异常 |
| AC-4.4 | WHEN `revokeTaskIdentity` 传入不存在的 taskId THEN Promise resolve（幂等）；重新以相同 taskId 生成后可正常检索 | System API 返回值 | 边界 |

**正常流程步骤（供测试脚本编排）：**

1. 系统 SA 调用 InnerAPI `RetrieveAuthTokenByTaskId(callerToken, taskId)`（凭证未过期、HMAC 校验通过、callerToken 可解析 os_account_id）。
2. 断言返回该任务关联的 authToken 字节。
3. 非系统 SA / 缺 `QUERY_TOOL_PERMISSIONS` 或 `INTERACT_ACROSS_LOCAL_ACCOUNTS` 时调用，断言返回 201。
4. 凭证过期/撤销/篡改/INSTANT 已执行后调用，断言返回 24010002（对外映射）。

> **待确认项（关联 proposal Q-3）：** ATL3(30000)/ATL4(40000) 之外是否引入更高/更低 ATL 的锁屏豁免，以及拒绝权限列表的内容，由安全侧基线（useriam `AuthTrustLevel`，分级定义见「核心术语定义」）在 design 阶段细化；spec 固化当前可观察行为（ATL3/4 豁免、拒绝列表为空 → 放行）。

## 验收追溯

> 本表只维护 Spec 内部的 AC → 规则 → 可观察表面事实。AC → Task 映射由 `execution-plan.md` 维护；测试入口、Red 条件和通过标准由「验证映射」维护；实际执行证据由 `evidence/reviews/` 维护。

| AC | 关联规则 | 可观察表面 |
|----|----------|--------------|
| AC-1.1 | R-1 | System API 返回值（`Promise<Uint8Array>`） |
| AC-1.2 | R-2 | System API 返回值（`Promise<TaskIdentity>`） |
| AC-1.3 | R-3 | System API 错误码 24010002 |
| AC-1.4 | R-3 | System API 错误码 24010002 |
| AC-1.5 | R-4 | System API 返回值 / 错误码 24010002 |
| AC-1.6 | R-5 | System API 错误码 201 / 202 |
| AC-1.7 | R-6 | System API 错误码 24010000 |
| AC-2.1 | R-7, R-9 | System API 返回值（授权结果与 ticket 生成成功） |
| AC-2.2 | R-7 | System API 错误码 24010006 |
| AC-2.3 | R-7 | System API 错误码 24010006 / 放行 |
| AC-2.4 | R-7, R-10 | System API 错误码（基线兼容） |
| AC-3.1 | R-2, R-9 | System API 返回值（校验通过）；一次性任务凭证执行后不可再次通过（可观察） |
| AC-3.2 | R-9 | System API 错误码 24010002 |
| AC-3.3 | R-7, R-9 | System API 错误码 24010006 |
| AC-3.4 | R-6, R-9 | System API 错误码 24010000 |
| AC-3.5 | R-2, R-7 | System API 返回值 |
| AC-4.1 | R-2, R-4 | InnerAPI 返回值（authToken 字节） |
| AC-4.2 | R-5 | InnerAPI 错误码 201 |
| AC-4.3 | R-2, R-9 | InnerAPI 错误码 24010002 |
| AC-4.4 | R-4 | System API 返回值 |

## 规则定义

> 统一规则表。类型标签：**行为**（正常路径）、**边界**（输入/状态的临界点）、**异常**（非法输入或异常状态）、**恢复**（系统异常后的恢复策略）。

| 规则ID | 类型 | 触发条件 | 预期行为 | 边界/约束 | 关联AC |
|--------|------|----------|----------|-----------|--------|
| R-1 | 行为 | 系统应用调用 `generateTaskChallenge`，入参合法 | 生成 32 字节随机 challenge，Promise resolve `Uint8Array`；同一 (caller,taskId) 重复调用覆盖旧 challenge | `Uint8Array` 长度=32；challenge 以 (caller, taskId) 为 key 存储 | AC-1.1 |
| R-2 | 行为 | 正向路径执行：低危不携 authToken / 高危携 authToken 且 IAM 校验通过；凭证校验（检索/执行）有效 | 生成 `TaskIdentity{taskAttributes, identity, challenge}`；taskExpireTime 存储时转为绝对 epoch 秒（-1 表示永不过期，仅 DELAYED）；检索/执行校验时按过期/撤销/篡改判定 | 双 challenge HMAC；identity=base64(HMAC_SHA256(taskAttributes+taskId+challenge2, session_key))；challenge=base64(ch1+ch2)；taskAttributes 含 callerTokenId + authToken(base64) + atl | AC-1.2, AC-3.1, AC-3.5, AC-4.1, AC-4.3 |
| R-3 | 异常 | 高危路径 IAM `VerifyAuthToken` 返回的挑战值与待认证挑战值（pending challenge）不匹配，或待认证挑战值不存在/超时 | 拒绝签发：对外 24010002（内部 `TASK_IDENTITY_MISMATCH` / `TASK_IDENTITY_NOT_FOUND`） | 待认证挑战值 10 分钟（boot-time）有效 | AC-1.3, AC-1.4 |
| R-4 | 行为 | `revokeTaskIdentity(taskIds)` / `RetrieveAuthTokenByTaskId` / 重新生成同 taskId | 撤销：Promise resolve，删除对应记录，幂等（不存在的 taskId 不报错）；检索：返回 authToken 字节；撤销后再生成同 taskId 可正常检索 | taskIds 1~100 个、非空、无路径穿越字符（10 种 pattern） | AC-1.5, AC-4.1, AC-4.4 |
| R-5 | 异常 | 调用方权限不足或非系统应用 | 权限不足返回 201；非系统应用调用 system API 返回 202；检索类调用非系统 SA/缺权限返回 201 | `QUERY_TOOL_PERMISSIONS`；检索额外需 `INTERACT_ACROSS_LOCAL_ACCOUNTS` | AC-1.6, AC-4.2 |
| R-6 | 边界 | 入参非法：taskId 空/executeType 非法/authToken 超 4096/allowableDuration 非法/callerToken 空或 0 | 拒绝并返回 24010000 | authToken ≤4096；**allowableDuration 约束为 (0,60]（已确认，声明文件口径即最终约束）**；callerToken 空或 "0" 视为非法 | AC-1.7, AC-3.4 |
| R-7 | 行为 | **锁屏豁免判定**（requestToolPermissions 发授权与 verifyTicket 执行均涉及；仅当任务凭证声明允许锁屏执行时进入判定） | **放行路径**：任务凭证 `allowScreenLockExecution=true` 且 ATL 为该 ATL 的有效豁免强度（当前 ATL3(30000)/ATL4(40000)），且该 ATL 存在锁屏策略（policyFound=true），且请求的权限不落在该 ATL 锁屏拒绝权限列表中（当前两者拒绝列表为空），则锁屏态下授权/执行放行。**拦截路径**：ATL<=0、或 policyFound=false（该 ATL 无锁屏策略）、或请求权限命中拒绝列表时，返回 24010006（`OPERATION_FAILED_UNDER_SCREEN_LOCK`，内部 `0x22002`），不生成 ticket/不通过校验，不产生副作用 | 豁免条件必须全部满足才放行：`allowScreenLockExecution=true` + ATL>0 + policyFound + 权限不在拒绝列表；任一不满足即拦截。**当前 ATL3(30000)/ATL4(40000) 为有效可豁免强度且拒绝列表为空**；任务凭证未携带（基线调用）或不含豁免声明时按基线 `needUnlockScreen` 语义处理（AC-2.4/R-10） | AC-2.1, AC-2.2, AC-2.3, AC-2.4, AC-3.3 |
| R-8 | 恢复 | 任务凭证的有效期与状态在系统空闲/重启后仍被可靠维护：过期的一次性（INSTANT）任务凭证与过期挑战在重启后不被错误放行 | 过期/已撤销/一次性已执行的任务凭证在任何时刻校验都不通过（包括系统空闲回收或重启后），不出现"已失效凭证被重新放行" | 凭证与挑战的存储/回收实现属内部资源管理（见 design.md 状态归属与不变量）；spec 只约束"对外可观察的有效性判定稳定不变" | AC-3.1, AC-3.2 |
| R-9 | 行为 | **链上任务校验的对外行为**：requestToolPermissions 携带 taskIdentity（授权前校验任务凭证有效性）；verifyTicket 消息携带 taskIdentity（执行前校验任务凭证有效性） | 校验通过则继续授权/执行流程；校验失败则整体失败（对外 24010002；参数错误 24010000），不产生授权/执行副作用 | **一次性（INSTANT）任务凭证校验通过并执行后不可再次通过校验，延时（DELAYED）任务凭证可重复通过校验**；携带无效任务凭证（已撤销/过期/篡改/一次性已执行）时授权与执行的校验均失败 | AC-2.1, AC-3.1, AC-3.2, AC-3.4, AC-4.3 |
| R-10 | 行为 | `requestToolPermissions` 未携带 taskIdentity（基线既有调用） | 保持基线行为与错误码语义不变：需解锁的 CLI/权限在锁屏态按既有 `needUnlockScreen` 判定返回 24010006；无需解锁的正常放行 | 兼容性约束，不因本次变更改变触发条件 | AC-2.4 |

## 验证映射

> 每个 AC 至少关联一个可执行测试入口。`Red 条件` 描述实现前测试为什么必然失败。
> **保证方式分工：** JS/ArkTS 入口的接口（System API：`generateTaskChallenge`/`generateTaskIdentity`/`revokeTaskIdentity`/`requestToolPermissions`/`verifyTicket`）通过 **XTS 测试用例或测试 HAP** 保证；C++ 入口的 InnerAPI（`RetrieveAuthTokenByTaskId`）通过 **TDD 单测（HWTEST_F）** 保证。

| 编号 | 对应规格项 | 测试入口 | 验证方式（保证手段） | Red 条件（实现前失败信号） | 通过标准 |
|------|------------|----------|----------------------|---------------------------|----------|
| VM-1 | R-1 / AC-1.1 | XTS 用例 / 测试 HAP：generateTaskChallenge | XTS 或测试 HAP（JS 入口） | 尚无 generateTaskChallenge 能力时，调用返回 24010000/异常错误码而非 32 字节 challenge；或 challenge 长度 ≠32 | Promise resolve，challenge 长度=32，重复调用同 (caller,taskId) 覆盖 |
| VM-2 | R-2 / AC-1.2 | XTS 用例 / 测试 HAP：generateTaskIdentity 低危 | XTS 或测试 HAP（JS 入口） | 低危路径不存在时 no-authToken 调用被拒/无返回；或返回 identity/challenge 为空 | resolve `TaskIdentity`，identity 非空 base64(HMAC)，challenge 解码 64 字节 |
| VM-3 | R-3 / AC-1.3 | XTS 用例：高危 IAM challenge 不匹配（可注入 IAM mock） | XTS 或测试 HAP（JS 入口） | 未实现比对逻辑时，错误 challenge 被接受（返回 identity 而非 mismatch） | reject 24010002（内部 MISMATCH） |
| VM-4 | R-3 / AC-1.4 | XTS 用例：无待认证挑战值路径 | XTS 或测试 HAP（JS 入口） | 未做待认证挑战值前置校验时，无挑战值直接签发成功 | reject 24010002（内部 NOT_FOUND） |
| VM-5 | R-5 / AC-1.6 | XTS 用例：无权限/非系统应用调用 | XTS 或测试 HAP（JS 入口） | 未做权限校验时无权限调用成功 | 201/202 |
| VM-6 | R-6 / AC-1.7 | XTS 用例：参数边界 | XTS 或测试 HAP（JS 入口） | 未校验边界时非法参数被接受 | 24010000，且有精确触发条件 |
| VM-7 | R-4 / AC-1.5, AC-4.4 | XTS 用例：撤销后检索 + 幂等 + 重生成 | XTS 或测试 HAP（JS 入口） | 撤销未删除记录时撤销后仍可检索到；或撤销不存在 taskId 报错 | resolve；随后检索返回 NOT_FOUND→24010002；重生成同 taskId 正常检索 |
| VM-8 | R-7,R-9 / AC-2.1 | XTS 用例 / 测试 HAP：generateTaskIdentity(高危 ATL3/4) → requestToolPermissions 携带 taskIdentity → 锁屏态授权 | XTS 或测试 HAP（JS 入口） | 未实现 ATL 锁屏豁免时，锁屏态 requestToolPermissions 携带合法任务凭证仍返回 24010006 | 调用成功进入授权+ticket 流程，无 24010006；请求权限不在 ATL 拒绝列表 |
| VM-9 | R-7 / AC-2.2, AC-2.3 | XTS 用例：矩阵（无 taskIdentity / allowScreenLockExecution=false / ATL<=0 / policyFound=false / 命中拒绝列表） | XTS 或测试 HAP（JS 入口） | 未实现锁屏豁免判定时，具备豁免条件与否都被同样处理（无区分） | 仅豁免条件全部满足时放行；否则 24010006，无 ticket |
| VM-10 | R-7,R-10 / AC-2.4 | XTS 回归用例：既有 requestToolPermissions（未携带 taskIdentity）锁屏行为 | XTS 回归 | 基线既有测试在锁屏态需解锁权限时返回异常或行为漂移 | 与基线一致：需解锁的在锁屏态返回 24010006，无需解锁的正常放行 |
| VM-11 | R-9 / AC-3.1, AC-3.2 | XTS 用例 / 测试 HAP：verifyTicket 携带 taskIdentity（合法 / 一次性任务已执行 / 已撤销） | XTS 或测试 HAP（JS 入口） | 未实现任务凭证校验时，verifyTicket 携带非法/一次性已执行的凭证仍通过 | 有效 DELAYED 校验通过；INSTANT 校验通过并执行后重复校验失败；已撤销→24010002 |
| VM-12 | R-9 / AC-3.4 | XTS 用例：verifyTicket 携带 taskIdentity 但 callerToken 空/0 | XTS 或测试 HAP（JS 入口） | 未做 callerToken 校验时空 token 被接受 | 24010000（内部 InvalidArgument） |
| VM-13 | R-7,R-9 / AC-3.3, AC-3.5 | XTS 用例 / 测试 HAP：锁屏态 verifyTicket（DELAYED/-1，ATL 达标） | XTS 或测试 HAP（JS 入口） | 锁屏执行豁免未实现时，锁屏态校验被拦截 | 锁屏态校验通过、任务可执行/拉起 |
| VM-14 | R-2,R-9 / AC-4.1, AC-4.3 | C++ TDD：`saf_agent_fence_retrieve_auth_token_test.cpp`（15 用例） | TDD 单测（HWTEST_F，C++ InnerAPI 入口） | 检索路径缺失时返回错误而非 authToken 字节；过期/撤销/篡改凭证仍返回 | 有效凭证返回 authToken 字节；过期/撤销/篡改→24010002 |
| VM-15 | R-5 / AC-4.2 | C++ TDD：RetrieveAuthTokenByTaskId 权限 | TDD 单测（HWTEST_F，C++ InnerAPI 入口） | 未做系统 SA/权限校验时任意调用成功 | 201 |
| VM-16 | R-7,R-9 / AC-2.1, AC-3.1（性能对照实验） | benchmark：锁屏态 vs 解锁态下的 `requestToolPermissions`（携带 taskIdentity 锁屏豁免）与 `verifyTicket`（锁屏拉起）时延对照 | benchmark（≥100 次采样 / AndroidX 同口径或 hilog 计时桩） | 无对照基线或未采集冷路径时无法证明劣化 <150ms | 锁屏拉起校验时延相对解锁态基线**最大劣化 ≤150ms**；锁屏态单次拉起校验 P95 ≤250ms |

## 测试数据样例

> 供 XTS 用例、测试 HAP 与 C++ TDD 直接参数化的样例数据。所有样例会表达真实调用场景与精确错误码，与「规则定义」/「接口规格」一致；样例本身不构成对外契约，仅作为测试脚本编排的输入基线。

### 正常流程样例

| # | 场景 | 输入样例 | 预期输出/断言 |
|---|------|----------|---------------|
| TD-1 | 低危签发 | `generateTaskIdentity({ taskId: "agent_task_123", taskExpireTime: 3600, executeType: ExecuteType.DELAYED, allowScreenLockExecution: true })`（不携 authToken） | resolve `TaskIdentity`；identity 为非空 base64(HMAC)，challenge 解码后 64 字节 |
| TD-2 | 高危签发（ATL3） | `generateTaskChallenge(...)` → IAM 认证（ATL=30000）→ `generateTaskIdentity({ ..., authToken: <32B> })` | resolve `TaskIdentity`；taskAttributes 含 atl=30000 |
| TD-3 | 高危 challenge 不匹配 | 挑战值 A 生成后，以 IAM 返回挑战值 B 携 authToken 调用 `generateTaskIdentity` | reject 24010002（内部 MISMATCH） |
| TD-4 | 撤销 | `revokeTaskIdentity(["agent_task_123"])` | resolve（幂等）；随后检索/校验返回 24010002 |
| TD-5 | 锁屏放行（AC-2.1） | TD-2 凭证 + `permissionQuery.taskIdentity` 携带 + 设备锁屏 + 请求权限不在 ATL3 拒绝列表 | 正常进入授权+ticket 流程，无 24010006 |
| TD-6 | 锁屏拦截（AC-2.2） | 同 TD-5 但 `allowScreenLockExecution=false` 或 ATL<=0 | 24010006，不生成 ticket |
| TD-7 | verifyTicket 携带凭证 | TD-2 凭证（DELAYED）经 verifyTicket 消息携带 | 校验通过，返回 CliInfo；重复调用仍通过 |
| TD-8 | INSTANT 一次性 | TD-1 但 `executeType: INSTANT` + verifyTicket 校验通过并执行 | 执行后再次对同一凭证 verifyTicket 失败（24010002） |

### 错误码触发样例

| 错误码 | 触发方式 | 关联 AC |
|--------|----------|---------|
| 201 | 调用方无 `QUERY_TOOL_PERMISSIONS`（或检索类无 `INTERACT_ACROSS_LOCAL_ACCOUNTS`） | AC-1.6, AC-4.2 |
| 202 | 非系统应用调用 System API | AC-1.6 |
| 24010000 | taskId 空、executeType=0/3、authToken 4097 字节、allowableDuration=0 或 61、callerToken 空或 "0" | AC-1.7, AC-3.4 |
| 24010002 | IAM challenge 不匹配 / 无 pending challenge / 凭证已撤销 / 已过期 / 篡改 / INSTANT 已执行 | AC-1.3, AC-1.4, AC-1.5, AC-3.2, AC-4.3 |
| 24010006 | 锁屏态未豁免（无 taskIdentity / allowScreenLockExecution=false / ATL<=0 / policyFound=false / 命中拒绝列表） | AC-2.2, AC-2.3, AC-3.3 |

## API 变更分析

> 开放级别：`Public`（三方应用）/ `System`（@systemapi，系统应用）/ `InnerAPI`（部件间接口，不面向应用）。

### 新增 API

| API 名称 | 开放级别 | 入参概要 | 返回值 | 错误码范围 | 功能描述 | Depends On（外部模块 API） | 关联 AC |
|----------|----------|----------|--------|------------|----------|---------------------------|---------|
| `generateTaskChallenge` | System | `TaskIdentityInfo` | `Promise<Uint8Array>` | 201/202/24010000/24010001/24010002 | 生成任务认证挑战值 | 无外部模块 API（本地 CSPRNG 生成）；不依赖 IAM | AC-1.1, AC-1.6, AC-1.7 |
| `generateTaskIdentity` | System | `TaskIdentityInfo` | `Promise<TaskIdentity>` | 201/202/24010000/24010001/24010002 | 为代理命令生成基于任务的身份认证身份（含机主认证 ATL 绑定） | **高危路径依赖**：`useriam_framework` 的 `UserAccessCtrlClient::VerifyAuthToken`（经本仓 CXX 桥）；低危路径不依赖 | AC-1.2, AC-1.3, AC-1.4, AC-1.6, AC-1.7, AC-2.1, AC-2.2, AC-3.5 |
| `revokeTaskIdentity` | System | `string[]`（taskId） | `Promise<void>` | 201/202/24010000/24010001/24010002 | 批量撤销任务认证标识 | 无外部模块 API（本地删除） | AC-1.5, AC-4.4 |
| `RetrieveAuthTokenByTaskId` | InnerAPI | `callerToken` + `taskId` | authToken 字节 | 201/24010000/24010001/24010002 | 任务有效性校验：校验任务凭证并返回关联 authToken，作为系统侧校验/放行条件 | 无外部模块 API（服务侧本地校验）；调用方权限复用既有 PERMISSION 体系 | AC-4.1, AC-4.2, AC-4.3 |

> **变更 API 的 Depends On：** 既有 `requestToolPermissions`（锁屏豁免分支）与 `verifyTicket`（任务校验分支）依赖：①插件（闭源）`GetAtlPermissionPolicy` 事件（锁屏拒绝权限策略来源）；②IAM `VerifyAuthToken` 认证结果（高危路径含 ATL）。未携带 taskIdentity 时无上述新增依赖。

### 变更/废弃 API

> 不涉及：本次变更无既有 API 的变更或废弃。既有 `requestToolPermissions` / `grantToolPermissionsByUser` / `requestToolTicket` / 远程授权 API 的对外行为与错误码语义**保持不变**（proposal 非目标约束），详见下方「兼容性声明」。

### API 与错误码事实

> 已有事实来源为权威声明文件 `D:\@ohos.abilityToolAccessCtrl.d.ts`（2026 现行版本）或本仓实现。新增事实标记 `New` 并引用 proposal/AC 作为依据。

| 事实项 | 事实类型 | 开放级别 | 精确签名/数值 | 触发条件 | 对外行为 | 事实来源（文件:行/符号或 New 依据） | 关联 AC |
|--------|----------|----------|---------------|----------|----------|--------------------------------------|---------|
| `generateTaskChallenge` | New API | System | `function generateTaskChallenge(taskIdentityInfo: TaskIdentityInfo): Promise<Uint8Array>` | 系统应用持 `QUERY_TOOL_PERMISSIONS`，taskId 非空、executeType 合法 | resolve 32 字节 `Uint8Array`；201/202/24010000/24010001/24010002 | `D:\@ohos.abilityToolAccessCtrl.d.ts:1297`；New 依据 `proposal.md` 目标 1/2（AC-1.1） | AC-1.1 |
| `generateTaskIdentity` | New API | System | `function generateTaskIdentity(taskIdentityInfo: TaskIdentityInfo): Promise<TaskIdentity>` | 系统应用持 `QUERY_TOOL_PERMISSIONS`；低危不携 authToken / 高危携 authToken | resolve `TaskIdentity{taskAttributes, identity, challenge}` | `D:\@ohos.abilityToolAccessCtrl.d.ts:1215`；New 依据 `proposal.md` 目标 1/2（AC-1.2） | AC-1.2 |
| `revokeTaskIdentity` | New API | System | `function revokeTaskIdentity(taskId: string[]): Promise<void>` | 系统应用持 `QUERY_TOOL_PERMISSIONS` | resolve（幂等） | `D:\@ohos.abilityToolAccessCtrl.d.ts:1236`；New 依据 `proposal.md` 目标 3（AC-1.5） | AC-1.5 |
| `TaskIdentityInfo` | New API | System | `interface TaskIdentityInfo { taskId: string; taskExpireTime: number; executeType: ExecuteType; allowScreenLockExecution: boolean; authToken?: Uint8Array; allowableDuration?: number }` | 生成身份/挑战时传入 | 定义任务身份输入 | `D:\@ohos.abilityToolAccessCtrl.d.ts:862`；New 依据 `proposal.md` 目标 1/2（AC-1.1） | AC-1.1 |
| `TaskIdentityInfo.allowScreenLockExecution` | New API | System | `boolean`（@since 26.1.0） | 生成任务身份时声明任务是否允许锁屏执行 | 允许锁屏执行标记 | `D:\@ohos.abilityToolAccessCtrl.d.ts:899`；New 依据 `proposal.md` 目标 2（AC-2.1） | AC-2.1 |
| `ExecuteType` | New API | System | `enum { INSTANT = 0x01, DELAYED = 0x02 }` | 任务执行类别输入 | 定义任务即时/延时类型 | `D:\@ohos.abilityToolAccessCtrl.d.ts:833`；New 依据 `proposal.md` 目标 1（AC-1.1） | AC-1.1 |
| `TaskIdentity` | New API | System | `interface TaskIdentity { taskAttributes: string; identity: string; challenge: string }` | 生成身份后的返回 | 任务身份凭证 | `D:\@ohos.abilityToolAccessCtrl.d.ts:794`；New 依据 `proposal.md` 目标 1（AC-1.2） | AC-1.2 |
| `RetrieveAuthTokenByTaskId` | New API | InnerAPI | `static int32_t RetrieveAuthTokenByTaskId(int32_t callerToken, const std::string &taskId, std::vector<int32_t> &authToken)` | 系统 SA 持 QUERY+INTERACT_ACROSS 权限；凭证未过期、HMAC 校验通过、callerToken 可解析 os_account_id | 校验通过返回 authToken 字节（输出参数）；权限不足 201；NotFound/Expired/Mismatch 对外映射 24010002 | `New`: 依据 `proposal.md` 目标 2（系统侧任务有效性校验 InnerApi，AC-4.1）；实现已落 `saf_agent_fence.h:236` | AC-4.1, AC-4.2, AC-4.3 |
| BusinessError 201 | Existing Error Code | System | `201` | 调用方无 `QUERY_TOOL_PERMISSIONS` | reject BusinessError 201 | `D:\@ohos.abilityToolAccessCtrl.d.ts:986/1203/1286`（既有声明错误码） | AC-1.6, AC-4.2 |
| BusinessError 202 | Existing Error Code | System | `202` | 非系统应用调用 system API | reject BusinessError 202 | `.d.ts:1205,1287` | AC-1.6 |
| BusinessError 24010000 | Existing Error Code | System | `24010000` | 参数无效（taskId 空/executeType 非法/allowableDuration 越界/callerToken 空或 0 等） | reject BusinessError 24010000 | `D:\@ohos.abilityToolAccessCtrl.d.ts:1206`（既有声明错误码，参数无效语义） | AC-1.7, AC-3.4 |
| BusinessError 24010001 | Existing Error Code | System | `24010001` | 服务异常（IPC failed 等） | reject BusinessError 24010001 | `D:\@ohos.abilityToolAccessCtrl.d.ts:1207` | AC-1.1, AC-1.6 |
| BusinessError 24010002 | Existing Error Code | System | `24010002` | 内部错误：任务相关内部码 NotFound/Expired/Mismatch 的对外统一映射 | reject BusinessError 24010002 | `D:\@ohos.abilityToolAccessCtrl.d.ts:992`；内部码见 `saf_result_defs.h:92/93/95`（`SAF_ERR_TASK_IDENTITY_NOT_FOUND 0x71005` / `EXPIRED 0x71006` / `MISMATCH 0x71008`） | AC-1.3, AC-1.4, AC-1.5, AC-3.1, AC-3.2, AC-4.3 |
| BusinessError 24010006 | Existing Error Code | System | `24010006` | 锁屏态下发授权/执行未被豁免（未标记 allowScreenLockExecution / ATL 不达标 / 权限命中拒绝列表） | reject BusinessError 24010006（设备锁定时不允许执行受保护操作） | `D:\@ohos.abilityToolAccessCtrl.d.ts:995/1188`（既有错误码）；对外映射源 `napi_common.cpp:52`（`OPERATION_FAILED_UNDER_SCREEN_LOCK=24010006`）、内部 `0x22002` | AC-2.2, AC-2.3, AC-2.4, AC-3.3 |

> **错误码事实注意：** 24010006（`OPERATION_FAILED_UNDER_SCREEN_LOCK`）是声明文件与 NAPI 映射中既有错误码（内部 `0x22002 ScreenIsLocked`），本次变更将其作为**锁屏豁免判定的对外错误码**，同时用于 `requestToolPermissions`（锁屏态发授权未豁免）与 `verifyTicket`（锁屏态执行未豁免）两条链路。内部码 0x71005/0x71006/0x71008（`SAF_ERR_TASK_IDENTITY_*`）为本次新增内部码（基线 `saf_result_defs.h` 仅含 `TICKET_OPERATION|1/2`），对外统一由 NAPI/InnerAPI 映射层转为 24010002。

## 接口规格

> 与「API 变更分析」（声明层清单）互补，本节定义接口调用语义、参数约束和行为场景。

### 接口定义

**abilityToolAccessCtrl.generateTaskChallenge**

| 属性 | 值 |
|------|-----|
| 函数签名 | `generateTaskChallenge(taskIdentityInfo: TaskIdentityInfo): Promise<Uint8Array>` |
| 返回值 | `Promise<Uint8Array>` — 32 字节随机挑战值 |
| 开放级别 | System |
| 错误码 | 201 / 202 / 24010000 / 24010001 / 24010002 |
| Depends On（外部模块 API） | 无 |
| 关联 AC | AC-1.1, AC-1.6, AC-1.7 |

**参数约束**

| 参数 | 类型 | 必填 | 默认值 | 约束条件 |
|------|------|------|--------|---------|
| taskIdentityInfo.taskId | string | 是 | — | 非空；无路径穿越字符 |
| taskIdentityInfo.taskExpireTime | number | 是 | — | 秒；-1 表示永不过期（仅 DELAYED），正数为持续时间 |
| taskIdentityInfo.executeType | ExecuteType | 是 | — | 仅 1(INSTANT)/2(DELAYED) |
| taskIdentityInfo.allowScreenLockExecution | boolean | 是 | — | 是否允许锁屏执行 |
| taskIdentityInfo.authToken | Uint8Array? | 否 | — | ≤4096 字节；高危路径携带 |
| taskIdentityInfo.allowableDuration | number? | 否 | — | 秒；约束为 (0,60]（已确认） |

**行为场景**

| # | 触发条件 | 可观察表面 | 预期行为 | 关联 AC |
|---|----------|--------------|----------|---------|
| 1 | 合法入参 + 持权限 | System API | resolve 32 字节 challenge | AC-1.1 |
| 2 | 无权限/非系统应用 | System API | reject 201/202 | AC-1.6 |
| 3 | 入参非法 | System API | reject 24010000 | AC-1.7 |

**abilityToolAccessCtrl.generateTaskIdentity**

| 属性 | 值 |
|------|-----|
| 函数签名 | `generateTaskIdentity(taskIdentityInfo: TaskIdentityInfo): Promise<TaskIdentity>` |
| 返回值 | `Promise<TaskIdentity>` — {taskAttributes, identity, challenge} |
| 开放级别 | System |
| 错误码 | 201 / 202 / 24010000 / 24010001 / 24010002 |
| Depends On（外部模块 API） | 高危路径：`useriam_framework` `UserAccessCtrlClient::VerifyAuthToken`（经 CXX 桥） |
| 关联 AC | AC-1.2, AC-1.3, AC-1.4, AC-1.5, AC-1.6, AC-1.7, AC-2.1, AC-2.2 |

**参数约束**

同上表（taskIdentityInfo 约束一致）。

**行为场景**

| # | 触发条件 | 可观察表面 | 预期行为 | 关联 AC |
|---|----------|--------------|----------|---------|
| 1 | 不携 authToken（低危） | System API | resolve TaskIdentity（identity/challenge 为 base64 值） | AC-1.2 |
| 2 | 携 authToken 且 IAM 校验通过（高危） | System API | resolve TaskIdentity（taskAttributes 含 authToken+atl） | AC-1.2 |
| 3 | 携 authToken 但 IAM challenge 不匹配 | System API | reject 24010002（MISMATCH） | AC-1.3 |
| 4 | 高危未先 challenge / 已超时 | System API | reject 24010002（NOT_FOUND） | AC-1.4 |
| 5 | 入参非法 | System API | reject 24010000 | AC-1.7 |

**abilityToolAccessCtrl.revokeTaskIdentity**

| 属性 | 值 |
|------|-----|
| 函数签名 | `revokeTaskIdentity(taskId: string[]): Promise<void>` |
| 返回值 | `Promise<void>` |
| 开放级别 | System |
| 错误码 | 201 / 202 / 24010000 / 24010001 / 24010002 |
| Depends On（外部模块 API） | 无 |
| 关联 AC | AC-1.5, AC-4.4 |

**参数约束**

| 参数 | 类型 | 必填 | 默认值 | 约束条件 |
|------|------|------|--------|---------|
| taskId | string[] | 是 | — | 1~100 个；每个非空、无路径穿越字符 |

**行为场景**

| # | 触发条件 | 可观察表面 | 预期行为 | 关联 AC |
|---|----------|--------------|----------|---------|
| 1 | 合法列表 | System API | resolve；撤销对应记录 | AC-1.5 |
| 2 | 不存在的 taskId | System API | resolve（幂等） | AC-4.4 |
| 3 | 入参非法 | System API | reject 24010000 | AC-1.7 |

**RetrieveAuthTokenByTaskId（C++ InnerAPI）**

| 属性 | 值 |
|------|-----|
| 函数签名 | `RetrieveAuthTokenByTaskId(callerToken, taskId)`（C++，具体签名见本仓 `saf_agent_fence.h`） |
| 返回值 | authToken 字节 |
| 开放级别 | InnerAPI |
| 错误码 | 201 / 24010000 / 24010001 / 24010002 |
| Depends On（外部模块 API） | 无（服务侧本地校验；调用方权限复用既有 PERMISSION 体系） |
| 关联 AC | AC-4.1, AC-4.2, AC-4.3 |

**行为场景**

| # | 触发条件 | 可观察表面 | 预期行为 | 关联 AC |
|---|----------|--------------|----------|---------|
| 1 | 系统 SA + 凭证有效 + HMAC 通过 | InnerAPI | 返回 authToken 字节 | AC-4.1 |
| 2 | 凭证过期/篡改/撤销 | InnerAPI | 映射 24010002（EXPIRED / MISMATCH / NOT_FOUND） | AC-4.3 |
| 3 | 非系统 SA / 缺权限 | InnerAPI | 201 | AC-4.2 |

## API 规格定义

> 条件触发（`API/SDK` = 「是」）。本表以权威声明文件 `D:\@ohos.abilityToolAccessCtrl.d.ts` 为签名/注释事实来源，补充要素由开发者确认。

### 公共规格属性

| 属性 | 值 |
|------|-----|
| API 类型 | ArkTS（d.ts 声明，3 个 System API）+ C++（InnerAPI 1 个，不对外声明） |
| 编程语言 | ArkTS |
| @syscap | `SystemCapability.Security.Asset` |
| @since | 26.1.0（声明文件既有值；跨平台/版本口径以版本规划确认为准） |
| 权限要求 | `ohos.permission.QUERY_TOOL_PERMISSIONS`（3 个 System API）；`RetrieveAuthTokenByTaskId` 另需 `INTERACT_ACROSS_LOCAL_ACCOUNTS`（跨用户交互权限） |
| 跨平台支持 | 否 |
| 元服务支持 | 否 |
| 卡片支持 | 否 |
| FA/Stage 模型 | `@FaAndStageModel` |

### API 详细规格

#### API: `generateTaskChallenge(taskIdentityInfo: TaskIdentityInfo): Promise<Uint8Array>`

**规格表**

| 属性 | 值 |
|------|-----|
| 函数签名 | `generateTaskChallenge(taskIdentityInfo: TaskIdentityInfo): Promise<Uint8Array>` |
| 入参 | `taskIdentityInfo: TaskIdentityInfo` — 任务身份信息（见接口规格参数约束） |
| 返回值 | `Promise<Uint8Array>` — 32 字节随机挑战值 |
| 错误码 | 201 / 202 / 24010000 / 24010001 / 24010002 |
| 开放级别 | System（@systemapi） |
| Depends On（外部模块 API） | 无（本地 CSPRNG 生成；不依赖 IAM） |
| 与公共属性的差异 | 同公共规格属性 |

**API 描述（接口定义三要素 + 接口使用五要素）**

*接口定义三要素*

| 要素 | 内容 |
|------|------|
| 含义/功能（What） | 基于任务身份信息生成任务认证挑战值（供后续携带身份认证凭据使用） |
| 使用场景（Why） | 系统应用执行需要机主认证的高危任务前，先生成 challenge，再由用户完成机主认证后携带 authToken 调用 `generateTaskIdentity` |
| 使用后效果（Effect） | 返回 32 字节挑战值，并建立待认证任务上下文 |

*接口使用五要素*

| 要素 | 内容 |
|------|------|
| 相似接口差异 | 与 `requestToolPermissions` 的 challenge 生成机制同源（双 challenge HMAC 模式），本接口面向任务级身份而非工具权限 |
| 缺省配置 | N/A（无默认值；challenge 10 分钟 boot-time 有效） |
| 规格限制 | 仅系统应用；权限 `QUERY_TOOL_PERMISSIONS`；入参 taskId 非空、executeType 合法 |
| 生效机制 | 同步生成后 Promise 返回；同一 (caller,taskId) 重复调用覆盖旧 challenge |
| 注意事项 | 高危路径必须在 challenge 有效期内完成 `generateTaskIdentity`；返回的 challenge 应与 IAM 认证返回的 challenge 保持一致用于比对 |

#### API: `generateTaskIdentity(taskIdentityInfo: TaskIdentityInfo): Promise<TaskIdentity>`

**规格表**

| 属性 | 值 |
|------|-----|
| 函数签名 | `generateTaskIdentity(taskIdentityInfo: TaskIdentityInfo): Promise<TaskIdentity>` |
| 入参 | `taskIdentityInfo: TaskIdentityInfo` — 任务身份信息 |
| 返回值 | `Promise<TaskIdentity>` — `{taskAttributes, identity, challenge}` |
| 错误码 | 201 / 202 / 24010000 / 24010001 / 24010002 |
| 开放级别 | System（@systemapi） |
| Depends On（外部模块 API） | 高危路径依赖 `useriam_framework` `UserAccessCtrlClient::VerifyAuthToken`（经 CXX 桥）；低危路径无依赖 |
| 与公共属性的差异 | 同公共规格属性 |

**API 描述（接口定义三要素 + 接口使用五要素）**

*接口定义三要素*

| 要素 | 内容 |
|------|------|
| 含义/功能（What） | 为代理命令（Agent 长时/定时任务）生成基于任务的身份认证身份 |
| 使用场景（Why） | 系统应用任务经办方为用户本人授权的 Agent 任务生成任务级身份凭证，支持跨会话/锁屏执行 |
| 使用后效果（Effect） | 返回包含 taskAttributes、identity（HMAC base64）、challenge 的任务身份凭证，任务可凭此凭证跨会话校验 |

*接口使用五要素*

| 要素 | 内容 |
|------|------|
| 相似接口差异 | 与 `requestToolPermissions`/`requestToolTicket` 的 ticket 生成不同，本接口生成任务级身份凭证（绑定任务 ID 与过期时间） |
| 缺省配置 | 不携带 `authToken` 时走低危路径（仅生成任务身份，不绑定机主认证结果） |
| 规格限制 | 仅系统应用；权限 `QUERY_TOOL_PERMISSIONS`；`authToken ≤4096` 字节；`allowableDuration` 有界 |
| 生效机制 | 同步生成后 Promise 返回；`taskExpireTime` 决定凭证有效期（-1 仅 DELAYED 永不过期） |
| 注意事项 | 高危路径需先 `generateTaskChallenge` 并在有效期内完成；凭证可撤销（`revokeTaskIdentity`） |

#### API: `revokeTaskIdentity(taskId: string[]): Promise<void>`

**规格表**

| 属性 | 值 |
|------|-----|
| 函数签名 | `revokeTaskIdentity(taskId: string[]): Promise<void>` |
| 入参 | `taskId: string[]` — 待撤销任务 ID 列表（1~100 个） |
| 返回值 | `Promise<void>` |
| 错误码 | 201 / 202 / 24010000 / 24010001 / 24010002 |
| 开放级别 | System（@systemapi） |
| Depends On（外部模块 API） | 无（本地删除记录） |
| 与公共属性的差异 | 同公共规格属性 |

**API 描述（接口定义三要素 + 接口使用五要素）**

*接口定义三要素*

| 要素 | 内容 |
|------|------|
| 含义/功能（What） | 批量撤销代理命令的任务认证标识，删除本地缓存 |
| 使用场景（Why） | 任务完成/用户收回授权/任务失效时撤销任务身份凭证 |
| 使用后效果（Effect） | 对应任务凭证失效，后续校验返回非成功结果（对外 24010002） |

*接口使用五要素*

| 要素 | 内容 |
|------|------|
| 相似接口差异 | 与授权类接口正交；同线程任务可并行撤销 |
| 缺省配置 | N/A |
| 规格限制 | 1~100 个 taskId；每个非空、无路径穿越字符；仅系统应用 |
| 生效机制 | 同步撤销后 Promise resolve；幂等（不存在的 taskId 不报错） |
| 注意事项 | 撤销后再生成相同 taskId 可正常使用 |

**设备行为差异**

*支持设备表*

| 设备类型 | 起始版本（@since） | 备注 |
|----------|-------------------|------|
| 手机 | 26.1.0 | |
| 平板 | 26.1.0 | |
| 折叠屏 | 26.1.0 | |
| 其他设备（桌面/穿戴/电视/车机等） | 26.1.0 | 系统侧安全能力，无设备品类差异 |

*差异明细表*

| 设备类型 | 差异行为 | 差异原因 |
|----------|----------|----------|
| 所有支持设备 | 与手机一致（proposal 1+8 设备差异：共用同一实现基线） | 系统侧安全能力，不依赖设备品类 |

## 兼容性声明

- **已有 API 行为变更:** 否。既有 `requestToolPermissions` / `grantToolPermissionsByUser` / `requestToolTicket` / 远程授权 API 的对外行为与错误码语义保持不变；`24010006` 为既有错误码复用。
- **配置文件格式变更:** 否。
- **数据存储格式变更:** 否（任务身份沿用既有 `taskAttributes` 磁盘结构，见 proposal 不涉及项确认）。
- **最低支持版本:** OpenHarmony 7.1（API 26.1.0）。
- **API 版本号策略:** 新增 API 标注 `@since 26.1.0`（已确认：OpenHarmony 7.1 的 SDK 配套版本即 26.1.0）。

## 架构约束

> 架构规则适用性及设计方案见 design.md。

| 关键约束 | 约束说明 | 影响 AC |
|----------|----------|---------|
| 仅系统应用可调用 | 3 个 System API 均要求 `@systemapi` + 系统应用 + 权限 | AC-1.6, AC-4.2 |
| 锁屏豁免必须以任务 ATL 锁屏策略为准 | 锁屏态放行条件 = 任务凭证 `allowScreenLockExecution=true` + ATL>0 + 该 ATL 有锁屏策略 + 请求权限不在拒绝列表；不得以 `RetrieveAuthTokenByTaskId` 单一接口作为唯一校验 | AC-2.1, AC-2.2, AC-2.3, AC-4.1 |
| 对外行为与错误码语义保持与基线 ticket 链路一致 | 任务身份校验（新增能力）的对外错误码语义（InvalidArgument/NotFound/Expired/Mismatch → 对外映射）与基线 ticket 链路风格一致，不改变既有 `requestToolPermissions` 行为（未携带 taskIdentity 时） | AC-2.4, AC-3.1, AC-3.2, AC-3.4 |
| 锁屏拦截统一使用 24010006 | 锁屏执行/发授权未达标统一返回 24010006（`OPERATION_FAILED_UNDER_SCREEN_LOCK`） | AC-2.2, AC-2.3, AC-3.3 |

## 非功能性需求

> 依据 proposal「不涉及项确认」与「资源开销审视」填写适用项。

| 类型 | 指标/阈值 | 验证方式 | 证据 |
|------|-----------|----------|------|
| 性能 | 凭证校验进入锁屏执行冷路径；**锁屏拉起校验时延（含任务凭证校验 + 锁屏豁免判定）相对解锁态基线的最大劣化 ≤150ms**（含插件策略查询）；单次锁屏拉起校验时延 P95 ≤250ms | benchmark（锁屏态 vs 解锁态对照采样 ≥100 次，见验证映射 VM-16 性能对照实验） | `evidence/resource/`（实验后落） |
| 存储(ROM) | 镜像增量 ≤100KB（约 2000 行 Rust 代码折算） | ABU/冒烟 | `evidence/resource/`（待验证） |
| 存储(磁盘) | 单 agent 任务信息磁盘缓存 ≤10MB | 磁盘占用检查 | `evidence/resource/`（待验证） |
| 内存 | 凭证存储沿用既有 LRU 32 上限（~24KB），无新增可持续内存占用 | hidumper/代码核对 | design.md 资源设计 |
| 安全 | 机主认证 ATL、凭证防篡改（HMAC）、锁屏执行管控 | 安全 review + 矩阵用例 | threat-model.md（如触发） |
| 可测试性 | 每个 AC 有测试入口 + Red 条件（见验证映射 16 项 VM，含 VM-16 性能对照实验） | 单测/集成/XTS | `evidence/reviews/` |
| 定界定位 | 锁屏拦截、凭证无效等失败可通过错误码（24010006 / 24010002 / 201）快速定界 | hilog/错误码 | spec 错误码事实 |

## 多设备适配声明

| 设备类型 | 行为差异 | 规格/约束 | 验证方式 | 证据 |
|----------|----------|-----------|----------|------|
| 手机 | 无差异 | 共用基线 | 端到端 | `evidence/reviews/` |
| 平板 | 无差异 | 共用基线 | 端到端 | `evidence/reviews/` |
| 折叠屏 | 无差异 | 共用基线 | 端到端 | `evidence/reviews/` |

## 全局特性影响

| 特性 | 适用？ | 结论 | 关联场景 |
|------|--------|------|----------|
| 无障碍 | 否 | 纯系统侧安全能力，无 UI 交互面 | — |
| 大字体 | 否 | 同上 | — |
| 深色模式 | 否 | 同上 | — |
| 多窗口/分屏 | 否 | 安全系统服务，不涉及 | — |
| 多用户 | 否 | proposal 非目标：不做跨用户凭证迁移/协同；凭证按 os_account_id 隔离 | US-1 |
| 版本升级 | 是 | 新增 @since 26.1.0 System API；既有 API 行为不变 | 兼容性声明 |
| 生态兼容 | 是 | 仅系统应用/系统 SA 可使用，不对三方开发者开放 | AC-1.6, AC-4.2 |

## 审批确认记录（Spec 评审结论）

> 以下确认项已完成 Owner 确认（Lin Shen，2026-10-24），其中涉及需求基线的结论由 proposal 侧同步对齐。

| # | 确认项 | 结论 | 状态 |
|---|--------|------|------|
| C-1 | `@since` / 目标发行版本 | **26.1.0 与版本规划一致**：OpenHarmony 7.1 对应的 SDK 配套版本即 26.1.0 | ✅ 已确认 |
| C-2 | `interface_sdk-js` 声明文件落地点 | **声明已合入** `interface_sdk-js`（MR 36745），签名与本 spec 权威声明 `D:\@ohos.abilityToolAccessCtrl.d.ts` 一致 | ✅ 已确认 |
| C-3 | `allowableDuration` 边界口径 | **保持 60**：声明文件侧口径 `(0,60]` 即最终约束，实现侧按此对齐（见 R-6） | ✅ 已确认 |
| C-4 | SIG 协同 / SELinux | **SELinux 配置非本项目目标**：`secure_access_fence.te` 配置已在 `selinux_adapter` 仓完成，本仓不涉及该配置交付 | ✅ 已确认 |
| C-5 | 锁屏豁免 ATL 扩展与拒绝列表 | **本规格即当前版本最终规格**：ATL3(30000)/ATL4(40000) 豁免、拒绝列表为空 → 放行的行为不再扩展；无补充项 | ✅ 已确认（无遗留） |
| C-6 | 闭源工具扫描整改（6 项） | **已纳入**：①性能指标量化——锁屏拉起校验时延相对解锁态基线最大劣化 ≤150ms、单次 P95 ≤250ms（NFR + VM-16）；②ATL 分级定义（AuthTrustLevel 枚举 4 级，见「核心术语定义」）；③每个 API 补充 Depends On（外部模块 API）；④锁屏拉起校验基准实验增加具体数值；⑤补充测试数据样例（测试数据样例表）；⑥用户故事补充正常流程步骤（供测试脚本编排）。均为规格表述层面的补齐，未改变对外行为与错误码语义 | ✅ 已纳入（Owner Lin Shen 2026-10-24） |

> 原「待确认项」清单已清零。上述结论中 C-2/C-4/C-5 与 proposal 侧概念一致（proposal 附录 Q-2/Q-3 及影响范围表述以此处为准）。