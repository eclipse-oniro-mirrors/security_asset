# 执行计划

> 将 Approved Spec 拆成可独立执行、可验证、可审查的 Task。每个 Task 用稳定 ID 和章节锚点引用上游事实，不复制 Spec/Design 正文。

## Plan 元数据

| 字段 | 内容 |
|------|------|
| Plan ID | PLAN-20260905541319 |
| 关联 Feature/Bug | FEAT-20260905541319（任务级身份凭证支撑 Agent 长时/定时任务在锁屏下执行） |
| 关联文档 | proposal.md / design.md / spec.md / threat-model.md |
| 复杂度 | 复杂 |
| 状态 | Approved |
| Owner | Lin Shen |

## 输入状态

| 输入 | 路径 | 要求状态 |
|------|------|----------|
| Requirement | `.codespec/changes/20260905541319/proposal.md` | Approved |
| Design | `.codespec/changes/20260905541319/design.md` | Approved |
| Spec | `.codespec/changes/20260905541319/spec.md` | Approved |
| Threat Model | `.codespec/changes/20260905541319/threat-model.md` | Draft（P0/P1 缓解已映射至 Task） |

## 执行原则

- **Spec 权威：** 实现与 AC、错误码、兼容性声明冲突时，停止并修订 spec/design/plan，不能在代码里自行改语义。
- **测试/证据先行：** 每个 Task 先确认失败测试或可复现证据缺口，再做最小实现并取得 fresh PASS 证据。
- **任务小型化：** 一个 Task 只覆盖一个独立闭环。
- **文件边界：** 只能修改 Task 的 Files/允许修改范围；需要额外文件时先修订 Plan。
- **状态所有权唯一：** 新增状态明确 owner、key/index、创建、读取、更新、清理触发和不变量（见各 Task 状态生命周期）。
- **证据回填：** Task 完成当次会话回填追溯状态、实际文件、Commit、Review Evidence 和 Actual Result，不跨会话补证。
- **反伪完成：** 只补声明、只写存储、只覆盖 happy path、只跑无关测试或只填写 PASS 文本，都不能替代 AC 闭环。
- **生成文件：** IDL 接口修改走声明源（`.idl`）修改；Rust `cxx_bridge` 声明与 C++ 桥由 `build.rs`/CXX 工具链生成；手工改生成物不能作为最终交付。

## 受影响文件全量清单

> 对照 design.md 调用链层级分析，逐层列出具体文件。

| 仓 | 层（来自 design.md） | 文件路径 | 修改类型 | 说明 |
|----|---------------------|----------|----------|------|
| security_asset | NAPI（JS 入口） | `frameworks/secure_access_fence/js/napi/src/agent_fence_napi.cpp` | 修改 | generateTaskChallenge/Identity/Revoke 的 ArkTS 入口 + typedarray 校验 |
| security_asset | NAPI | `frameworks/secure_access_fence/js/napi/inc/agent_fence_napi_context.h` | 修改 | NAPI 上下文新增任务身份字段 |
| security_asset | Inner API（C++ 入口） | `interfaces/inner_kits/c/secure_access_fence/agent_fence/inc/saf_agent_fence.h` | 修改 | 4 个任务身份静态方法声明 |
| security_asset | Inner API | `frameworks/secure_access_fence/inner_api/agent_fence/src/saf_agent_fence.cpp` | 修改 | proxy 调用 + 参数检查接入 |
| security_asset | Inner API | `frameworks/secure_access_fence/inner_api/agent_fence/libsaf_agent_fence.map` | 修改 | 符号导出 |
| security_asset | IPC | `frameworks/secure_access_fence/inner_api/ipc/ISecureAccessFence.idl` | 修改 | 新增 ipccode 12-15（生成源：`.idl`→Rust/C++ 声明） |
| security_asset | IPC | `frameworks/secure_access_fence/inner_api/ipc/SecureAccessFenceType.idl` | 修改 | 新增 `TaskIdentityInfo`/`TaskIdentity`/`ExecuteType` 类型 |
| security_asset | IPC | `frameworks/secure_access_fence/inner_api/ipc/src/sub_modules/task_identity.rs` | 新增 | 任务身份 IPC 序列化 + `deserialize_i32_u8_list` 范围校验 |
| security_asset | IPC | `frameworks/secure_access_fence/inner_api/ipc/src/sub_modules/mod.rs` | 修改 | 挂载 task_identity 子模块 |
| security_asset | IPC | `frameworks/secure_access_fence/inner_api/ipc/src/lib.rs` | 修改 | CMD 常量 12-15 + 序列化导出 |
| security_asset | 任务身份模块 | `services/secure_access_fence/core_service/src/task_identity/mod.rs` | 新增 | 挑战/签发（双 challenge + IAM）/撤销/校验脊柱/消费 |
| security_asset | 任务身份模块 | `services/secure_access_fence/core_service/src/task_identity/store.rs` | 新增 | LRU 内存缓存 + StorageBackend 委托 |
| security_asset | 任务身份模块 | `services/secure_access_fence/core_service/src/task_identity/storage_backend.rs` | 新增 | StorageBackend trait + DiskStorage + MockStorage |
| security_asset | 任务身份模块 | `services/secure_access_fence/core_service/src/task_identity/validator.rs` | 新增 | 参数/路径/边界校验（authToken≤4096、revoke 1~100、路径穿越） |
| security_asset | CXX 桥 | `services/secure_access_fence/core_service/src/wrapper.rs` | 修改 | cxx_verify_task_identity、get_atl_permission_policy、get_os_account_id（经 build.rs 生成绑定） |
| security_asset | CXX 桥（源） | `services/secure_access_fence/core_service/src/cxx/inc/iam_verify_wrapper.h` | 新增 | IAM 桥前向声明（生成源：CXX bridge fn 声明） |
| security_asset | CXX 桥（源） | `services/secure_access_fence/core_service/src/cxx/src/iam_verify_wrapper.cpp` | 新增 | `UserAccessCtrlClient::VerifyAuthToken` 同步封装（10s 超时） |
| security_asset | CXX 桥 | `services/secure_access_fence/core_service/src/cxx/src/secure_access_fence_service.cpp` | 修改 | GetCallingTokenId/GetOsAccountIdFromTokenId 接入 |
| security_asset | CXX 桥 | `services/secure_access_fence/core_service/src/cxx/inc/cxx_bridge_functions.h` | 修改 | bridge fn 声明扩展 |
| security_asset | SAF 服务（Rust） | `services/secure_access_fence/core_service/src/stub.rs` | 修改 | ipccode 12-15 分发 + 服务侧鉴权 |
| security_asset | SAF 服务（Rust） | `services/secure_access_fence/core_service/src/lib.rs` | 修改 | SAFService 方法 + verify_ticket_impl 插入任务校验消费 |
| security_asset | SAF 服务（Rust） | `services/secure_access_fence/core_service/src/metrics_macro.rs` | 修改 | 指标宏（execute_with_metrics_single） |
| security_asset | 锁屏豁免 | `services/secure_access_fence/core_service/src/cxx/inc/permission_manager.h` | 修改 | TaskAtlAttributes/VerifyAndParseTaskIdentity/GetAtlPermissionPolicy 声明 |
| security_asset | 锁屏豁免 | `services/secure_access_fence/core_service/src/cxx/src/permission_manager.cpp` | 修改 | RequestToolPermissions 锁屏豁免流程 + IsAtlLockScreenExecutionAllowed |
| security_asset | 插件接口 | `interfaces/inner_kits/plugin_interface/secure_access_fence/src/plugin_interface.rs` | 修改 | EventType::GetAtlPermissionPolicy + AtlPermissionPolicyKeys |
| security_asset | 错误码 | `interfaces/inner_kits/c/secure_access_fence/common/inc/saf_result_defs.h` | 修改 | 内部码 0x71005-0x71008 |
| security_asset | 构建 | `services/secure_access_fence/core_service/BUILD.gn` | 修改 | 新增源 + `user_auth_framework:userauth_client` 依赖 |
| security_asset | 构建 | `bundle.json` | 修改 | `user_auth_framework` 加入 deps.components |
| security_asset | C++ 测试 | `test/secure_access_fence/agent_fence/src/saf_agent_fence_retrieve_auth_token_test.cpp` | 新增 | RetrieveAuthTokenByTaskId 15 个 HWTEST_F |
| security_asset | Rust 单测 | `test/secure_access_fence/unittest/ut_test/services/core_service/src/` | 新增 | task_identity 模块单测 |

**检查项：**
- [x] design.md 调用链每一层（NAPI→Inner→IPC→SA→task_identity→CXX→IAM/插件）都有对应文件列出
- [x] 每个文件修改类型和职责说明明确
- [x] 无遗漏层（存储、错误码、构建、插件接口已覆盖）

## AC 到 Task 追溯

> AC = Acceptance Criteria。spec 每个 AC 映射到至少一个 Task（单一事实源）。

| AC | 来源 | Task | 计划代码范围 | 实际 Code Ref | Commit | Review Evidence | 验证状态 |
|----|------|------|--------------|-----------------|--------|-----------------|----------|
| AC-1.1 | spec.md | TASK-1, TASK-2 | `NAPI/IPC/IDL + task_identity/mod.rs` | `agent_fence_napi.cpp`、`task_identity/mod.rs:101` | `cd731e5b` | `verification.md#execution-records` | Pass |
| AC-1.2 | spec.md | TASK-1, TASK-2 | `同上` | `task_identity/mod.rs:175` | `cd731e5b` | `verification.md#execution-records` | Pass |
| AC-1.3 | spec.md | TASK-2 | `task_identity/mod.rs`（IAM 比对） | `task_identity/mod.rs:119` | `cd731e5b` | `verification.md#execution-records` | Pass |
| AC-1.4 | spec.md | TASK-2 | `task_identity/mod.rs`（pending 前置） | `task_identity/mod.rs:106-114,127` | `cd731e5b` | `verification.md#execution-records` | Pass |
| AC-1.5 | spec.md | TASK-1, TASK-2 | `permission_manager.cpp / task_identity/mod.rs` | `task_identity/mod.rs:240` | `cd731e5b` | `verification.md#execution-records` | Pass |
| AC-1.6 | spec.md | TASK-1 | `stub.rs` 服务侧鉴权 | `stub.rs`（ipccode 12-15 + check_query_tool_permission） | `cd731e5b` | `verification.md#execution-records` | Pass |
| AC-1.7 | spec.md | TASK-2 | `task_identity/validator.rs` + NAPI | `task_identity/validator.rs:75` + `napi_common.cpp` | `cd731e5b` | `verification.md#execution-records` | Pass |
| AC-2.1 | spec.md | TASK-4 | `permission_manager.cpp` 锁屏豁免 | `permission_manager.cpp:948-985` | `fc582502` | `verification.md#execution-records` | Pass |
| AC-2.2 | spec.md | TASK-4 | `permission_manager.cpp` + plugin_interface | `permission_manager.cpp:972-985`、`wrapper.rs:243` | `fc582502` | `verification.md#execution-records` | Pass |
| AC-2.3 | spec.md | TASK-4 | `permission_manager.cpp` + wrapper 策略 | `permission_manager.cpp:147-154`（IsAtlLockScreenExecutionAllowed） | `fc582502` | `verification.md#execution-records` | Pass |
| AC-2.4 | spec.md | TASK-4 | `permission_manager.cpp`（兼容分支） | `permission_manager.cpp:963`（needUnlock 基线） | `fc582502` | `verification.md#execution-records` | Pass |
| AC-3.1 | spec.md | TASK-5 | `lib.rs` verify_ticket_impl + task_identity | `lib.rs:660`（verify_task_identity_and_consume）+ `task_identity/mod.rs:391` | `fc582502` | `verification.md#execution-records` | Pass |
| AC-3.2 | spec.md | TASK-5 | `task_identity/mod.rs`（消费校验） | `task_identity/mod.rs:391-414` | `fc582502` | `verification.md#execution-records` | Pass |
| AC-3.3 | spec.md | TASK-4, TASK-5 | `permission_manager.cpp` + `lib.rs` | `permission_manager.cpp:984`、`lib.rs:642-650` | `fc582502` | `verification.md#execution-records` | Pass |
| AC-3.4 | spec.md | TASK-5 | `task_identity/mod.rs`（callerToken 校验） | `task_identity/mod.rs:311-313` | `cd731e5b` | `verification.md#execution-records` | Pass |
| AC-3.5 | spec.md | TASK-4, TASK-5 | `task_identity/mod.rs` + lib.rs | `task_identity/mod.rs:327`（-1 永不过期）+ `lib.rs:660` | `fc582502` | `verification.md#execution-records` | Pass |
| AC-4.1 | spec.md | TASK-1, TASK-3 | `saf_agent_fence.*` + `task_identity/mod.rs+store` | `saf_agent_fence.h:236`、`task_identity/mod.rs:380` | `cd731e5b` | `verification.md#execution-records` | Pass |
| AC-4.2 | spec.md | TASK-1 | `stub.rs` Retrieve 双重鉴权 | `stub.rs`（check_retrieve_auth_token_permission） | `cd731e5b` | `verification.md#execution-records` | Pass |
| AC-4.3 | spec.md | TASK-2, TASK-3 | `task_identity/` 校验脊柱 + 存储 | `task_identity/mod.rs:306`（fetch_record_and_verify） | `fc582502` | `verification.md#execution-records` | Pass |
| AC-4.4 | spec.md | TASK-3 | `task_identity/mod.rs + store`（幂等/重生成） | `task_identity/mod.rs:240-249`、`store.rs`（delete/insert） | `cd731e5b` | `verification.md#execution-records` | Pass |

## 首批实现边界

**首批必须实现：** 4 个任务身份 API 全链路（挑战/签发/撤销/检索）、锁屏豁免判定、verifyTicket 任务校验消费、磁盘+LRU 存储、服务侧鉴权。
**可后置：** 审计打点（TH-009 建议项）、`allowableDuration` 内部常量收敛（D-2）、threat-model 中 P2 项。
**不建议延后：** 锁屏豁免判定（P0 TH-003）、凭证篡改防护（P0 TH-004）、服务侧鉴权（P0 TH-001）——延后会导致安全底线不闭合。

## Task 粒度原则

- 每个 Task 对应一个可独立验收的最小能力闭环。
- 简单变更：1-2 张 Task Card；复杂变更：按接线层/规则层/存储层/豁免层/消费层拆分。
- 单 Task 上下文（Spec+Design+代码+测试）超过 3000 行时沿 AC/文件边界拆分。

## 禁止项

- [x] 无占位词、「视需求而定」类含糊指令、隐式依赖、自行寻上下文、无验证 AC、跨 Task 引用
- [x] 无"根据需要实现""酌情处理"等模糊指令
- [x] 无跨 Task 隐式依赖（依赖在"前置依赖"列显式声明）
- [x] 无要求 Agent 自行寻找未列出的上下文文件
- [x] 无未映射到验证方式的 AC
- [x] 无"与 Task-N 类似""参考 Task-N 实现"等引用（每个 Task 自包含）

## Task 列表

| Task ID | 目标 | 文件范围 | AC 映射 | 前置依赖 | 完成判据 | 验证命令 | 状态 |
|---------|------|----------|---------|----------|----------|----------|------|
| TASK-1 | API/IDL 声明 + 服务侧鉴权接线 | `.idl`、`saf_agent_fence.*`、`NAPI`、`stub.rs`、`saf_result_defs.h`、BUILD/bundle | AC-1.1,1.2,1.5,1.6,4.1,4.2 | design+spec Approved | IDL/NAPI/InnerAPI 可编译链接；服务侧 201/202 校验通过 | `hb build secure_access_fence` + 单测 | Done |
| TASK-2 | 任务身份签发/校验核心（双 challenge HMAC + IAM） | `task_identity/mod.rs`、`validator.rs`、`iam_verify_wrapper.*` | AC-1.1~1.4,1.7,4.3 | TASK-1 | 签发/校验/过期/篡改/边界单测通过 | C++/Rust 单测 | Done |
| TASK-3 | 任务凭证存储（磁盘+LRU+撤销幂等） | `task_identity/store.rs`、`storage_backend.rs` | AC-4.1,4.3,4.4,3.2 | TASK-1 | 磁盘/LRU/撤销/重生成/过期清理单测通过 | Rust 单测 | Done |
| TASK-4 | 锁屏豁免判定 + 任务有效性检索 | `permission_manager.cpp`、`wrapper.rs`、`plugin_interface.rs`、`saf_agent_fence.*` | AC-2.1~2.4,3.3 | TASK-2, TASK-3 | 豁免矩阵遍历 + RetrieveAuthTokenByTaskId 15 用例通过 | HWTEST_F + 集成 | Done |
| TASK-5 | verifyTicket 链上任务校验消费 | `lib.rs`（verify_ticket_impl 插入） | AC-3.1~3.5 | TASK-2, TASK-3 | verifyTicket 携带 taskIdentity 校验/消费端到端通过 | 集成/XTS | Done |

## Task 详情

### TASK-1: API/IDL 声明 + 服务侧鉴权接线

| 字段 | 内容 |
|------|------|
| 任务目标 | 交付 4 个任务身份接口的完整声明链路（IDL ipccode 12-15 → NAPI → Inner API → IPC 序列化）与服务侧鉴权 |
| AC 映射 | AC-1.1, AC-1.2, AC-1.5, AC-1.6, AC-4.1, AC-4.2 |
| 前置依赖 | design + spec Approved；`interface_sdk-js` 声明已在 MR 36745 合入（C-2） |
| 非目标 | 业务签发/校验逻辑（归 TASK-2）；存储（归 TASK-3）；锁屏豁免（归 TASK-4） |
| 完成判据 | IPCDL/NAPI/Inner API/错误码可编译链接；服务侧 stub 对 4 命令独立鉴权（201/202）；`RetrieveAuthTokenByTaskId` 双重权限校验 |
| 停止条件 | `interface_sdk-js` 声明与本仓签名不一致时停止并修订 spec |
| Design 状态/不变量 | STATE-1/2 创建点（TASK-2）；INV-1, INV-3 依赖此接线 |

**状态所有权和生命周期**

> 本 Task 无新增状态（仅声明接线），状态生命周期由 TASK-2/TASK-3 定义。

| Design Ref | 状态 | Owner | Key / Index | 创建 | 读取/更新 | 清理触发 | 生命周期不变量 |
|------------|------|-------|-------------|------|-----------|----------|----------------|
| N/A | N/A | N/A | N/A | N/A | N/A | N/A | N/A（仅声明接线，无运行状态） |

**任务间接口（Produces / Consumes）**

| Direction | Contract | Provider / Consumer | Dependency | Compatibility Constraint |
|-----------|----------|---------------------|------------|--------------------------|
| Produces | `TaskIdentityInfo` / `TaskIdentity` / `ExecuteType`（IDL 类型） | TASK-2 | 供 `generate_task_identity` 消费 | 与 `.d.ts` 类型一致（taskId/taskExpireTime/executeType/allowScreenLockExecution/authToken/allowableDuration） |
| Produces | ipccode 12-15 序列化 + 错误码 0x71005-8 | TASK-2~TASK-5 | 跨 Task 复用 | 对外映射 24010000/24010002；内部码 TICKET_OPERATION 域 |
| Consumes | （无，TASK-1 为接线前置） | — | — | — |

**Read-only Context**

| 路径 | 读取目的 |
|------|----------|
| `D:\@ohos.abilityToolAccessCtrl.d.ts` | 权威签名（generateTaskChallenge/Identity/Revoke/TaskIdentityInfo 等） |
| `interfaces/inner_kits/c/secure_access_fence/agent_fence/inc/saf_agent_fence.h`（基线部分） | 既有 C++ 方法签名风格 |

**Files**

| 操作 | 文件 | 说明 |
|------|------|------|
| Modify | `frameworks/secure_access_fence/inner_api/ipc/ISecureAccessFence.idl` | 新增 ipccode 12-15（生成源） |
| Modify | `frameworks/secure_access_fence/inner_api/ipc/SecureAccessFenceType.idl` | 新增 TaskIdentityInfo/TaskIdentity/ExecuteType（生成源） |
| Modify | `frameworks/secure_access_fence/inner_api/ipc/SecureAccessFenceType.idl` 生成物（`ISecureAccessFence.h` 等） | 由 IDL 生成命令产出，不手工改 |
| Create | `frameworks/secure_access_fence/inner_api/ipc/src/sub_modules/task_identity.rs` | IPC 序列化 + u8 范围校验 |
| Modify | `frameworks/secure_access_fence/inner_api/ipc/src/sub_modules/mod.rs` | 挂载子模块 |
| Modify | `frameworks/secure_access_fence/inner_api/ipc/src/lib.rs` | CMD 常量 + 导出 |
| Modify | `interfaces/inner_kits/c/secure_access_fence/agent_fence/inc/saf_agent_fence.h` | 4 方法声明 |
| Modify | `frameworks/secure_access_fence/inner_api/agent_fence/src/saf_agent_fence.cpp` | proxy 调用 + 参数检查 |
| Modify | `frameworks/secure_access_fence/js/napi/src/agent_fence_napi.cpp` + `inc/agent_fence_napi_context.h` | ArkTS 入口 + typedarray 校验 |
| Modify | `services/secure_access_fence/core_service/src/stub.rs` | ipccode 12-15 分发 + 服务侧鉴权 |
| Modify | `interfaces/inner_kits/c/secure_access_fence/common/inc/saf_result_defs.h` | 内部码 0x71005-8 |
| Modify | `services/secure_access_fence/core_service/BUILD.gn` + `bundle.json` | 新增源 + user_auth_framework 依赖 |
| Test | `test/secure_access_fence/agent_fence/src/`（新增鉴权限用例） | 201/202 校验 |

**禁止修改文件**

| 文件/路径 | 原因 |
|-----------|------|
| `.d.ts`/`interface_sdk-js` | 已由 MR 36745 合入（C-2），本仓不重复改 |
| `task_identity/mod.rs` 等业务逻辑 | 归 TASK-2 |
| `selinux_adapter` 仓 | 策略在该仓（C-4） |

**Spec References**

- AC / Rules: `AC-1.1`, `AC-1.5`, `AC-1.6`, `AC-4.1`, `AC-4.2`, `R-4`, `R-5`
- Anchors: `spec.md#用户故事`, `spec.md#API-与错误码事实`, `spec.md#接口规格`

**Design References**

- IDs / Anchors: `ADR-1`, `ADR-6`, `design.md#代码事实基线`（IDL/错误码/权限），`design.md#调用链层级分析`（NAPI/Inner/IPC/SA 层）

**Required Rules**

| Rule ID | Must / Must Not |
|---------|-----------------|
| OH-ARCH-IPC-SAF | 服务侧 stub 独立鉴权为唯一防线；IPC 载荷校验 |
| OH-ARCH-API-LEVEL | 级别 System/InnerAPI；权限 `QUERY_TOOL_PERMISSIONS`；@since 26.1.0 |
| OH-ARCH-COMPONENT-BUILD | `user_auth_framework` 依赖声明正确 |

**Steps**

- [ ] 写失败测试：无权限调用返回 201、非系统应用返回 202（C++/XTS 用例）；IDL 接口存在性用例
- [ ] 运行测试，确认当前失败（接口不存在/鉴权缺失）
- [ ] 修改 IDL→生成绑定、NAPI、Inner API、stub 鉴权、错误码
- [ ] 运行测试，确认 GREEN
- [ ] 回填 AC 追溯、实际文件、Commit、Review Evidence、Actual Result 和计划偏差

**Reference Pattern**

| 参考实现 | 复用约定 | 偏差及理由 |
|----------|----------|------------|
| `stub.rs:97`（基线 CMD→handler 分发） | 扩展既有 match 分支 | 无 |
| `napi_common.cpp:40`（MapErrorCode） | 新增内部码纳入同一映射 | 无 |

**Anti-Fake Completion**

| Check | Required Evidence |
|-------|-------------------|
| AC closed | 4 接口可调用；鉴权正向/异常路径证据（201/202） |
| Scope respected | diff 仅含本 Task Files |
| State lifecycle complete | N/A（无状态） |
| Interface contract matched | IDL 类型与 `.d.ts` 一致 |
| Relevant verification | 鉴权限用例直接覆盖本 Task |

**Verification**

| 证据类型 | 命令/路径 | Expected Result | Actual Result |
|----------|-----------|-----------------|---------------|
| 编译 | `hb build secure_access_fence` | IDL/NAPI/Inner 编译链接通过 | PASS：构建通过（Owner 确认，2026-10-24） |
| 单测 | `HWTEST_F` 鉴权限用例 | 201/202 精确返回 | PASS：无权限/非系统应用用例通过（AC-1.6） |
| 静态检查 | `cargo clippy` / 代码评审 | 无新增 warning | PASS：clippy 无新增（构建机） |

**Handoff Summary**

| 项 | 内容 |
|----|------|
| 任务描述 | TASK-1：API/IDL 声明链路 + 服务侧鉴权接线 |
| 允许修改 | `.idl`、`saf_agent_fence.*`、`agent_fence_napi.*`、`napi_common.*`、`stub.rs`、`saf_result_defs.h`、`sub_modules/task_identity.rs`、BUILD/bundle |
| 允许新建 | `sub_modules/task_identity.rs`、测试文件 |
| 只读参考 | `.d.ts`、design.md 基线事实、spec API 章节 |
| Spec 引用 | `AC-1.1,1.5,1.6,4.1,4.2` / `spec.md#API-与错误码事实` |
| Design 引用 | `ADR-1,6` / `design.md#代码事实基线` |
| 执行步骤 | 本 Task Card 的 `Steps` |
| 验证命令 | `hb build secure_access_fence` / 期望: PASS |
| 完成规则 | 不得修改允许范围外文件；无 fresh verification evidence 不得声明完成 |

**Review Handoff**

| Reviewer | Input |
|----------|-------|
| Spec Compliance | AC→Task→Code→Commit、鉴权边界、是否额外行为 |
| Code Quality | 实际代码范围、风险点（IPC 序列化安全性）、Base/Head SHA |
| Verification | Expected/Actual、fresh evidence、鉴权回归 |

---

### TASK-2: 任务身份签发/校验核心（双 challenge HMAC + IAM）

| 字段 | 内容 |
|------|------|
| 任务目标 | 交付任务身份签发（低危/高危双路径）与统一校验脊柱（fetch_record_and_verify），含 IAM 桥 |
| AC 映射 | AC-1.1~AC-1.4, AC-1.7, AC-4.3 |
| 前置依赖 | TASK-1（IDL 类型/错误码接线） |
| 非目标 | 存储落盘（归 TASK-3）；锁屏豁免（归 TASK-4）；verifyTicket 消费（归 TASK-5） |
| 完成判据 | challenge 生成、签发双路径、pending 比对、HMAC 校验、过期、篡改、参数边界单测全部通过 |
| 停止条件 | IAM `VerifyAuthToken` 返回语义与设计假设冲突时停止，回 spec/design 澄清 |
| Design 状态/不变量 | STATE-1 创建（签发时）；STATE-2 创建（challenge）；INV-1, INV-3, INV-4 |

**状态所有权和生命周期**

| Design Ref | 状态 | Owner | Key / Index | 创建 | 读取/更新 | 清理触发 | 生命周期不变量 |
|------------|------|-------|-------------|------|-----------|----------|----------------|
| STATE-2 | 待认证挑战值（pending challenge） | `task_identity/store.rs` | `(os_account_id, caller, task_id)` | `generate_task_challenge` | `verify_iam_and_get_challenge`（比对） | 10 分钟超时 / 比对后 | INV-4 |
| STATE-1（签发侧） | 任务凭证记录写入 | `task_identity/store.rs` | `(os_account_id, caller, task_id)` | `generate_task_identity` | fetch_record_and_verify（校验） | 撤销/过期/一次性消费 | INV-1, INV-3 |

**任务间接口（Produces / Consumes）**

| Direction | Contract | Provider / Consumer | Dependency | Compatibility Constraint |
|-----------|----------|---------------------|------------|--------------------------|
| Produces | `generate_task_identity(...) -> Result<TaskIdentity>`；`generate_task_challenge(...) -> Result<Vec<u8>>`；`revoke_task_identities(...)` | TASK-4, TASK-5 | 供校验/豁免/消费 | 签名与 `.d.ts` 一致；错误码 NotFound/Expired/Mismatch→24010002 |
| Produces | IAM 桥 `VerifyAuthToken(authToken, allowableDuration) -> (challenge, atl)` | TASK-4 | 供锁屏豁免读取 atl | 10s 超时；真实 IAM 路径 |
| Consumes | `TaskIdentityInfo`/`TaskIdentity`（TASK-1） | TASK-1 | 输入类型 | 与 IDL/.d.ts 一致 |

**Read-only Context**

| 路径 | 读取目的 |
|------|----------|
| `services/secure_access_fence/core_service/src/ticket_operation/mod.rs:164,172` | 双 challenge HMAC 模式参考（create_ticket_key_manager / batch_generate_ticket） |
| `services/secure_access_fence/core_service/src/ticket_operation/ticket_key_manager.rs:20` | derive_ticket_session_key 契约 |

**Files**

| 操作 | 文件 | 说明 |
|------|------|------|
| Create | `services/secure_access_fence/core_service/src/task_identity/mod.rs` | 挑战/签发/撤销/校验脊柱/消费 |
| Create | `services/secure_access_fence/core_service/src/task_identity/validator.rs` | 参数/路径/边界 |
| Create | `services/secure_access_fence/core_service/src/cxx/inc/iam_verify_wrapper.h` + `src/iam_verify_wrapper.cpp` | IAM 桥 |
| Create | `services/secure_access_fence/core_service/src/cxx/inc/cxx_bridge_functions.h`（扩展） | bridge fn 声明 |
| Modify | `services/secure_access_fence/core_service/src/wrapper.rs` | cxx_verify_task_identity + IAM 桥绑定 |
| Test | `test/secure_access_fence/unittest/ut_test/services/core_service/src/task_identity_test.rs` | 单测 |

**禁止修改文件**

| 文件/路径 | 原因 |
|-----------|------|
| `store.rs`/`storage_backend.rs` | 归 TASK-3 |
| `permission_manager.cpp` | 归 TASK-4 |

**Spec References**

- AC / Rules: `AC-1.1`, `AC-1.2`, `AC-1.3`, `AC-1.4`, `AC-1.7`, `AC-4.3`, `R-1`, `R-2`, `R-3`, `R-6`
- Anchors: `spec.md#规则定义`, `spec.md#接口规格`

**Design References**

- IDs / Anchors: `ADR-1`（双 challenge HMAC）、`ADR-5`（IAM 桥）、`design.md#状态归属与不变量`（STATE-1/2, INV-1/3/4）

**Required Rules**

| Rule ID | Must / Must Not |
|---------|-----------------|
| OH-ARCH-API-LEVEL | 凭证签发绑定 atl（高危）；HMAC 全量覆盖 taskAttributes |
| OH-ARCH-ERROR-LOG | 失败日志不外泄 authToken/identity |
| OH-ARCH-IPC-SAF | IAM 经包装桥，阻塞同步 ≤10s |

**Steps**

- [ ] 写失败测试：challenge 生成、pending 比对（Mismatch）、篡改（HMAC fail）、过期（Expired）、参数边界（24010000）
- [ ] 运行测试，确认 Red
- [ ] 实现 mod.rs（双 challenge + IAM + 校验脊柱）+ validator + IAM 桥
- [ ] 运行测试，确认 GREEN
- [ ] 回填追溯

**Reference Pattern**

| 参考实现 | 复用约定 | 偏差及理由 |
|----------|----------|------------|
| `ticket_operation/mod.rs:172`（batch_generate_ticket） | 双 challenge HMAC 同构 | 任务属性含 atl/allowScreenLockExecution，其余同构 |
| `ticket_operation/ticket_key_manager.rs:20` | derive_ticket_session_key | 无 |

**Anti-Fake Completion**

| Check | Required Evidence |
|-------|-------------------|
| AC closed | 签发/校验正向/异常/边界证据 |
| Scope respected | diff 仅 task_identity 核心 |
| State lifecycle complete | STATE-2 创建→比对→清理；STATE-1 创建→校验证据 |
| Interface contract matched | TaskIdentity 结构与 `.d.ts` 一致 |
| Relevant verification | 单测直接覆盖签发/校验 |

**Verification**

| 证据类型 | 命令/路径 | Expected Result | Actual Result |
|----------|-----------|-----------------|---------------|
| 单测 | `cargo test task_identity` | 全通过 | PASS：签发/校验/过期/篡改/边界用例通过（Owner 确认） |
| 静态检查 | `cargo clippy` | 无新增 warning | PASS：clippy 无新增（构建机） |

**Handoff Summary**

| 项 | 内容 |
|----|------|
| 任务描述 | TASK-2：任务身份签发/校验核心 |
| 允许修改 | `task_identity/mod.rs`、`validator.rs`、`iam_verify_wrapper.*`、`wrapper.rs`、`cxx_bridge_functions.h` |
| 允许新建 | 上述文件 + `task_identity_test.rs` |
| 只读参考 | `ticket_operation/*`、design ADR-1/5 |
| Spec 引用 | `AC-1.1~1.4,1.7,4.3` / `spec.md#规则定义` |
| Design 引用 | `ADR-1,5` / `STATE-1,2` / `INV-1,3,4` |
| 执行步骤 | 本 Task Card 的 `Steps` |
| 验证命令 | `cargo test task_identity` / 期望: PASS |
| 完成规则 | 不得修改允许范围外文件；无 fresh verification evidence 不得声明完成 |

**Review Handoff**

| Reviewer | Input |
|----------|-------|
| Spec Compliance | 签发/校验行为与 AC 一致、是否误用内部实现 |
| Code Quality | HMAC 实现、IAM 桥超时、是否泄露敏感 |
| Verification | Expected/Actual、篡改/过期注入证据 |

---

### TASK-3: 任务凭证存储（磁盘 + LRU + 撤销幂等）

| 字段 | 内容 |
|------|------|
| 任务目标 | 交付 StorageBackend（磁盘 serde+Mock）+ LruMap 内存缓存（cap=32）+ 撤销/重生成/过期清理 |
| AC 映射 | AC-4.1, AC-4.3, AC-4.4, AC-3.2 |
| 前置依赖 | TASK-1（类型/错误码） |
| 非目标 | 签发逻辑（归 TASK-2）；锁屏豁免（归 TASK-4） |
| 完成判据 | 磁盘读写、LRU 淘汰、撤销幂等、重生成、过期清理单测通过 |
| 停止条件 | 磁盘路径权限/SAF 沙箱约束与 `0o750/0o640` 冲突时停止并回 design |
| Design 状态/不变量 | STATE-1/2 存储载体；INV-2, INV-6 |

**状态所有权和生命周期**

| Design Ref | 状态 | Owner | Key / Index | 创建 | 读取/更新 | 清理触发 | 生命周期不变量 |
|------------|------|-------|-------------|------|-----------|----------|----------------|
| STATE-1 | 任务记录（磁盘+LRU） | `TaskIdentityStore` | `(os_id, caller, taskId)` | 签发写入 | fetch_record_and_verify（内存优先/磁盘兜底） | 撤销/一次性消费/on_idle 过期清理 | INV-2, INV-6 |
| STATE-2 | 待认证挑战值（磁盘+LRU） | `TaskIdentityStore` | `(os_id, caller, taskId)` | 挑战生成 | 比对后消费 | 10min 超时清理 | INV-4 |
| LRU 容量 | 有界缓存 | `store.rs` | — | — | entries/challenges 各 32 | — | INV-6 |

**任务间接口（Produces / Consumes）**

| Direction | Contract | Provider / Consumer | Dependency | Compatibility Constraint |
|-----------|----------|---------------------|------------|--------------------------|
| Produces | `insert_task_entry` / `get_task_record` / `delete_task_entry` / `insert_challenge` / `get_challenge` | TASK-2, TASK-4, TASK-5 | 凭证读写 | 内存优先/磁盘兜底语义 |
| Produces | `TaskIdentityRecord` 磁盘格式 | TASK-2, TASK-4 | 持久化 | JSON 字段含 taskAttributes/identity/challenge/expire/executeType |
| Consumes | `TaskIdentity`/错误码（TASK-1, TASK-2） | TASK-1, TASK-2 | 输入 | — |

**Read-only Context**

| 路径 | 读取目的 |
|------|----------|
| `services/secure_access_fence/core_service/src/task_identity/storage_backend.rs`（已存在基线实现） | trait/磁盘路径/权限（cd731e5b 已合入） |
| design.md「资源所有权矩阵」 | STATE-1/2 owner 与清理触发 |

**Files**

| 操作 | 文件 | 说明 |
|------|------|------|
| Create/Modify | `task_identity/store.rs` | LruMap + TaskIdentityStore |
| Create/Modify | `task_identity/storage_backend.rs` | StorageBackend + DiskStorage + MockStorage |
| Test | `test/secure_access_fence/unittest/ut_test/services/core_service/src/task_identity_store_test.rs` | 存储单测 |

**禁止修改文件**

| 文件/路径 | 原因 |
|-----------|------|
| `task_identity/mod.rs` 业务逻辑 | 归 TASK-2 |
| `permission_manager.cpp` | 归 TASK-4 |

**Spec References**

- AC / Rules: `AC-4.1`, `AC-4.3`, `AC-4.4`, `AC-3.2`, `R-4`, `R-6`
- Anchors: `spec.md#用户故事`, `spec.md#API-与错误码事实`

**Design References**

- IDs / Anchors: `ADR-4`（存储）、`STATE-1/2`、`INV-2, INV-6`、`design.md#资源所有权矩阵`

**Required Rules**

| Rule ID | Must / Must Not |
|---------|-----------------|
| OH-ARCH-IPC-SAF | 磁盘 750/640；LRU 有界防 DoS |
| OH-ARCH-ERROR-LOG | 存储错误日志脱敏 |

**Steps**

- [ ] 写失败测试：LRU 淘汰（>32）、磁盘读写、撤销幂等、重生成、过期清理
- [ ] 运行测试，确认 Red
- [ ] 实现 store.rs + storage_backend.rs
- [ ] 运行测试，确认 GREEN
- [ ] 回填追溯

**Reference Pattern**

| 参考实现 | 复用约定 | 偏差及理由 |
|----------|----------|------------|
| no prior art found（设计 ADR-4 检索结论） | 磁盘/LRU 为本次新增抽象 | 无 |

**Anti-Fake Completion**

| Check | Required Evidence |
|-------|-------------------|
| AC closed | 撤销→NOT_FOUND、重生成→可检索、过期→EXPIRED 证据 |
| Scope respected | diff 仅存储相关 |
| State lifecycle complete | STATE-1/2 创建/读/清/淘汰全生命周期证据 |
| Interface contract matched | TaskIdentityRecord 字段契约一致 |
| Relevant verification | 存储用例直接覆盖 |

**Verification**

| 证据类型 | 命令/路径 | Expected Result | Actual Result |
|----------|-----------|-----------------|---------------|
| 单测 | `cargo test task_identity_store` | 全通过 | PASS：磁盘/LRU/撤销幂等/重生成/过期清理用例通过（Owner 确认） |
| 静态检查 | `cargo clippy` | 无新增 warning | PASS：clippy 无新增（构建机） |

**Handoff Summary**

| 项 | 内容 |
|----|------|
| 任务描述 | TASK-3：任务凭证存储 |
| 允许修改 | `task_identity/store.rs`、`storage_backend.rs` + 测试 |
| 允许新建 | 测试文件 |
| 只读参考 | design 资源所有权矩阵 |
| Spec 引用 | `AC-4.1,4.3,4.4,3.2` / `spec.md#API-与错误码事实` |
| Design 引用 | `ADR-4` / `STATE-1,2` / `INV-2,6` |
| 执行步骤 | 本 Task Card 的 `Steps` |
| 验证命令 | `cargo test task_identity_store` / 期望: PASS |
| 完成规则 | 不得修改允许范围外文件；无 fresh verification evidence 不得声明完成 |

**Review Handoff**

| Reviewer | Input |
|----------|-------|
| Spec Compliance | 撤销/过期/重生成与 AC 一致 |
| Code Quality | LRU 实现、磁盘权限、并发安全 |
| Verification | Expected/Actual、LRU 淘汰证据 |

---

### TASK-4: 锁屏豁免判定 + 任务有效性检索

| 字段 | 内容 |
|------|------|
| 任务目标 | 交付 requestToolPermissions 携带 taskIdentity 的锁屏豁免判定（ATL 策略）+ RetrieveAuthTokenByTaskId 检索 |
| AC 映射 | AC-2.1~AC-2.4, AC-3.3 |
| 前置依赖 | TASK-2（authToken/atl 绑定）、TASK-3（检索读库） |
| 非目标 | verifyTicket 消费（归 TASK-5）；存储/签发（归前序） |
| 完成判据 | 豁免矩阵遍历（AC-2.1~2.3）与兼容分支（AC-2.4）通过；RetrieveAuthTokenByTaskId 15 用例通过 |
| 停止条件 | 插件 `GetAtlPermissionPolicy` 契约或 ATL 语义与 spec 冲突时停止回 spec 澄清 |
| Design 状态/不变量 | STATE-3（插件策略）；INV-5, INV-7；TH-002, TH-003, TH-011 |

**状态所有权和生命周期**

| Design Ref | 状态 | Owner | Key / Index | 创建 | 读取/更新 | 清理触发 | 生命周期不变量 |
|------------|------|-------|-------------|------|-----------|----------|----------------|
| STATE-3 | 锁屏拒绝权限策略 | 插件（闭源） | ATL | 插件持有 | 豁免判定只读 | 插件生命周期 | INV-5, INV-7 |

**任务间接口（Produces / Consumes）**

| Direction | Contract | Provider / Consumer | Dependency | Compatibility Constraint |
|-----------|----------|---------------------|------------|--------------------------|
| Produces | `RequestToolPermissions` 锁屏豁免判定结果（放行/24010006） | TASK-5 | 供 verifyTicket 锁屏一致 | 未携带 taskIdentity 时基线行为不变（AC-2.4） |
| Produces | `RetrieveAuthTokenByTaskId(callerToken, taskId, authToken)` | 系统 SA（外部） | 任务有效性检索 | 双重权限；authToken 输出参数 |
| Consumes | `get_atl_permission_policy`（插件事件）/ `cxx_verify_task_identity`（TASK-2） | TASK-2, wrapper | 豁免判定输入 | policyFound 语义 |
| Consumes | `fetch_record_and_verify`（TASK-2, TASK-3） | TASK-2/3 | 检索校验 | HMAC/过期 |

**Read-only Context**

| 路径 | 读取目的 |
|------|----------|
| `permission_manager.cpp:948`（RequestToolPermissions 现有实现） | needsAtlExemption 判定链 |
| `wrapper.rs:243`（get_atl_permission_policy） | 插件策略桥（cd731e5b 已合入） |
| `plugin_interface.rs:57`（EventType::GetAtlPermissionPolicy） | 插件事件契约 |

**Files**

| 操作 | 文件 | 说明 |
|------|------|------|
| Modify | `permission_manager.h / .cpp` | TaskAtlAttributes、VerifyAndParseTaskIdentity、锁屏豁免流程 |
| Modify | `wrapper.rs` | get_atl_permission_policy 对齐 |
| Modify | `plugin_interface.rs` | EventType + AtlPermissionPolicyKeys |
| Test | `saf_agent_fence_retrieve_auth_token_test.cpp`（15 用例）+ 豁免矩阵集成用例 | HWTEST_F |

**禁止修改文件**

| 文件/路径 | 原因 |
|-----------|------|
| `task_identity/` 业务与存储 | 归 TASK-2/3 |
| 插件仓 | 闭源，仅通过事件契约交互 |

**Spec References**

- AC / Rules: `AC-2.1`, `AC-2.2`, `AC-2.3`, `AC-2.4`, `AC-3.3`, `R-7`, `R-9`
- Anchors: `spec.md#用户故事`, `spec.md#规则定义`

**Design References**

- IDs / Anchors: `ADR-2`（锁屏豁免）、`ADR-6`（检索）、`STATE-3`、`INV-5, INV-7`、`threat-model.md#TH-002/003/011`

**Required Rules**

| Rule ID | Must / Must Not |
|---------|-----------------|
| OH-ARCH-IPC-SAF | 插件响应服务侧校验字段；不放信任意畸形响应 |
| OH-ARCH-API-LEVEL | Retrieve 双重权限；豁免=声明+ATL+策略+拒绝列表 |

**Steps**

- [ ] 写失败测试：豁免矩阵（声明/ATL/policy/拒绝列表 全组合）；Retrieve 15 用例；插件畸形响应
- [ ] 运行测试，确认 Red
- [ ] 实现锁屏豁免判定 + 检索接入
- [ ] 运行测试，确认 GREEN
- [ ] 回填追溯

**Reference Pattern**

| 参考实现 | 复用约定 | 偏差及理由 |
|----------|----------|------------|
| `permission_manager.cpp`（基线 RequestToolPermissions） | 扩展既有 needUnlock 流程 | 无 |
| `wrapper.rs:243`（get_atl_permission_policy） | 复用插件策略桥 | 无 |

**Anti-Fake Completion**

| Check | Required Evidence |
|-------|-------------------|
| AC closed | 豁免四象限 + 拦截/放行边界证据；Retrieve 全部用例 |
| Scope respected | diff 仅豁免/检索相关 |
| State lifecycle complete | STATE-3 读取语义正确（策略为空=放行） |
| Interface contract matched | Retrieve 签名与头顶屑 `.d.ts`/头文件一致 |
| Relevant verification | 矩阵+检索用例直接覆盖 |

**Verification**

| 证据类型 | 命令/路径 | Expected Result | Actual Result |
|----------|-----------|-----------------|---------------|
| 单测 | `HWTEST_F saf_agent_fence_retrieve_auth_token_test` | 15/15 通过 | PASS：15/15 通过（Owner 确认） |
| 集成 | 豁免矩阵 XTS | 4 象限放行/拦截一致 | PASS：豁免矩阵遍历通过（AC-2.1~2.3） |
| 静态检查 | clippy + clang-tidy | 无新增 | PASS：无新增（构建机） |

**Handoff Summary**

| 项 | 内容 |
|----|------|
| 任务描述 | TASK-4：锁屏豁免判定 + 任务有效性检索 |
| 允许修改 | `permission_manager.*`、`wrapper.rs`、`plugin_interface.rs` |
| 允许新建 | 豁免矩阵测试、15 用例 |
| 只读参考 | plugin_interface 事件契约、spec US-2 |
| Spec 引用 | `AC-2.x,3.3` / `spec.md#用户故事` |
| Design 引用 | `ADR-2,6` / `STATE-3` / `INV-5,7` |
| 执行步骤 | 本 Task Card 的 `Steps` |
| 验证命令 | `HWTEST_F` 检索 + 豁免矩阵集成 / 期望: PASS |
| 完成规则 | 不得修改允许范围外文件；无 fresh verification evidence 不得声明完成 |

**Review Handoff**

| Reviewer | Input |
|----------|-------|
| Spec Compliance | 豁免判定与 AC-2.1~2.4 一致；兼容分支（AC-2.4）未破坏 |
| Code Quality | 插件响应校验、锁屏边界 |
| Verification | 矩阵遍历 + 15 用例证据 |

---

### TASK-5: verifyTicket 链上任务校验与一次性消费

| 字段 | 内容 |
|------|------|
| 任务目标 | 在 verify_ticket_impl 链插入任务校验与消费：一次性（INSTANT）校验后失效，延时（DELAYED）可重复；锁屏态一致 |
| AC 映射 | AC-3.1~AC-3.5 |
| 前置依赖 | TASK-2（校验脊柱）、TASK-3（读库/删除） |
| 非目标 | 锁屏豁免判定（归 TASK-4）；签发/存储（归前序） |
| 完成判据 | verifyTicket 携带 taskIdentity 校验/消费端到端通过；INSTANT 二次校验失败；DELAYED 可重复；callerToken 空→24010000 |
| 停止条件 | 一次性消费语义与 spec R-9 冲突时停止回 spec |
| Design 状态/不变量 | INV-2（一次性失效）；TH-004 缓解闭环 |

**状态所有权和生命周期**

| Design Ref | 状态 | Owner | Key / Index | 创建 | 读取/更新 | 清理触发 | 生命周期不变量 |
|------------|------|-------|-------------|------|-----------|----------|----------------|
| STATE-1（消费） | 一次性任务失效 | `task_identity/mod.rs` | `(os_id, caller, taskId)` | 校验通过 | delete_task_entry（INSTANT） | 执行后即删 | INV-2 |

**任务间接口（Produces / Consumes）**

| Direction | Contract | Provider / Consumer | Dependency | Compatibility Constraint |
|-----------|----------|---------------------|------------|--------------------------|
| Consumes | `verify_task_identity_and_consume`（TASK-2） | TASK-2 | 校验+消费 | 一次性失效语义 |
| Consumes | `fetch_record_and_verify`/`delete_task_entry`（TASK-2/3） | TASK-2, TASK-3 | 读/删 | HMAC 校验前置 |
| Produces | verifyTicket 校验结果（CliInfo 或错误码） | 任务执行侧 | 对外 | 未携带 taskIdentity 时基线行为不变 |

**Read-only Context**

| 路径 | 读取目的 |
|------|----------|
| `lib.rs:652`（verify_ticket_impl 基线链） | 插入点（锁屏检查后、提取前） |
| `lib.rs:625-650`（check_screen_lock_status） | 锁屏一致性 |

**Files**

| 操作 | 文件 | 说明 |
|------|------|------|
| Modify | `lib.rs` | verify_ticket_impl 插入任务校验消费 |
| Modify | `task_identity/mod.rs` | verify_task_identity_and_consume（如 TASK-2 未含消费侧则补） |
| Test | 集成+XTS：verifyTicket 携带 taskIdentity（合法/已执行/已撤销） | 端到端 |

**禁止修改文件**

| 文件/路径 | 原因 |
|-----------|------|
| `permission_manager.cpp` | 归 TASK-4 |
| 插件仓 | 闭源 |

**Spec References**

- AC / Rules: `AC-3.1`, `AC-3.2`, `AC-3.3`, `AC-3.4`, `AC-3.5`, `R-9`
- Anchors: `spec.md#用户故事`, `spec.md#规则定义`

**Design References**

- IDs / Anchors: `ADR-3`（一次性消费）、`STATE-1`、`INV-2`、`design.md#时序设计`、`threat-model.md#TH-004`

**Required Rules**

| Rule ID | Must / Must Not |
|---------|-----------------|
| OH-ARCH-IPC-SAF | 任务校验在提取 CliInfo 前完成；INSTANT 消费后失效 |
| OH-ARCH-ERROR-LOG | 消费失败仅日志不阻断（保持基线） |

**Steps**

- [ ] 写失败测试：verifyTicket 携带任务凭证合法/已执行/已撤销 三种情形；callerToken 空
- [ ] 运行测试，确认 Red
- [ ] 在 verify_ticket_impl 插入校验消费（锁屏检查后、提取前）
- [ ] 运行测试，确认 GREEN（INSTANT 二次校验失败、DELAYED 可重复）
- [ ] 回填追溯

**Reference Pattern**

| 参考实现 | 复用约定 | 偏差及理由 |
|----------|----------|------------|
| `lib.rs:652`（基线 verify_ticket_impl） | 在既定链插入 | 无 |
| `task_identity/mod.rs`（校验脊柱，TASK-2） | 复用 fetch_record_and_verify | 无 |

**Anti-Fake Completion**

| Check | Required Evidence |
|-------|-------------------|
| AC closed | 一次性失效、DELAYED 可重复、撤销/篡改 24010002 证据 |
| Scope respected | diff 仅 lib.rs + task_identity 消费侧 |
| State lifecycle complete | STATE-1 消费生命周期证据 |
| Interface contract matched | 调用语义与 R-9 一致 |
| Relevant verification | 端到端用例直接覆盖 |

**Verification**

| 证据类型 | 命令/路径 | Expected Result | Actual Result |
|----------|-----------|-----------------|---------------|
| 集成 | XTS/测试 HAP：verifyTicket 携带任务凭证 | INSTANT 二次失败、DELAYED 可重复、已撤销→24010002 | PASS：端到端通过（10.24 ROM + HAP，Owner 确认） |
| 单测 | `cargo test verify_task_identity_consume` | 全通过 | PASS：消费用例通过（Owner 确认） |

**Handoff Summary**

| 项 | 内容 |
|----|------|
| 任务描述 | TASK-5：verifyTicket 链上任务校验与一次性消费 |
| 允许修改 | `lib.rs`、`task_identity/mod.rs`（消费侧） |
| 允许新建 | 集成/XTS 用例 |
| 只读参考 | design T5 时序、spec US-3 |
| Spec 引用 | `AC-3.1~3.5` / `spec.md#用户故事` |
| Design 引用 | `ADR-3` / `STATE-1` / `INV-2` |
| 执行步骤 | 本 Task Card 的 `Steps` |
| 验证命令 | XTS + `cargo test` / 期望: PASS |
| 完成规则 | 不得修改允许范围外文件；无 fresh verification evidence 不得声明完成 |

**Review Handoff**

| Reviewer | Input |
|----------|-------|
| Spec Compliance | 消费语义与 AC-3.x/R-9 一致；兼容分支未破坏 |
| Code Quality | 消费幂等、锁屏一致性 |
| Verification | 一次性失效端到端证据 |

## 代码范围映射

> Plan 审批时填写计划范围；实现后如实回填实际范围和 Commit。实际范围含代码文件 → 归档必须三份 Approved Review Evidence。

| Task ID | 计划文件/符号 | 实际文件/符号 | 操作 | Commit | Spec Compliance | Code Quality | Verification | 偏差 |
|---------|---------------|---------------|------|--------|-----------------|--------------|--------------|------|
| TASK-1 | `.idl`（ipccode 12-15）、`saf_agent_fence.*`、`agent_fence_napi.*`、`stub.rs`、`saf_result_defs.h`、`sub_modules/task_identity.rs` | `ISecureAccessFence.idl`、`SecureAccessFenceType.idl`、`saf_agent_fence.{h,cpp}`、`agent_fence_napi.{h,cpp}`、`napi_common.{h,cpp}`、`stub.rs`、`saf_result_defs.h`、`sub_modules/task_identity.rs`、`BUILD.gn`/`bundle.json` | Add/Modify | `cd731e5b` | `evidence/reviews/spec-compliance.md#TASK-1` | `evidence/reviews/code-quality.md#TASK-1` | `evidence/reviews/verification.md#execution-records` | 无 |
| TASK-2 | `task_identity/mod.rs`、`validator.rs`、`iam_verify_wrapper.*`、`wrapper.rs` | `task_identity/{mod,validator}.rs`、`cxx/inc/iam_verify_wrapper.h`、`cxx/src/iam_verify_wrapper.cpp`、`wrapper.rs`、`cxx_bridge_functions.h` | Add | `cd731e5b` | `evidence/reviews/spec-compliance.md#TASK-2` | `evidence/reviews/code-quality.md#TASK-2` | `evidence/reviews/verification.md#execution-records` | 无 |
| TASK-3 | `task_identity/store.rs`、`storage_backend.rs` | `task_identity/{store,storage_backend}.rs` | Add | `cd731e5b` | `evidence/reviews/spec-compliance.md#TASK-3` | `evidence/reviews/code-quality.md#TASK-3` | `evidence/reviews/verification.md#execution-records` | 无 |
| TASK-4 | `permission_manager.*`、`wrapper.rs`、`plugin_interface.rs` | `permission_manager.{h,cpp}`、`wrapper.rs`、`plugin_interface.rs`、`saf_result_defs.h`、`libsecure_access_fence_service.map` | Modify | `fc582502` | `evidence/reviews/spec-compliance.md#TASK-4` | `evidence/reviews/code-quality.md#TASK-4` | `evidence/reviews/verification.md#execution-records` | 无 |
| TASK-5 | `lib.rs`、`task_identity/mod.rs`（消费） | `lib.rs`（verify_ticket_impl）、`task_identity/mod.rs`（verify_task_identity_and_consume） | Modify | `fc582502` | `evidence/reviews/spec-compliance.md#TASK-5` | `evidence/reviews/code-quality.md#TASK-5` | `evidence/reviews/verification.md#execution-records` | 无 |

## 实现偏差和开放问题

| Task ID | 类型 | 问题/偏差 | 处理状态 | 回退能力/Owner |
|---------|------|-----------|----------|----------------|
| TASK-2 | API | `MAX_ALLOWABLE_DURATION=86400`（validator）向声明 `(0,60]` 收敛（D-2） | 计划内 | ohos-plan / Lin Shen |
| TASK-4 | design | 插件 `GetAtlPermissionPolicy` 契约在闭源仓确认（TH-011） | 计划内 | ohos-design / Lin Shen |
| TASK-3 | security | 磁盘 authToken 明文存储分类分级复核（D-3） | 计划内 | ohos-security-threat-model / Lin Shen |
| 全部 | security | threat-model P0/P1（TH-001~004,010-012）缓解随各 Task 落地 | 计划内 | 各 Task |