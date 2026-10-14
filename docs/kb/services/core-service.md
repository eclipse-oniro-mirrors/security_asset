# Core Service 模块 Context

> 文档版本：v1.0
> 更新时间：2026-10-13
> 来源：`services/asset/core_service/`、`services/secure_access_fence/core_service/`

## 定位

Core Service 是两个 SA 的核心实现层，包含 SA 入口、IPC Stub、业务操作分发、生命周期管理、公共事件监听、升级逻辑和系统事件上报。

## 快速路由

### 源码入口

### Asset Core Service（SA 8100）

| 文件 | 说明 |
|------|------|
| `services/asset/core_service/src/lib.rs` | AssetAbility、AssetService、SA 注册、生命周期 |
| `services/asset/core_service/src/stub.rs` | IPC Stub（on_remote_request） |
| `services/asset/core_service/src/operations.rs` | 操作分发模块 |
| `services/asset/core_service/src/operations/common.rs` | 共享操作辅助 |
| `services/asset/core_service/src/common_event.rs` | 公共事件订阅 |
| `services/asset/core_service/src/common_event/listener.rs` | 事件监听器 |
| `services/asset/core_service/src/common_event/start_event.rs` | 启动事件 |
| `services/asset/core_service/src/upgrade_operator.rs` | DB 升级 |
| `services/asset/core_service/src/upgrade_ce.rs` | CE 升级 |
| `services/asset/core_service/src/usage_statistics.rs` | 数据量统计上传 |
| `services/asset/core_service/src/sys_event.rs` | HiSysEvent 上报 |
| `services/asset/core_service/src/trace_scope.rs` | HiTrace 打点 |
| `services/asset/core_service/src/data_size_mod.rs` | 数据大小处理 |
| `services/asset/core_service/BUILD.gn` | 构建目标 `libasset_service.dylib.so` |

#### 操作模块

| 操作 | 文件 |
|------|------|
| Add | `services/asset/core_service/src/operations/operation_add.rs` |
| Remove | `services/asset/core_service/src/operations/operation_remove.rs` |
| Update | `services/asset/core_service/src/operations/operation_update.rs` |
| Query | `services/asset/core_service/src/operations/operation_query.rs` |
| PreQuery | `services/asset/core_service/src/operations/operation_pre_query.rs` |
| PostQuery | `services/asset/core_service/src/operations/operation_post_query.rs` |
| QuerySyncResult | `services/asset/core_service/src/operations/operation_query_sync_result.rs` |
| BatchAdd | `services/asset/core_service/src/operations/operation_batch_add.rs` |
| BatchRemove | `services/asset/core_service/src/operations/operation_batch_remove.rs` |
| BatchUpdate | `services/asset/core_service/src/operations/operation_batch_update.rs` |

### SAF Core Service（SA 66532）

| 文件 | 说明 |
|------|------|
| `services/secure_access_fence/core_service/src/lib.rs` | SAFAbility、SAFService、SA 注册 |
| `services/secure_access_fence/core_service/src/stub.rs` | IPC Stub |
| `services/secure_access_fence/core_service/src/wrapper.rs` | C++ FFI 桥接（cxx_is_screen_locked、notify_error、notify_performance_metrics） |
| `services/secure_access_fence/core_service/src/metrics_macro.rs` | execute_with_metrics! / execute_with_metrics_batch! 宏 |
| `services/secure_access_fence/core_service/src/remote_grant_status.rs` | 远程授权使能/禁能 |
| `services/secure_access_fence/core_service/src/common_event.rs` | 公共事件 |
| `services/secure_access_fence/core_service/src/cxx/secure_access_fence_ipc.cpp` | C++ IPC |
| `services/secure_access_fence/core_service/src/cxx/src/secure_access_fence_service.cpp` | C++ 服务辅助 |
| `services/secure_access_fence/core_service/src/cxx/src/permission_manager.cpp` | C++ 权限管理 |
| `services/secure_access_fence/core_service/src/cxx/src/time_wrapper.cpp` | C++ 时间包装 |
| `services/secure_access_fence/core_service/BUILD.gn` | 构建目标 `libsecure_access_fence_service.dylib.so` |

#### Ticket 操作

| 模块 | 文件 |
|------|------|
| 分发 | `services/secure_access_fence/core_service/src/ticket_operation/mod.rs` |
| 密钥管理 trait | `services/secure_access_fence/core_service/src/ticket_operation/ticket_key_manager.rs` |
| 账户模式 | `services/secure_access_fence/core_service/src/ticket_operation/account_based_ticket_key_manager.rs` |
| 本地模式 | `services/secure_access_fence/core_service/src/ticket_operation/local_based_ticket_key_manager.rs` |
| 防重放 | `services/secure_access_fence/core_service/src/ticket_operation/challenge_store.rs` |

#### 远程控制

| 模块 | 文件 |
|------|------|
| 分发 | `services/secure_access_fence/core_service/src/remote_control/mod.rs` |
| 受控设备 | `services/secure_access_fence/core_service/src/remote_control/controlled_device.rs` |
| 控制端设备 | `services/secure_access_fence/core_service/src/remote_control/controller_device.rs` |
| 授权记录 | `services/secure_access_fence/core_service/src/remote_control/grant_record.rs` |
| 远程 Challenge | `services/secure_access_fence/core_service/src/remote_control/remote_challenge_manager.rs` |
| CLI 管理 | `services/secure_access_fence/core_service/src/remote_control/cli_manager.rs` |
| 账户认证 | `services/secure_access_fence/core_service/src/remote_control/account_based_auth_manager.rs` |

### 公共模块

| 模块 | Asset 路径 | SAF 路径 |
|------|-----------|-----------|
| crate | `services/asset/common/src/lib.rs` | `services/secure_access_fence/common/src/lib.rs` |
| CallingInfo | `services/asset/common/src/calling_info.rs` | `services/secure_access_fence/common/src/calling_info.rs` |
| Counter | `services/asset/common/src/counter.rs` | `services/secure_access_fence/common/src/counter.rs` |
| TaskManager | `services/asset/common/src/task_manager.rs` | `services/secure_access_fence/common/src/task_manager.rs` |
| ProcessInfo | `services/asset/common/src/process_info.rs` | — |

### 测试入口

| 测试 | 路径 |
|------|------|
| Asset UT | `test/asset/unittest/ut_test/services/core_service/src/lib.rs` |
| Asset 操作 UT | `test/asset/unittest/ut_test/services/core_service/src/operations/operation_add.rs` |
| Asset Stub | `test/asset/unittest/ut_test/services/core_service/test_stub/ut_core_service_lib_stub.rs` |
| SAF UT | `test/secure_access_fence/unittest/ut_test/services/core_service/src/lib.rs` |
| SAF 远程控制 UT | `test/secure_access_fence/unittest/ut_test/services/core_service/src/remote_control/mod.rs` |

## 数据流

Asset Core Service 完整调用链：

```
IPC 请求 → stub.rs (IpcCode 分发)
  → operation_add/remove/update/query.rs
    → CryptoManager.find/add (获取加密实例) → Crypto.encrypt → HUKS
    → DB Operator.execute_query → SQLite (密文存储)
    → DB Key Operator.get_db_key → HUKS (数据库密钥)
  → 返回结果

SAF Core Service 完整调用链：
  IPC 请求 → stub.rs → ticket_operation 或 remote_control
    → TicketKeyManager → HUKS (Ticket 密钥验证)
    → ChallengeStore (防重放检查)
    → remote_control: ControlledDevice/ControllerDevice (设备包生成/验证)
```

Asset 和 SAF 两个 Core Service 共享相似的架构模式：IPC Stub 分发 → 操作模块处理 → 底层模块（Crypto/DB/Ticket）执行。但底层模块完全不同：Asset 走 Crypto→DB 链路，SAF 走 TicketKeyManager→ChallengeStore 链路。

两个服务的公共模块（CallingInfo、Counter、TaskManager）提供统一的调用者信息管理和生命周期计数。Common Event 监听在 SA 启动时注册，处理包卸载、用户删除等系统事件，触发相应的资产清理和密钥删除。

## 开发要点

- 操作模块（operation_*.rs）每个对应一个 IPC Code，新增操作需同步注册
- 公共事件监听在 SA 启动时注册，修改事件需同步更新 listener.rs
- DB 升级逻辑在 upgrade_operator.rs，修改表结构需补充升级路径

## 常见开发场景

- 新增一个 Asset/SAF 操作时，在此查找操作分发和对应的 operation 文件
- 修改 SA 生命周期行为时，定位 on_start/on_active/on_idle 等回调
- 排查公共事件订阅问题时，查看 common_event 和 listener 实现

## 相关主题

- [Asset 服务架构](../architecture/asset-service.md)
- [安全访问围栏架构](../architecture/secure-access-fence.md)
- [Crypto Manager 模块](./crypto-manager.md)
- [DB Operator 模块](./db-operator.md)
- [IPC 接口定义](../api/ipc-interfaces.md)
