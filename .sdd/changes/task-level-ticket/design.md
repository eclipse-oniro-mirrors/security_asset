# Design

## Context and constraints

- 业务目标：为 CLI 工具提供任务级身份管理（challenge → IAM 认证 → identity 签发 → 撤销 → authToken 检索）。
- 4 个独立 API，不集成 requestToolPermissions/verify_ticket 链路（ATL 豁免与 ticket 内嵌 task 信息移出范围）。
- IAM 通过 CXX bridge 调用真实 `UserAccessCtrlClient::VerifyAuthToken`（非打桩）。
- 存储层通过 `StorageBackend` trait 抽象，`store.rs` 零 `#[cfg]`。
- 内存使用 LRU 32 缓存（~24KB），磁盘为 source of truth。

## Architecture

```
NAPI (agent_fence_napi.cpp)
  ↓ ValidateTaskIdentityInfo → SafAgentFence::Generate*
Inner API (saf_agent_fence.cpp)
  ↓ SecureAccessFenceProxy (IDL generated)
IPC (stub.rs, ipccode 12-15)
  ↓ deserialize_task_identity_info / validator
SAFService (lib.rs)
  ↓ check_query_tool_permission / check_retrieve_auth_token_permission
task_identity/mod.rs
  ↓ generate / revoke / retrieve + IAM + HMAC
task_identity/store.rs (Box<dyn StorageBackend>)
  ↓ LruMap memory cache + disk I/O delegate
storage_backend.rs
  ├─ DiskStorage (std::fs, FILE_LOCKS)
  └─ MockStorage (no-op)
```

## Key design decisions

### D1: IPC codes 12-15

| ipccode | API |
|---------|-----|
| 12 | GenerateTaskChallenge |
| 13 | GenerateTaskIdentity |
| 14 | RevokeTaskIdentity |
| 15 | RetrieveAuthTokenByTaskId (Inner API only, 无 NAPI) |

RetrieveAuthTokenByTaskId 仅 C++ Inner API（`saf_agent_fence.h`），不注册 NAPI。

### D2: Dual-challenge HMAC

复用 `batch_generate_ticket` 的双 challenge 模式：
- challenge1（32 字节随机）：派生 `session_key = TicketKeyManager.derive_ticket_session_key(os_account_id, "", challenge1)`
- challenge2（32 字节随机）：追加到 HMAC 数据
- HMAC data = `taskAttributes.bytes + calling_uid.to_be_bytes() + taskId.bytes + challenge2`
- `identity = base64(HMAC_SHA256(data, session_key))`
- `challenge = base64(challenge1 + challenge2)`（64 字节）
- verify 时从 challenge 拆分 challenge1/challenge2，重算 HMAC 比对 identity

### D3: IAM 集成（真实调用）

`iam_verify_wrapper.cpp`（独立文件，从 `secure_access_fence_service.cpp` 抽离）：
- `SyncVerifyCallback`：继承 `VerifyTokenCallback`，`promise/future` 同步等待
- `VerifyAuthToken(authToken, allowableDurationMs)`：调用 `UserAccessCtrlClient::GetInstance().VerifyAuthToken`，10 秒超时，返回 `{result_code, challenge, auth_trust_level}`
- CXX bridge：`wrapper.rs` 声明 `fn VerifyAuthToken(...) -> IamVerifyResult`

SELinux：`secure_access_fence.te` 添加 `allow secure_access_fence sa_useriam_userauth_service:samgr_class { get }; binder_call(secure_access_fence, useriam);`

### D4: os_account_id 解析

所有 task identity API 的 stub（`GenerateTaskChallenge`/`GenerateTaskIdentity`/`RevokeTaskIdentity`/`RetrieveAuthTokenByTaskId`）统一使用 `GetOsAccountIdFromTokenId` 解析 os_account_id：
- `GetCallingTokenId()` → `GetOsAccountIdFromTokenId(token)`：
  - `TOKEN_HAP` → `GetHapTokenInfo → hapInfo.userID`
  - `TOKEN_NATIVE`（SA）/其他 → 返回 0
- RetrieveAuthTokenByTaskId 使用入参 `callerToken`，其余 3 个使用 `GetCallingTokenId()`

### D5: 权限校验

| API | 校验函数 | 条件 |
|-----|----------|------|
| generate* / revoke | `check_query_tool_permission` | CheckPermission(QUERY_TOOL_PERMISSIONS) + CheckIsSystemHap() |
| RetrieveAuthTokenByTaskId | `check_retrieve_auth_token_permission` | CheckIsSystemSa() + CheckPermission(QUERY_TOOL_PERMISSIONS) + CheckPermission(INTERACT_ACROSS_LOCAL_ACCOUNTS) |

`CheckIsSystemSa()`（新增 `access_token_wrapper.cpp`）：`GetTokenTypeFlag == TOKEN_NATIVE`

### D6: 存储架构（StorageBackend trait + LRU 内存）

```
storage_backend.rs
  trait StorageBackend: Send + Sync {
      write_record_file / read_record_file / delete_record_file
      write_challenge_file(os_account_id, caller, task_id, ...) / read_challenge_file(...) / delete_challenge_file(...)
      cleanup_disk_expired / cleanup_stale_task_files / cleanup_stale_challenge_files
      cleanup_file_locks / clear_file_locks / enumerate_os_account_ids
  }
  create_storage() →
    #[cfg(not(SAFTest))] Box::new(DiskStorage)
    #[cfg(SAFTest)]     Box::new(MockStorage)
```

`store.rs` 内存使用 `LruMap`（HashMap + VecDeque，cap=32），磁盘委托 `Box<dyn StorageBackend>`。`#[cfg]` 仅存在于 `create_storage()` 中（2 行）。

### D7: LRU 内存管理

`LruMap<K, V>`：`HashMap` 查找 + `VecDeque` 访问顺序（front=LRU, back=MRU）。
- `get()` → 命中后 touch（key 移到尾部）
- `put()` → 已存在则更新+touch；新增且满（≥32）→ `pop_front` 淘汰 LRU → insert
- `remove()` → 从 map 和 order 删除
- `iter()` → 遍历（用于 cleanup）

`insert_challenge` / `insert_task_entry`：磁盘写入 → LRU put（满自动淘汰，不拒绝）
`get_challenge` / `get_task_record`：内存优先（LRU touch）→ 磁盘兜底
`cache_to_memory`：磁盘命中回写 LRU put（可能淘汰旧条目）
`delete_task_entry`：磁盘删除 → LRU remove

### D8: on_idle 清理策略

`clean_expired_instant_tasks`：
1. 直接 `inner.entries = LruMap::new(); inner.challenges = LruMap::new()` — 旧 LruMap 自动 drop 释放内存
2. 磁盘清理：`storage.cleanup_disk_expired` 删除过期 INSTANT 任务 + 过期 challenge
3. 文件锁清理：`storage.cleanup_file_locks`

不再逐条过滤内存条目，直接全量清空（磁盘是 source of truth，后续按需回填）。

### D9: Challenge 存储

Challenge 以 `(caller, task_id)` 为 key，不同 taskId 可并行持有各自 pending challenge：
- 路径：`taskChallenges/{caller}/{taskId}.json`
- 内存 key：`LruMap<(String, String), ChallengeEntry>`（caller, task_id）
- 超时：10 分钟 boot-time（`GetBootTimeMs()`）
- `get_challenge` 只读不删
- `insert_challenge` / `get_challenge` / `delete_challenge` 均需 `task_id` 参数

### D10: NAPI 错误码映射

NAPI 层通过 `MapErrorCode` 将 SAF 内部码映射为 5 种公开码：
- 201（权限）/ 202（非系统应用）/ 24010000（参数无效）/ 24010001（服务异常）/ 24010002（内部错误）

### D11: Revoked set 已移除

不再维护 revoked set：
- `delete_task_entry`：仅磁盘删除 + LRU remove，不标记 revoked
- `retrieve_auth_token_by_task_id`：不检查 revoked，直接 `get_task_record`
- 撤销后检索返回 `TASK_IDENTITY_NOT_FOUND`（而非 `TASK_IDENTITY_REVOKED`）
- `TaskIdentityRevoked`（0x71007）错误码保留定义但不再返回

## File inventory

| 文件 | 职责 |
|------|------|
| `task_identity/mod.rs` | 核心业务：generate/revoke/retrieve + IAM + HMAC |
| `task_identity/store.rs` | LRU 内存缓存（LruMap 32） + StorageBackend 委托（零 #[cfg]） |
| `task_identity/storage_backend.rs` | trait + DiskStorage + MockStorage（仅 2 行 #[cfg]） |
| `task_identity/validator.rs` | 纯参数校验（与 napi_common.cpp 同步） |
| `cxx/src/iam_verify_wrapper.cpp` | IAM VerifyAuthToken + SyncVerifyCallback |
| `cxx/inc/iam_verify_wrapper.h` | 前向声明 |
| `cxx/src/secure_access_fence_service.cpp` | GetCallingTokenId + GetOsAccountIdFromTokenId |
| `stub.rs` | IPC 分发（ipccode 12-15）+ 序列化 |
| `sub_modules/task_identity.rs` | IPC 序列化/反序列化（含 u8 范围校验 U8_MAX） |
| `napi_common.cpp` | NAPI 解析 + ValidateTaskIdentityInfo + typedarray type 校验 + data 判空 |
| `agent_fence_napi.cpp` | 3 个 NAPI 函数（逐元素 int32→uint8 转换） |
| `saf_agent_fence.{h,cpp}` | Inner API（4 个方法） |

## Alternatives considered

- **ATL 豁免集成 requestToolPermissions**：移出范围
- **verify_ticket task 校验**：移出范围
- **IAM 打桩 ATL=3**：已替换为真实 IAM 调用
- **per-caller 512 上限 + 全量清空**：已替换为 LRU 32 + 自动淘汰
- **revoked set**：已移除（仅影响错误码 Revoked vs NotFound，无功能差异）
- **#[cfg(not(SAFTest))] 散落 store.rs**：已通过 StorageBackend trait 消除
