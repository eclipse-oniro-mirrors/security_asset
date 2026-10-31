# SAF IPC 序列化模块 Context

> 文档版本：v1.0
> 更新时间：2026-10-13
> 来源：`frameworks/secure_access_fence/inner_api/ipc/`

## 定位

SAF IPC 模块负责 SAF DataType（VerifyTicketInfo、RemoteAuthPackage 等）的 IPC 序列化/反序列化，以及 IDL 接口和远程消息包装。详见 [IPC 接口定义](../api/ipc.md)。

## 快速路由

### 源码入口

| 文件 | 说明 |
|------|------|
| `frameworks/secure_access_fence/inner_api/ipc/ISecureAccessFence.idl` | IDL 接口定义 |
| `frameworks/secure_access_fence/inner_api/ipc/SecureAccessFenceType.idl` | IDL 类型定义 |
| `frameworks/secure_access_fence/inner_api/ipc/src/lib.rs` | SA_ID 常量 |
| `frameworks/secure_access_fence/inner_api/ipc/src/remote_message_wrapper.rs` | 远程消息包装 |
| `frameworks/secure_access_fence/inner_api/ipc/src/sub_modules/mod.rs` | 子模块入口 |
| `frameworks/secure_access_fence/inner_api/ipc/src/sub_modules/ticket.rs` | Ticket 序列化 |
| `frameworks/secure_access_fence/inner_api/ipc/src/sub_modules/remote_control.rs` | 远程控制序列化 |
| `frameworks/secure_access_fence/inner_api/ipc/BUILD.gn` | 构建配置（crate: saf_ipc） |

### 测试入口

| 测试 | 路径 |
|------|------|
| SAF Stub UT | `test/secure_access_fence/unittest/ut_test/services/core_service/test_stub/ut_core_service_lib_stub.rs` |

## 数据流

IPC 序列化模块在 SAF 调用链中的位置：

```
调用方构造 SAF 类型 → saf_ipc 序列化 → IPC 传输
  → SA stub.rs → saf_ipc 反序列化 → 还原为 SAF 类型
  → ticket_operation / remote_control 处理 → 结果序列化返回
```

SAF IPC 使用 IDL 文件定义接口和类型，通过 codegen 生成序列化代码。`remote_message_wrapper.rs` 统一封装远程控制消息的序列化。

## 开发要点

- SAF 使用 IDL 文件定义接口，修改 IDL 后需重新生成 IPC 代码
- 新增 IPC 方法时需同步更新 IDL、序列化代码（sub_modules）和 stub 分发
- remote_message_wrapper 统一封装远程控制消息序列化，修改包格式需同步更新

## 常见开发场景

- 新增 SAF IPC 序列化函数时，在此查找 sub_modules 现有的序列化模式
- 排查 SAF IPC 通信问题时，定位 SA_ID 常量和 IDL 入口
- 修改 SAF IDL 类型定义后，确认对应的 Rust 序列化实现

## 相关主题

- [IPC 接口定义](../api/ipc.md)
- [Definition 模块](./definition.md)
- [安全访问围栏架构](../architecture/service.md)
