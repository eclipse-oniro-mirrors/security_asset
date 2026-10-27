# Asset 插件系统 Context

> 文档版本：v1.0
> 更新时间：2026-10-13
> 来源：`services/asset/plugin/`、`interfaces/inner_kits/plugin_interface/asset/`

## 定位

Asset 插件系统通过 `dlopen` 动态加载扩展库（`libasset_ext_ffi.z.so`），实现可扩展的克隆、同步、升级等扩展事件处理，不修改核心代码即可增加新能力。

## 快速路由

### 源码入口

| 文件 | 说明 |
|------|------|
| `interfaces/inner_kits/plugin_interface/asset/src/plugin_interface.rs` | IAssetPlugin、IAssetPluginCtx trait、EventType、ExtDbMap |
| `services/asset/plugin/src/lib.rs` | asset_plugin crate 入口 |
| `services/asset/plugin/src/asset_plugin.rs` | AssetPlugin（dlopen libasset_ext_ffi.z.so） |
| `services/asset/plugin/BUILD.gn` | 构建目标 `libasset_plugin` |

### 插件接口 trait

| trait | 定义位置 | 说明 |
|-------|----------|------|
| `IAssetPlugin` | `plugin_interface.rs` | Asset 扩展插件接口（clone/sync/upgrade 事件） |
| `IAssetPluginCtx` | `plugin_interface.rs` | Asset 插件上下文接口 |

### EventType

Asset 插件支持的扩展事件类型定义在 `plugin_interface.rs` 中，包括 clone（克隆）、sync（同步）、upgrade（升级）等。

### ExtDbMap

`ExtDbMap` 类型定义在 `plugin_interface.rs` 中，用于插件扩展数据库操作映射。

### 符号导出

| 文件 | 说明 |
|------|------|
| `interfaces/inner_kits/plugin_interface/asset/libasset_plugin_interface.map` | Asset 插件接口符号映射 |

### 测试入口

| 测试 | 路径 |
|------|------|
| 插件间接测试（module_test） | `test/asset/unittest/module_test/src/asset_system_api_test.cpp` |

## 数据流

Asset 插件系统调用链路：

```
Core Service → AssetPlugin::new() → dlopen("libasset_ext_ffi.z.so")
  → 调用 IAssetPlugin trait 方法 → 处理扩展事件
  → 返回结果给 Core Service
```

Asset 插件通过 `dlopen` 动态加载 `libasset_ext_ffi.z.so`，Core Service 在启动时尝试加载插件，加载成功后通过 `IAssetPlugin` trait 调用扩展事件处理。插件加载失败不影响核心服务启动，仅相关扩展功能不可用。

## 开发要点

- 插件通过 dlopen 动态加载，新增扩展事件需更新 IAssetPlugin trait
- 插件库路径硬编码为 libasset_ext_ffi.z.so，修改路径需同步更新 BUILD.gn
- 插件加载失败不影响核心服务启动，但相关扩展功能不可用

## 常见开发场景

- 新增扩展事件类型时，在此查找 IAssetPlugin trait 和 EventType 定义
- 排查插件加载失败时，定位 dlopen 路径和符号导出配置
- 修改插件接口时，确认 plugin_interface.rs 的 trait 定义和 .map 符号映射

## 相关主题

- [Core Service 模块](./core-service.md)
- [Rust SDK](../api/rust-sdk.md)
- [Asset 服务架构](../architecture/service.md)
