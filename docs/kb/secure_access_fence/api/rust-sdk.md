# SAF Rust SDK Context

> 文档版本：v1.0
> 更新时间：2026-10-13
> 来源：`interfaces/inner_kits/rs/secure_access_fence/`

## 定位

面向系统服务 Rust 侧调用的 SAF SDK，通过 IPC 与安全访问围栏服务（SA 66532）通信，封装 Ticket 生成/验证、远程控制和命令权限查询等能力的 Rust 客户端调用。

## 快速路由

### 源码入口

| 文件 | 说明 |
|------|------|
| `interfaces/inner_kits/rs/secure_access_fence/src/lib.rs` | SAF Rust SDK 入口 |
| `interfaces/inner_kits/rs/secure_access_fence/BUILD.gn` | 构建目标 `libsaf_sdk` |
| `interfaces/inner_kits/rs/secure_access_fence/libsaf_sdk.map` | 符号导出映射 |

### 测试入口

| 测试 | 路径 |
|------|------|
| SAF Rust SDK 测试 | `test/secure_access_fence/unittest/` |

## 数据流

```
应用 Rust 代码 → saf_sdk 调用
  → IPC 序列化 (saf_ipc crate) → send IPC to SA 66532
  → SAFService::on_remote_request (stub.rs) → ticket_operation / remote_control
  → 返回结果通过 IPC 反序列化回到 SDK 调用方
```

SAF Rust SDK 是应用与 SAF 服务（SA 66532）之间的 IPC 客户端，封装与 `SafAgentFence`（C++ 客户端）对应的 Rust 侧调用能力。IPC 序列化复用 `saf_ipc` crate（IDL codegen 生成）。

## 开发要点

- SAF Rust SDK 通过 IPC 调用 SA 66532，调用前需确保 SA 已启动
- 新增 SDK 函数需同步更新 SAF IDL（`ISecureAccessFence.idl`）和 stub.rs 分发
- SAF 插件接口 trait 定义在 `interfaces/inner_kits/plugin_interface/secure_access_fence/src/plugin_interface.rs`（参见 [插件系统](../services/plugin-system.md)）

## 常见开发场景

- Rust 系统服务通过 inner_kits 调用 SAF Ticket 验证和远程控制能力
- 插件开发者通过 SAF plugin_interface trait 实现扩展功能
- 构建系统通过 BUILD.gn 控制 libsaf_sdk 是否编入

## 相关主题

- [SAF Agent Fence API](./agent-fence-api.md)
- [IPC 接口定义](./ipc.md)
- [插件系统](../services/plugin-system.md)
- [安全访问围栏架构](../architecture/service.md)
