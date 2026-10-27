# SAF Framework OS Dependency 模块 Context

> 文档版本：v1.0
> 更新时间：2026-10-13
> 来源：`frameworks/secure_access_fence/os_dependency/`

## 定位

SAF 框架层 OS Dependency 提供日志能力，与服务层 OS Dependency（C++ 系统能力包装）互补。详见 [OS Dependency 模块](../services/os-dependency.md)。

## 快速路由

### 源码入口

### SAF 框架层 OS Dependency

| 模块 | crate 名 | 路径 | 说明 |
|------|----------|------|------|
| 日志 | `saf_log` | `frameworks/secure_access_fence/os_dependency/log/src/lib.rs` | SAF 日志 |

### SAF 工具模块

| 文件 | 说明 |
|------|------|
| `frameworks/secure_access_fence/utils/src/lib.rs` | 入口（crate: saf_utils） |
| `frameworks/secure_access_fence/utils/src/time.rs` | 时间工具 |
| `frameworks/secure_access_fence/utils/src/json_builder.rs` | JSON 工具（get_compact_json_value） |

### 测试入口

| 测试 | 路径 |
|------|------|
| SAF Framework OS Dependency 测试 | `test/secure_access_fence/unittest/` |

## 数据流

框架层 OS Dependency 在 SAF 服务调用链中的位置：

```
ticket_operation / remote_control → 需要日志时调用 saf_log crate → hilog
所有 SAF 模块 → 需要时间/JSON 工具时调用 saf_utils crate
```

SAF 框架层 OS Dependency 与服务层 OS Dependency 互补：框架层提供 Rust crate 供 Rust 代码直接调用，服务层提供 C++ wrapper 供 C++ 代码调用。两者都封装了相同的系统能力（日志等），但调用路径不同。

## 开发要点

- SAF 日志统一通过 saf_log crate，不要在业务代码中直接调用 hilog
- 时间工具和 JSON 工具通过 saf_utils crate 提供，避免重复实现

## 常见开发场景

- 需要 SAF 日志输出时，定位 saf_log crate 的接口
- 需要时间或 JSON 处理时，使用 saf_utils crate 的 time.rs / json_builder.rs

## 相关主题

- [OS Dependency 模块（服务层）](../services/os-dependency.md)
- [安全访问围栏架构](../architecture/service.md)
