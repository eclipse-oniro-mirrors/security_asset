# SAF Core Service 模块 Context

> 文档版本：v1.0
> 更新时间：2026-10-13
> 来源：`services/secure_access_fence/core_service/`

## 定位

SAF Core Service 是 SA 66532 的核心实现层，包含 SA 入口、IPC Stub、C++ FFI 桥接、Ticket 操作（密钥管理 + 防重放）、远程控制（设备包生成/验证 + 授权记录）、指标宏和远程授权状态管理。

## 快速路由

### 源码入口

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

### Ticket 操作

| 模块 | 文件 |
|------|------|
| 分发 | `services/secure_access_fence/core_service/src/ticket_operation/mod.rs` |
| 密钥管理 trait | `services/secure_access_fence/core_service/src/ticket_operation/ticket_key_manager.rs` |
| 账户模式 | `services/secure_access_fence/core_service/src/ticket_operation/account_based_ticket_key_manager.rs` |
| 本地模式 | `services/secure_access_fence/core_service/src/ticket_operation/local_based_ticket_key_manager.rs` |
| 防重放 | `services/secure_access_fence/core_service/src/ticket_operation/challenge_store.rs` |

### 远程控制

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

| 模块 | 路径 |
|------|------|
| crate | `services/secure_access_fence/common/src/lib.rs` |
| CallingInfo | `services/secure_access_fence/common/src/calling_info.rs` |
| Counter | `services/secure_access_fence/common/src/counter.rs` |
| TaskManager | `services/secure_access_fence/common/src/task_manager.rs` |

CallingInfo、Counter、TaskManager 与 Asset 共享相似实现。

### 测试入口

| 测试 | 路径 |
|------|------|
| SAF UT | `test/secure_access_fence/unittest/ut_test/services/core_service/src/lib.rs` |
| SAF 远程控制 UT | `test/secure_access_fence/unittest/ut_test/services/core_service/src/remote_control/mod.rs` |
| SAF Stub | `test/secure_access_fence/unittest/ut_test/services/core_service/test_stub/ut_core_service_lib_stub.rs` |

## 数据流

SAF Core Service 完整调用链：

```
IPC 请求 → stub.rs → ticket_operation 或 remote_control
  → TicketKeyManager → HUKS (Ticket 密钥验证)
  → ChallengeStore (防重放检查)
  → remote_control: ControlledDevice/ControllerDevice (设备包生成/验证)
```

SAF Core Service 采用 IPC Stub 分发 → ticket_operation/remote_control 模块处理 → 底层模块（TicketKeyManager→ChallengeStore、ControlledDevice/ControllerDevice）执行的架构模式。C++ FFI 桥接（`wrapper.rs`）用于 Rust 与 C++ 侧（IPC、权限管理、时间包装）的互操作，`metrics_macro.rs` 提供统一的性能指标打点。公共模块（CallingInfo、Counter、TaskManager）提供统一的调用者信息管理和生命周期计数。

## 开发要点

- Ticket 密钥管理模式（账户/本地）通过 trait 切换，新增模式需实现 TicketKeyManager trait
- Challenge Store 是全局防重放机制，修改需注意线程安全
- 远程控制包格式变更需同时更新 controlled_device.rs 和 controller_device.rs
- C++ FFI 桥接在 wrapper.rs 中声明，新增 C++ 调用需同步更新 cxx 桥接定义

## 常见开发场景

- 新增一个 SAF 操作时，在此查找 ticket_operation/remote_control 分发和对应文件
- 修改 SA 生命周期行为时，定位 SAFAbility 的 on_start 等回调
- 排查 Ticket 验证或远程控制问题时，查看 ticket_operation/remote_control 实现

## 相关主题

- [安全访问围栏架构](../architecture/service.md)
- [IPC 接口定义](../api/ipc.md)
- [SAF Agent Fence API](../api/agent-fence-api.md)
- [错误码参考](../error-codes.md)
