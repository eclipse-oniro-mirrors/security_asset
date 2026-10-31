# SAF 插件系统 Context

> 文档版本：v1.0
> 更新时间：2026-10-13
> 来源：`services/secure_access_fence/plugin/`、`interfaces/inner_kits/plugin_interface/secure_access_fence/`

## 定位

SAF 插件系统通过 `dlopen` 动态加载 SAF 扩展库，实现可扩展的事件处理，不修改核心代码即可增加新能力。

## 快速路由

### 源码入口

| 文件 | 说明 |
|------|------|
| `interfaces/inner_kits/plugin_interface/secure_access_fence/src/plugin_interface.rs` | SAF 插件接口 trait |
| `services/secure_access_fence/plugin/src/lib.rs` | saf_plugin crate 入口 |
| `services/secure_access_fence/plugin/src/saf_plugin.rs` | SAFPlugin（dlopen SAF 扩展） |
| `services/secure_access_fence/plugin/BUILD.gn` | 构建配置 |

### 插件接口 trait

| trait | 定义位置 | 说明 |
|-------|----------|------|
| SAF 插件 trait | `interfaces/inner_kits/plugin_interface/secure_access_fence/src/plugin_interface.rs` | SAF 扩展接口 |

### 符号导出

| 文件 | 说明 |
|------|------|
| `interfaces/inner_kits/plugin_interface/secure_access_fence/libsaf_plugin_interface.map` | SAF 插件接口符号映射 |

### 测试入口

| 测试 | 路径 |
|------|------|
| SAF 插件测试 | `test/secure_access_fence/unittest/` |

## 数据流

SAF 插件系统调用链路：

```
Core Service → SAFPlugin::new() → dlopen(SAF 扩展库)
  → 调用 SAF 插件 trait 方法 → 处理扩展事件
  → 返回结果给 Core Service
```

SAF 插件通过 `dlopen` 动态加载 SAF 扩展库，Core Service 在启动时尝试加载插件，加载成功后通过 SAF 插件 trait 调用扩展事件处理。插件加载失败不影响核心服务启动，仅相关扩展功能不可用。

## 开发要点

- SAF 插件通过 dlopen 动态加载，新增扩展事件需更新 SAF 插件 trait
- 插件加载失败不影响核心服务启动，但相关扩展功能不可用
- 修改 SAF 插件接口时，确认 plugin_interface.rs 的 trait 定义和 .map 符号映射

## 常见开发场景

- 新增 SAF 扩展事件类型时，在此查找 SAF 插件 trait 定义
- 排查 SAF 插件加载失败时，定位 dlopen 路径和符号导出配置
- 修改 SAF 插件接口时，确认 plugin_interface.rs 的 trait 定义和 .map 符号映射

## 相关主题

- [SAF Core Service 模块](./core-service.md)
- [SAF Rust SDK](../api/rust-sdk.md)
- [安全访问围栏架构](../architecture/service.md)
