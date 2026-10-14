# 安全访问围栏架构 Context

> 文档版本：v1.0
> 更新时间：2026-10-13
> 来源：代码仓结构分析

## 定位

安全访问围栏（Secure Access Fence，SAF）是 SA 66532（进程 `secure_access_fence`），提供基于 Ticket 的权限验证、远程控制（Controller/Controlled Device 包）、命令权限批量查询与授予。支持防重放 Challenge Store 和账户/本地两种 Ticket 密钥管理模式。

## 快速路由

### 源码入口

| 模块 | 路径 | 说明 |
|------|------|------|
| SA 入口 | `services/secure_access_fence/core_service/src/lib.rs` | `SAFAbility` 实现 `Ability` trait，静态 `.init_array` 注册 SA 66532 |
| IPC Stub | `services/secure_access_fence/core_service/src/stub.rs` | 请求分发 |
| C++ IPC | `services/secure_access_fence/core_service/src/cxx/secure_access_fence_ipc.cpp` | C++ IPC 实现 |
| C++ Service | `services/secure_access_fence/core_service/src/cxx/src/secure_access_fence_service.cpp` | C++ 服务辅助 |
| C++ 权限管理 | `services/secure_access_fence/core_service/src/cxx/src/permission_manager.cpp` | 权限管理 |
| C++ 时间包装 | `services/secure_access_fence/core_service/src/cxx/src/time_wrapper.cpp` | 时间包装 |
| FFI 桥接 | `services/secure_access_fence/core_service/src/wrapper.rs` | Rust→C++ FFI |
| 指标宏 | `services/secure_access_fence/core_service/src/metrics_macro.rs` | execute_with_metrics! 宏 |
| 远程授权状态 | `services/secure_access_fence/core_service/src/remote_grant_status.rs` | 远程授权使能/禁能 |
| 插件系统 | `services/secure_access_fence/plugin/src/saf_plugin.rs` | `dlopen` SAF 扩展 |
| 公共模块 | `services/secure_access_fence/common/src/` | CallingInfo、Counter、TaskManager |
| OS 适配 | `services/secure_access_fence/os_dependency/src/` | screen_lock、os_account、access_token、crypto_wrapper、cli_tool_wrapper |

### API 入口

| 接口 | 路径 |
|------|------|
| IDL 接口定义 | `frameworks/secure_access_fence/inner_api/ipc/ISecureAccessFence.idl` |
| IDL 类型定义 | `frameworks/secure_access_fence/inner_api/ipc/SecureAccessFenceType.idl` |
| IPC 序列化 | `frameworks/secure_access_fence/inner_api/ipc/src/` |
| Ticket IPC | `frameworks/secure_access_fence/inner_api/ipc/src/sub_modules/ticket.rs` |
| Remote Control IPC | `frameworks/secure_access_fence/inner_api/ipc/src/sub_modules/remote_control.rs` |
| 远程消息包装 | `frameworks/secure_access_fence/inner_api/ipc/src/remote_message_wrapper.rs` |
| 数据类型 | `frameworks/secure_access_fence/definition/src/lib.rs` |
| Agent Fence 客户端 | `frameworks/secure_access_fence/inner_api/agent_fence/src/saf_agent_fence.cpp` |
| 参数校验 | `frameworks/secure_access_fence/inner_api/agent_fence/src/saf_agent_params_checker.cpp` |
| JS NAPI 绑定 | `frameworks/secure_access_fence/js/napi/src/agent_fence_napi.cpp` |
| SAF 工具 | `frameworks/secure_access_fence/utils/src/`（time、json_builder） |
| SAF 日志 | `frameworks/secure_access_fence/os_dependency/log/src/lib.rs` |

### IPC 方法（IDL）

| 方法 | IPC Code | 说明 |
|------|----------|------|
| BatchGenerateTicket | 1 | 批量生成 Ticket |
| BatchVerifyTicket | 2 | 批量验证 Ticket |
| VerifyTicket | 3 | 单个验证 Ticket |
| GenerateControlledDevicePackage | 4 | 生成受控设备包 |
| VerifyControlledDevicePackage | 5 | 验证受控设备包 |
| GenerateControllerDevicePackage | 6 | 生成控制端设备包 |
| VerifyControllerDevicePackage | 7 | 验证控制端设备包 |
| GetRemoteGrantStatus | 8 | 获取远程授权状态 |
| UpdateRemoteGrantStatus | 9 | 更新远程授权状态 |
| BatchQueryCommandPermission | 500 | 批量查询命令权限 |
| RequestToolPermissions | 501 | 请求工具权限 |
| GrantToolPermissionsByUser | 502 | 用户授予工具权限 |

### Ticket 操作模块

| 模块 | 路径 | 说明 |
|------|------|------|
| 分发器 | `services/secure_access_fence/core_service/src/ticket_operation/mod.rs` | |
| 密钥管理 trait | `services/secure_access_fence/core_service/src/ticket_operation/ticket_key_manager.rs` | |
| 账户模式 | `services/secure_access_fence/core_service/src/ticket_operation/account_based_ticket_key_manager.rs` | 基于系统账号 |
| 本地模式 | `services/secure_access_fence/core_service/src/ticket_operation/local_based_ticket_key_manager.rs` | 基于本地 |
| 防重放 Challenge | `services/secure_access_fence/core_service/src/ticket_operation/challenge_store.rs` | 全局 Challenge Store |

### 远程控制模块

| 模块 | 路径 | 说明 |
|------|------|------|
| 分发器 | `services/secure_access_fence/core_service/src/remote_control/mod.rs` | |
| 受控设备 | `services/secure_access_fence/core_service/src/remote_control/controlled_device.rs` | 包生成/验证 |
| 控制端设备 | `services/secure_access_fence/core_service/src/remote_control/controller_device.rs` | 包生成/验证 |
| 授权记录 | `services/secure_access_fence/core_service/src/remote_control/grant_record.rs` | |
| 远程 Challenge | `services/secure_access_fence/core_service/src/remote_control/remote_challenge_manager.rs` | |
| CLI 管理 | `services/secure_access_fence/core_service/src/remote_control/cli_manager.rs` | |
| 账户认证管理 | `services/secure_access_fence/core_service/src/remote_control/account_based_auth_manager.rs` | |

### 进程配置

| 文件 | 说明 |
|------|------|
| `etc/init/secure_access_fence.cfg` | init 进程配置 |
| `sa_profile/66532.json` | SA 按需启动事件配置 |

### 测试入口

| 测试 | 路径 |
|------|------|
| Agent Fence Ticket 测试 | `test/secure_access_fence/agent_fence/src/saf_agent_fence_gen_and_ver_ticket_test.cpp` |
| SAF Core Service UT | `test/secure_access_fence/unittest/ut_test/services/core_service/src/lib.rs` |

## 数据流

```
应用 → SafAgentFence (C++客户端) → IPC → SAFService (SA 66532)
  → ticket_operation: TicketKeyManager → ChallengeStore → HUKS (密钥验证)
  → remote_control: ControlledDevice/ControllerDevice → GrantRecord → 远程授权
  → 返回验证结果 → 应用执行受控操作
```

SAF 服务内部包含两个核心子系统：Ticket 操作和远程控制。Ticket 操作通过 Challenge Store 实现防重放，密钥管理支持账户模式和本地模式两种策略。远程控制模块处理受控设备和控制端设备间的权限授予包生成与验证，支持分布式场景下的跨设备权限管理。

Ticket 验证流程：应用请求验证 → Challenge Store 检查 Challenge 唯一性 → TicketKeyManager 调用 HUKS 校验密钥 → 返回验证结果。远程控制流程：控制端生成设备包 → 受控端验证设备包 → GrantRecord 记录授权 → 后续操作基于授权记录放行。

## 开发要点

- Ticket 密钥管理模式（账户/本地）通过 trait 切换，新增模式需实现 TicketKeyManager trait
- Challenge Store 是全局防重放机制，修改需注意线程安全
- 远程控制包格式变更需同时更新 controlled_device.rs 和 controller_device.rs

## 常见开发场景

- 新增 Ticket 验证逻辑时，在此查找 TicketKeyManager trait 和两种实现模式
- 修改远程控制包格式时，定位受控端和控制端的包生成/验证代码
- 排查防重放问题时，查看 challenge_store 的 Challenge 生命周期

## 相关主题

- [整体架构概览](./overview.md)
- [SAF Agent Fence API](../api/saf-agent-fence.md)
- [IPC 接口定义](../api/ipc-interfaces.md)
- [Core Service 模块](../services/core-service.md)
