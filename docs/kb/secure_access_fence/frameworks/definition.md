# SAF Definition 模块 Context

> 文档版本：v1.0
> 更新时间：2026-10-13
> 来源：`frameworks/secure_access_fence/definition/`

## 定位

SAF Definition 模块定义 SAF 子系统的核心数据类型：SAF Tag/DataType、Ticket 验证信息、CLI 信息、权限查询、远程认证包、远程用户认证结果、远程信息、错误码和辅助宏。是所有其他 SAF 模块共享的数据基础。

## 快速路由

### 源码入口

| 文件 | 说明 |
|------|------|
| `frameworks/secure_access_fence/definition/src/lib.rs` | SAF Tag/DataType、VerifyTicketInfo、CliInfo、PermissionQuery、RemoteAuthPackage、RemoteUserAuthResults、RemoteInfo、ErrCode |
| `frameworks/secure_access_fence/definition/src/macros.rs` | SAF 宏 |
| `frameworks/secure_access_fence/definition/src/macros_lib.rs` | SAF 日志/错误宏 |
| `frameworks/secure_access_fence/definition/src/extension.rs` | SAF 扩展类型 |
| `frameworks/secure_access_fence/definition/BUILD.gn` | 构建配置 |

### 关键类型

| 类型 | 说明 |
|------|------|
| `VerifyTicketInfo` | Ticket 验证信息 |
| `CliInfo` | CLI 信息 |
| `PermissionQuery` | 权限查询 |
| `RemoteAuthPackage` | 远程认证包 |
| `RemoteUserAuthResults` | 远程用户认证结果 |
| `RemoteInfo` | 远程信息 |

### 测试入口

| 测试 | 路径 |
|------|------|
| SAF Definition 测试 | `test/secure_access_fence/unittest/` |

## 数据流

SAF Definition 模块定义的类型在整个 IPC 链路中流转：

```
调用方构造 SAF DataType（如 VerifyTicketInfo、PermissionQuery）
  → saf_ipc 序列化为 IPC MessageParcel
  → SA stub.rs 反序列化回 SAF 类型
  → ticket_operation / remote_control 读取字段执行业务逻辑
  → 结果构造为 SAF 类型 → 序列化返回
```

SAF Tag/DataType 与 IDL（`SecureAccessFenceType.idl`）定义对应，新增类型需同步更新 IDL 和 Rust `lib.rs`（Definition）。ErrCode 枚举与 C 侧 `SafResultCode` 对应，用于 IPC 错误传递。

## 开发要点

- SAF DataType 新增类型时必须同步更新 IDL（SecureAccessFenceType.idl）和 Rust lib.rs
- SAF 错误码新增后需同步更新 saf_result_defs.h 和 Rust ErrCode 枚举
- RemoteAuthPackage/RemoteInfo 等远程控制类型变更需考虑 IPC 序列化兼容性

## 常见开发场景

- 新增 SAF 数据类型时，在此查找 lib.rs 定义和 IDL 类型映射
- 添加 SAF 错误码时，定位 ErrCode 枚举和对应的宏
- 修改远程认证包结构时，评估对 IPC 序列化和上下游模块的影响

## 相关主题

- [IPC 接口定义](../api/ipc.md)
- [IPC 序列化模块](./ipc.md)
- [安全访问围栏架构](../architecture/service.md)
