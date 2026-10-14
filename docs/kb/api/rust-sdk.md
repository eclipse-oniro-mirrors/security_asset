# Rust SDK Context

> 文档版本：v1.0
> 更新时间：2026-10-13
> 来源：`interfaces/inner_kits/rs/asset/`

## 定位

面向系统服务 Rust 侧调用的 SDK，通过 IPC 与 Asset Service（SA 8100）通信。按 `asset_build_mode` 配置选择 full 实现或 empty 桩。

## 快速路由

### 源码入口

| 文件 | 说明 |
|------|------|
| `interfaces/inner_kits/rs/asset/src/lib.rs` | 入口，re-export full.rs 或 empty.rs |
| `interfaces/inner_kits/rs/asset/src/full.rs` | 完整实现：IPC 调用 SA 8100 |
| `interfaces/inner_kits/rs/asset/src/empty.rs` | 空桩（非 full 构建） |
| `interfaces/inner_kits/rs/asset/src/timeout_feature.c` | 超时特性 C 辅助 |
| `interfaces/inner_kits/rs/asset/client_dependency/inc/img_dependency_wrapper.h` | IMG 依赖包装头文件 |
| `interfaces/inner_kits/rs/asset/client_dependency/src/img_dependency_wrapper.cpp` | IMG 依赖包装实现 |
| `interfaces/inner_kits/rs/asset/BUILD.gn` | 构建目标 `libasset_sdk` |
| `interfaces/inner_kits/rs/asset/libasset_sdk.map` | 符号导出映射 |

### API 入口

full.rs 提供以下函数（通过 IPC 发送到 SA 8100）：

| 函数 | 对应 IpcCode |
|------|--------------|
| add | Add |
| query | Query |
| update | Update |
| remove | Remove |
| pre_query | PreQuery |
| post_query | PostQuery |
| query_sync_result | QuerySyncResult |
| batch_add | BatchAdd |
| batch_remove | BatchRemove |
| batch_update | BatchUpdate |

### 构建配置

| 配置 | 说明 |
|------|------|
| `asset_build_mode = "full"` | 使用 full.rs，正常 IPC 调用 |
| 其他值 | 使用 empty.rs，返回未实现错误 |

### SAF Rust SDK

| 文件 | 说明 |
|------|------|
| `interfaces/inner_kits/rs/secure_access_fence/src/lib.rs` | SAF Rust SDK |
| `interfaces/inner_kits/rs/secure_access_fence/BUILD.gn` | 构建目标 `libsaf_sdk` |
| `interfaces/inner_kits/rs/secure_access_fence/libsaf_sdk.map` | 符号导出映射 |

### 插件接口

| 文件 | 说明 |
|------|------|
| `interfaces/inner_kits/plugin_interface/asset/src/plugin_interface.rs` | IAssetPlugin、IAssetPluginCtx trait、EventType、ExtDbMap |
| `interfaces/inner_kits/plugin_interface/secure_access_fence/src/plugin_interface.rs` | SAF 插件接口 trait |

### 测试入口

| 测试 | 路径 |
|------|------|
| Rust SDK 正向测试 | `test/asset/unittest/inner_kits_rust/src/add.rs` 等 |
| Rust SDK 异常测试 | `test/asset/unittest/inner_kits_rust/src/add_abnormal.rs` 等 |

## 数据流

```
应用 Rust 代码 → asset_sdk::add/query/update/remove 等
  → IPC serialize_map (asset_ipc crate) → send IPC to SA 8100
  → AssetService::on_remote_request (stub.rs) → operation_*.rs
  → 返回结果通过 IPC 反序列化回到 SDK 调用方
```

Rust SDK 是应用与 Asset Service 之间的 IPC 客户端。full.rs 包含完整 IPC 调用实现，empty.rs 在非 full 构建模式下返回 `Unsupported` 错误。IMG 依赖包装用于特定平台的能力适配。

## 开发要点

- full.rs 和 empty.rs 通过 asset_build_mode 编译配置切换，非 full 模式返回未实现错误
- 新增 SDK 函数需同步更新 IPC 序列化（asset_ipc crate）和 stub.rs 分发
- IMG 依赖包装（img_dependency_wrapper）用于特定平台适配，移植到新平台需补充
- Rust SDK 通过 IPC 调用 SA 8100，调用前需确保 SA 已启动

## 常见开发场景

- Rust 系统服务通过 inner_kits 调用 Asset 存储能力
- 插件开发者通过 plugin_interface trait 实现扩展功能
- 构建系统通过 asset_build_mode 控制是否编入完整实现

## 相关主题

- [公共 NDK API](./ndk-api.md)
- [C System API](./system-api.md)
- [IPC 接口定义](./ipc-interfaces.md)
- [插件系统](../services/plugin-system.md)
