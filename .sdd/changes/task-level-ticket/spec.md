# 任务级身份管理（Task Identity Management）

## Intent

为 CLI 工具提供任务级身份管理能力：低危任务直接生成 task identity；高危任务经 IAM 认证后生成带 authToken 的 task identity。支持 challenge 生成、identity 签发、撤销、以及通过 callerToken+taskId 检索 authToken。本阶段为 4 个独立 API，不集成 requestToolPermissions/verify_ticket 链路。

## Requirements

### R1: generateTaskChallenge（高危认证路径）

- Given 调用方（系统应用，持有 `ohos.permission.QUERY_TOOL_PERMISSIONS`）传入合法 `TaskIdentityInfo`（`taskId` 非空、`taskExpireTime` 非0、INSTANT 不能为 -1）
- When 调用 `generateTaskChallenge`
- Then 服务生成 32 字节随机 challenge，以 `(caller_token, task_id)` 为 key 写入 pending challenge 缓存（磁盘+内存 LRU，10 分钟 boot-time 超时），返回 `Uint8Array`
- 同一 `(caller, taskId)` 重复调用覆盖旧 challenge；不同 taskId 可并行持有各自 challenge
- 磁盘路径：`taskChallenges/{caller}/{taskId}.json`
- 未持权限返回错误码 201；非系统应用返回 202

### R2: generateTaskIdentity（双模式）

- Given 调用方传入 `TaskIdentityInfo`，可选携带 `authToken`
- When 调用 `generateTaskIdentity`
- Then 按是否携带 `authToken` 分两条路径：

  1. **携带 authToken（高危路径）**：通过 CXX bridge 调用 IAM `VerifyAuthToken`（`iam_verify_wrapper.cpp` → `UserAccessCtrlClient::VerifyAuthToken`），获取 IAM 返回的 challenge 和 authTrustLevel；以 `(caller, task_id)` 查询 pending challenge 比对，不匹配返回 `TASK_IDENTITY_MISMATCH(0x71008)`；匹配后构建 `taskAttributes` JSON（含 `callerTokenId` + `authToken`(base64) + `atl`），使用**双 challenge 模式**：challenge1 派生 session_key，challenge2 追加到 HMAC 数据，`identity = base64(HMAC_SHA256(taskAttributes + taskId + challenge2, session_key))`，`challenge = base64(challenge1 + challenge2)`（superseded 于 `task-identity-bind-caller-token`，移除动态 calling_uid）
  2. **未携带 authToken（低危路径）**：不要求 pending，直接构建 `taskAttributes`（不含 `authToken`/`atl`），生成 `TaskIdentity`

- 磁盘始终先写，内存使用 LRU（32 条上限，满时淘汰最久未访问的条目）
- `taskExpireTime`：-1 表示永不过期（仅 DELAYED），正数为持续时间（存储时转为绝对 epoch 秒）
- NAPI 参数校验：taskId 必填、executeType 仅 1(INSTANT)/2(DELAYED)、authToken ≤4096 字节、allowableDuration 0 或 1~86400

### R3: revokeTaskIdentity

- Given 调用方传入 `taskIds: string[]`（1~100 个，每个非空、无路径穿越字符）
- When 调用 `revokeTaskIdentity`
- Then 删除磁盘 `{taskId}.json` 和内存 LRU 条目；幂等操作，不存在的 taskId 不报错
- 撤销后检索返回 `TASK_IDENTITY_NOT_FOUND`（不再有 `TASK_IDENTITY_REVOKED`）

### R4: RetrieveAuthTokenByTaskId（Inner API，C++ only）

- Given 系统 SA 调用方（`TOKEN_NATIVE`，持有 `ohos.permission.QUERY_TOOL_PERMISSIONS` + `ohos.permission.INTERACT_ACROSS_LOCAL_ACCOUNTS`）传入 `callerToken` 和 `taskId`
- When 调用 `RetrieveAuthTokenByTaskId`
- Then 通过 `GetOsAccountIdFromTokenId(callerToken)` 解析 os_account_id（HAP→GetHapTokenInfo→userID，SA→0）；查内存/磁盘记录；检查过期；验证 HMAC（superseded：原含 calling_uid 防篡改，见 `task-identity-bind-caller-token`）；提取并返回 authToken 字节

### R5: 存储与生命周期

- `StorageBackend` trait 抽象磁盘 I/O：`DiskStorage`（生产）+ `MockStorage`（测试），`store.rs` 零 `#[cfg]`
- 内存缓存：全局 `OnceLock<TaskIdentityStore>`，LRU 32 条上限（challenge 和 task 各独立 LruMap）
- `LruMap`：`HashMap + VecDeque`，get 时 touch（更新访问顺序），put 时满则淘汰 front（LRU）
- 磁盘：`el2/{os_account_id}/secure_access_fence/agent_plugin/taskAttributes/{caller}/{taskId}.json`（任务记录）、`taskChallenges/{caller}/{taskId}.json`（challenge），目录 0o750、文件 0o640，per-file Mutex
- `insert_task_entry`：磁盘写入 → LRU put（满则自动淘汰）
- `get_task_record`：内存优先（LRU touch），磁盘兜底，命中后 `cache_to_memory` 回写 LRU
- `on_idle`：直接清空两个 LruMap（`inner.entries = LruMap::new()`，旧 LruMap 自动 drop 释放内存），磁盘清理交给 `storage.cleanup_disk_expired`
- `taskExpireTime=-1`（仅 DELAYED）：永不过期（-1i64 as u64 = u64::MAX）
- 内存最大占用：challenge ~6.4KB（32×200B）+ task ~17.6KB（32×550B）= **~24KB**

### R6: 错误码

| 内部码 | NAPI 码 | 含义 |
|--------|---------|------|
| `TASK_IDENTITY_NOT_FOUND 0x71005` | 24010002 | 任务未找到（含撤销后检索） |
| `TASK_IDENTITY_EXPIRED 0x71006` | 24010002 | 任务已过期 |
| `TASK_IDENTITY_REVOKED 0x71007` | 24010002 | 已定义但不再返回（revoked set 已移除） |
| `TASK_IDENTITY_MISMATCH 0x71008` | 24010002 | HMAC 或 challenge 不匹配 |
| `CHALLENGE_LIMIT_EXCEEDED 0x71004` | 24010002 | 已定义但不再返回（LRU 自动淘汰替代） |
| `INVALID_CHALLENGE_SIZE 0x18003` | 24010002 | challenge 尺寸不足 |
| `SAF_ERR_ARG_INVALID 0x10001` | 24010000 | 参数无效 |
| — | 201 | 权限不足 |
| — | 202 | 非系统应用 |
| — | 24010001 | 服务异常 |

## Out of scope

- requestToolPermissions 携带 taskIdentity 校验
- ATL 锁屏豁免（`cli_min_atl_levels`、`TryApplyTaskAtlExemption`）
- verify_ticket task identity 校验与 INSTANT 消费
- `PermissionQuery` IPC 载荷扩展（taskIdentity 字段已移除）
- 公开 JS 错误码（24010011+）

## Acceptance criteria

- [ ] AC1: `generateTaskChallenge` 对合法入参返回 32 字节 `Uint8Array`，challenge 以 `(caller, taskId)` 为 key 存储（10 分钟 boot-time 超时）
- [ ] AC2: 高危路径未先 challenge 或 challenge 过期，`generateTaskIdentity` 返回 `TASK_IDENTITY_NOT_FOUND`
- [ ] AC3: 高危路径 IAM challenge 不匹配返回 `TASK_IDENTITY_MISMATCH`
- [ ] AC4: `generateTaskIdentity` 返回的 challenge 为 `base64(challenge1+challenge2)`（64 字节解码），identity 为 `base64(HMAC)`，taskAttributes 含 authToken(base64)+atl
- [ ] AC5: `revokeTaskIdentity` 后再检索返回 `TASK_IDENTITY_NOT_FOUND`；重新生成相同 taskId 可正常检索
- [ ] AC6: `RetrieveAuthTokenByTaskId` 从 callerToken 解析 os_account_id（SA→0, HAP→userID）
- [ ] AC7: HMAC 验证态（superseded：原为"含 calling_uid，跨 uid 检索返回 `TASK_IDENTITY_MISMATCH`"，见 `task-identity-bind-caller-token`）；跨调用方验证在 callerToken 匹配时可通过对齐 RequestToolPermissions 的 HMAC 构造
- [ ] AC8: LRU 内存上限 32，满时自动淘汰最久未访问的条目（不影响磁盘文件）
- [ ] AC9: NAPI `generateTaskChallenge` resolve 正确逐元素转换 `int32_t→uint8_t`（非 memcpy）
- [ ] AC10: NAPI 校验 typedarray type == `napi_uint8_array`，非 Uint8Array 返回 24010000
- [ ] AC11: IPC `deserialize_i32_u8_list` 校验 val 范围 0~255（`U8_MAX`），越界返回 `InvalidArgument`
- [ ] AC12: `RetrieveAuthTokenByTaskId` 权限校验：仅系统 SA + QUERY_TOOL_PERMISSIONS + INTERACT_ACROSS_LOCAL_ACCOUNTS
- [ ] AC13: SA on_idle 清空全部内存缓存 + 磁盘清理过期 INSTANT 任务
