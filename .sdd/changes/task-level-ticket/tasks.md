# Tasks

- [x] T1 — IDL 类型与接口：`SecureAccessFenceType.idl` 增 `ExecuteType`/`TaskIdentityInfo`(含 `authToken`)/`TaskIdentity`；`ISecureAccessFence.idl` 增 `[ipccode 12/13/14/15]` 四方法
- [x] T2 — `saf_ipc`：CMD 常量 12-15；`sub_modules/task_identity.rs` 序列化 + `deserialize_i32_u8_list` 含 u8 范围校验（`U8_MAX` 常量）
- [x] T3 — 错误码：`saf_result_defs.h` 增 0x71003/0x71004/0x71005-8；`definition` ErrCode 对应
- [x] T4 — `storage_backend.rs`（新）：`StorageBackend` trait + `DiskStorage` + `MockStorage` + `create_storage()`（仅 2 行 #[cfg]）；challenge 路径 `taskChallenges/{caller}/{taskId}.json`
- [x] T5 — `store.rs`：LRU 内存缓存（`LruMap` HashMap+VecDeque, cap=32）；磁盘写入 → LRU put；零 #[cfg]；移除 per-caller 限制和 revoked set
- [x] T6 — `mod.rs`：`generate_task_challenge`/`generate_task_identity`（双 challenge HMAC + IAM 真实调用）/`revoke_task_identities`/`retrieve_auth_token_by_task_id`（过期 + HMAC 验证，移除 is_task_revoked 调用）
- [x] T7 — IAM 集成：`iam_verify_wrapper.cpp`（SyncVerifyCallback + VerifyAuthToken）独立文件；`wrapper.rs` CXX bridge
- [x] T8 — `secure_access_fence_service.cpp`：`GetCallingTokenId()` + `GetOsAccountIdFromTokenId()`
- [x] T9 — `stub.rs`：分发 ipccode 12-15 + 序列化；所有 task identity handler 统一用 `GetOsAccountIdFromTokenId` 解析 os_account_id
- [x] T10 — `lib.rs`：4 个 SAFService 方法 + `check_retrieve_auth_token_permission`；`on_idle` 改用 `store.enumerate_os_account_ids()`；移除 `start_service` 中 `cleanup_stale_disk_records`
- [x] T11 — `access_token_wrapper.{h,cpp}`：新增 `CheckIsSystemSa()`
- [x] T12 — `saf_agent_fence.{h,cpp}`：4 个静态方法 + proxy 调用
- [x] T13 — NAPI：`agent_fence_napi.cpp` 3 函数（逐元素 int32→uint8）；`napi_common.cpp` ValidateTaskIdentityInfo + typedarray type 校验 + data 判空
- [x] T14 — `wrapper.rs`：CXX bridge 声明 `VerifyAuthToken`/`GetCallingTokenId`/`GetOsAccountIdFromTokenId`
- [x] T15 — 指标宏：`execute_with_metrics_single!` 于 `metrics_macro.rs`
- [x] T16 — `BUILD.gn`：`iam_verify_wrapper.cpp`；`user_auth_framework:userauth_client` 依赖
- [x] T17 — `bundle.json`：`user_auth_framework` 加入 deps.components
- [x] T18 — 符号导出：`.map` 文件通配符导出
- [x] T19 — SELinux：`secure_access_fence.te` 添加 IAM 调用权限
- [x] T20 — C++ 测试：`saf_agent_fence_retrieve_auth_token_test.cpp` 15 个 HWTEST_F 用例
- [x] T21 — `on_idle` 简化：直接清空 LruMap（`inner.entries = LruMap::new()`，旧值自动 drop）

## Verification evidence

| Criterion | Check | Result |
| --- | --- | --- |
| AC1-AC4（store + HMAC 逻辑） | 代码审阅 | 通过；需构建机编译验证 |
| AC5（撤销后检索返回 NOT_FOUND） | 代码审阅 | 通过；revoked set 已移除 |
| AC6（os_account_id 从 callerToken 解析） | 代码审阅 | 通过；所有 handler 统一用 GetOsAccountIdFromTokenId |
| AC7（HMAC 校验） | 代码审阅 | 通过（当时）；2026-10-23 变更见 `task-identity-bind-caller-token`：HMAC 不再含 calling_uid，taskAttributes 内嵌 callerTokenId |
| AC8（LRU 32 + 自动淘汰） | 代码审阅 | 通过；LruMap.put 满则 pop_front |
| AC9（NAPI 逐元素转换） | 代码审阅 | 通过；`bytes[i] = static_cast<uint8_t>(challenge[i])` |
| AC10（typedarray type 校验） | 代码审阅 | 通过；`type != napi_uint8_array` → INVALID_PARAMETER |
| AC11（IPC u8 范围校验） | 代码审阅 | 通过；`!(0..=U8_MAX).contains(&val)` → InvalidArgument |
| AC12（RetrieveAuthTokenByTaskId 权限） | 代码审阅 | 通过；`check_retrieve_auth_token_permission` 三重校验 |
| AC13（on_idle 清空 + 磁盘清理） | 代码审阅 | 通过；`inner.entries = LruMap::new()` + `cleanup_disk_expired` |
| 编译 | 构建机 | 待验证 |
| C++ 测试执行 | 设备 | 待验证 |

## Deferred

- requestToolPermissions 携带 taskIdentity 校验 + ATL 锁屏豁免（移出范围）
- verify_ticket task identity 校验 + INSTANT 消费（移出范围）
- `task_identity_test_cases.xlsx` 128 条测试用例待更新（revoked→NOT_FOUND）
