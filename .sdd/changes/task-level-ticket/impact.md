# Impact

## Affected surfaces

| Surface | Change |
| --- | --- |
| `SecureAccessFenceType.idl` | 新增 `ExecuteType`/`TaskIdentityInfo`(含 `authToken`)/`TaskIdentity` 类型 |
| `ISecureAccessFence.idl` | 新增 `[ipccode 12/13/14/15]` 四方法（12-14 为 NAPI 可达，15 为 Inner API only） |
| `saf_ipc` (Rust) | 新增 CMD 常量 12-15；`sub_modules/task_identity.rs` 序列化 + u8 范围校验（`U8_MAX`） |
| `task_identity/storage_backend.rs` (新) | `StorageBackend` trait + `DiskStorage` + `MockStorage`；`#[cfg]` 仅在 `create_storage()`（2 行） |
| `task_identity/store.rs` | LRU 内存缓存（`LruMap` HashMap+VecDeque, cap=32），零 `#[cfg]`；磁盘写入 → LRU put；移除 per-caller 限制和 revoked set |
| `task_identity/mod.rs` | 双 challenge HMAC + IAM 真实调用 + calling_uid 防篡改；移除 is_task_revoked 调用 |
| `task_identity/validator.rs` | 参数校验（与 napi_common.cpp 同步 10 种路径穿越 pattern） |
| `cxx/src/iam_verify_wrapper.cpp` (新) | `SyncVerifyCallback` + `VerifyAuthToken`（真实 IAM，从 service.cpp 抽离） |
| `cxx/src/secure_access_fence_service.cpp` | `GetCallingTokenId()` + `GetOsAccountIdFromTokenId()`；移除 IAM 代码 |
| `stub.rs` | 分发 ipccode 12-15；所有 task identity handler 统一用 `GetOsAccountIdFromTokenId` |
| `lib.rs` | 4 个 SAFService 方法 + `check_retrieve_auth_token_permission`；`on_idle` 遍历 os_account_id |
| `access_token_wrapper.{h,cpp}` | 新增 `CheckIsSystemSa()` |
| `agent_fence_napi.cpp` | 3 NAPI 函数（逐元素 int32→uint8 转换） |
| `napi_common.cpp` | ValidateTaskIdentityInfo + typedarray type 校验 (napi_uint8_array) + data 判空 |
| `saf_agent_fence.{h,cpp}` | 4 个 Inner API 静态方法 + proxy 调用 |
| `BUILD.gn` | `iam_verify_wrapper.cpp`；`user_auth_framework:userauth_client` 依赖 |
| `bundle.json` | `user_auth_framework` 加入 deps.components |
| 错误码 | `saf_result_defs.h` 增 0x71003/0x71004/0x71005-0x71008；NAPI 映射 201/202/24010000/24010001/24010002 |
| SELinux | `security_selinux_adapter` 仓库 `secure_access_fence.te` 添加 IAM 调用权限 |
| 符号导出 | `.map` 文件通配符导出 |
| C++ 测试 | `saf_agent_fence_retrieve_auth_token_test.cpp` 15 个用例 |

## Removed from original scope

- `PermissionQuery.taskIdentity` 字段（IDL/Rust/C++/NAPI 全部移除）
- ATL 锁屏豁免（`cli_min_atl_levels`/`TryApplyTaskAtlExemption`/`TaskAtlAttributes`）
- `verify_ticket` task identity 校验 + INSTANT 消费
- `permission_manager.cpp` task identity 相关修改（恢复原状）
- IAM 打桩 ATL=3（替换为真实调用）
- 全局任务数上限 1024 + per-caller 512 限制（替换为 LRU 32 + 自动淘汰）
- revoked set（`HashSet<RevokedKey>`、`is_task_revoked`、`TaskIdentityRevoked` 返回）
- `start_service` 中 `cleanup_stale_disk_records`（移除，由 on_idle + 懒检查覆盖）

## Data and migration

- 无既有持久化数据需要迁移
- 磁盘路径：`/data/service/el2/{os_account_id}/secure_access_fence/agent_plugin/taskAttributes/{caller}/{taskId}.json`（目录 0o750，文件 0o640）
- Challenge 路径：`/data/service/el2/{os_account_id}/secure_access_fence/agent_plugin/taskChallenges/{caller}/{taskId}.json`
- authToken base64 明文落盘（用户已确认）

## Risks

| Risk | Mitigation |
| --- | --- |
| IAM 调用超时 | `SyncVerifyCallback` 10 秒超时，返回 `IAM_VERIFY_RESULT_TIMEOUT` |
| HMAC 篡改 | HMAC data 含 calling_uid + taskId + challenge2，磁盘篡改均导致 mismatch |
| 路径穿越 | 10 种 pattern 在 NAPI 和 Rust validator 同步校验 |
| DoS | LRU 32 内存上限 + authToken ≤4096 字节 + taskAttributes JSON ≤8192 字节 |
| NAPI typedarray 类型错误 | 校验 `type == napi_uint8_array` + `data != nullptr` |
| IPC u8 值越界 | `deserialize_i32_u8_list` 校验 0~U8_MAX |
| 内存占用 | LRU 32 × 2（entries + challenges），最大 ~24KB；on_idle 全量清空释放 |
