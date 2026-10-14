# IPC 接口定义 Context

> 文档版本：v1.0
> 更新时间：2026-10-13
> 来源：`frameworks/asset/ipc/`、`frameworks/secure_access_fence/inner_api/ipc/`

## 定位

两个子系统的 IPC 通信协议定义。Asset 使用 Rust 枚举定义 IPC Code，SAF 使用 IDL 文件定义接口和类型。

## 快速路由

### 源码入口

### Asset IPC

| 文件 | 说明 |
|------|------|
| `frameworks/asset/ipc/src/lib.rs` | IpcCode 枚举、SA_ID=8100、序列化/反序列化 |
| `services/asset/core_service/src/stub.rs` | IPC Stub 实现（RemoteStub） |
| `frameworks/asset/ipc/BUILD.gn` | 构建配置 |

#### Asset IPC Code

| 枚举值 | Code | 说明 |
|--------|------|------|
| Add | FIRST_CALL_TRANSACTION | 新增 |
| Remove | +1 | 删除 |
| Update | +2 | 更新 |
| PreQuery | +3 | 预查询 |
| Query | +4 | 查询 |
| PostQuery | +5 | 后查询 |
| QuerySyncResult | +6 | 同步结果查询 |
| BatchAdd | +7 | 批量新增 |
| BatchRemove | +8 | 批量删除 |
| BatchUpdate | +9 | 批量更新 |

#### Asset IPC 序列化函数

| 函数 | 说明 |
|------|------|
| `serialize_map` / `deserialize_map` | 单个 AssetMap 序列化 |
| `serialize_maps` / `deserialize_maps` | 多个 AssetMap 序列化 |
| `serialize_sync_result` | 同步结果序列化 |
| `serialize_batch_result` | 批量结果序列化 |
| `ipc_err_handle` | IPC 错误处理 |

### SAF IPC

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

#### SAF IPC 类型（IDL）

| 类型 | 说明 |
|------|------|
| CommandInfo | 命令信息 |
| VerifyTicketInfo | 验证 Ticket 信息 |
| CliInfo | CLI 信息 |
| PermissionQuery | 权限查询 |
| RemoteInfo | 远程信息 |
| RemoteAuthPackage | 远程认证包 |
| RemoteUserAuthResults | 远程用户认证结果 |

### 调用链路

```
# Asset
Client → serialize_map → IPC → on_remote_request(stub.rs) → IpcCode 匹配 → operation_*.rs

# SAF
Client → SafAgentFence → IPC → on_remote_request(stub.rs) → IDL 方法分发 → ticket_operation / remote_control
```

### 测试入口

| 测试 | 路径 |
|------|------|
| Asset Stub UT | `test/asset/unittest/ut_test/services/core_service/test_stub/ut_core_service_lib_stub.rs` |
| SAF Stub UT | `test/secure_access_fence/unittest/ut_test/services/core_service/test_stub/ut_core_service_lib_stub.rs` |

## 开发要点

- IpcCode 枚举值必须与 stub.rs 中的 dispatch 顺序严格对应
- 新增 IPC 方法时需同步更新序列化函数和 stub 分发
- SAF 使用 IDL 文件定义接口，修改 IDL 后需重新生成 IPC 代码

## 常见开发场景

- 新增一个 Asset 操作时，需要在此处查找 IpcCode 定义和对应的序列化函数
- 排查 IPC 调用返回错误码时，定位 stub.rs 中的分发逻辑
- SAF 接口变更时，需要修改 IDL 并重新生成序列化代码

## 相关主题

- [Asset 服务架构](../architecture/asset-service.md)
- [安全访问围栏架构](../architecture/secure-access-fence.md)
- [Core Service 模块](../services/core-service.md)
- [SAF Agent Fence API](./saf-agent-fence.md)
