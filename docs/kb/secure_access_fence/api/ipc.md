# SAF IPC 接口定义 Context

> 文档版本：v1.0
> 更新时间：2026-10-13
> 来源：`frameworks/secure_access_fence/inner_api/ipc/`

## 定位

安全访问围栏（SAF）的 IPC 通信协议定义，使用 IDL 文件定义接口和类型，通过 codegen 生成序列化代码，并包含远程消息包装和子模块序列化。

## 快速路由

### 源码入口

| 文件 | 说明 |
|------|------|
| `frameworks/secure_access_fence/inner_api/ipc/ISecureAccessFence.idl` | 接口定义（12 方法） |
| `frameworks/secure_access_fence/inner_api/ipc/SecureAccessFenceType.idl` | 类型定义（15+ 结构体/枚举） |
| `frameworks/secure_access_fence/inner_api/ipc/src/lib.rs` | SA_ID 常量 |
| `frameworks/secure_access_fence/inner_api/ipc/src/remote_message_wrapper.rs` | 远程消息包装 |
| `frameworks/secure_access_fence/inner_api/ipc/src/sub_modules/ticket.rs` | Ticket IPC 序列化 |
| `frameworks/secure_access_fence/inner_api/ipc/src/sub_modules/remote_control.rs` | 远程控制 IPC 序列化 |
| `services/secure_access_fence/core_service/src/stub.rs` | IPC Stub |
| `services/secure_access_fence/core_service/src/cxx/secure_access_fence_ipc.cpp` | C++ IPC 实现 |
| `frameworks/secure_access_fence/inner_api/ipc/BUILD.gn` | 构建配置 |

### SAF IPC 类型（IDL）

| 类型 | 说明 |
|------|------|
| CommandInfo | 命令信息 |
| VerifyTicketInfo | 验证 Ticket 信息 |
| CliInfo | CLI 信息 |
| PermissionQuery | 权限查询 |
| RemoteInfo | 远程信息 |
| RemoteAuthPackage | 远程认证包 |
| RemoteUserAuthResults | 远程用户认证结果 |

### SAF IPC 方法（IDL）

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

### 调用链路

```
Client → SafAgentFence → IPC → on_remote_request(stub.rs) → IDL 方法分发 → ticket_operation / remote_control
```

### 测试入口

| 测试 | 路径 |
|------|------|
| SAF Stub UT | `test/secure_access_fence/unittest/ut_test/services/core_service/test_stub/ut_core_service_lib_stub.rs` |

## 开发要点

- SAF 使用 IDL 文件定义接口，修改 IDL 后需重新生成 IPC 代码
- 新增 IPC 方法时需同步更新 IDL、序列化代码（sub_modules）和 stub 分发
- `remote_message_wrapper.rs` 统一封装远程控制消息的序列化，修改包格式需同步更新

## 常见开发场景

- SAF 接口变更时，需要修改 IDL 并重新生成序列化代码
- 排查 IPC 调用返回错误码时，定位 stub.rs 中的分发逻辑
- 新增远程控制消息字段时，确认 remote_message_wrapper 和 sub_modules 的序列化实现

## 相关主题

- [安全访问围栏架构](../architecture/service.md)
- [SAF Agent Fence API](./agent-fence-api.md)
- [SAF Core Service 模块](../services/core-service.md)
- [SAF IPC 序列化模块](../frameworks/ipc.md)
