# SAF OS Dependency 模块 Context

> 文档版本：v1.0
> 更新时间：2026-10-13
> 来源：`services/secure_access_fence/os_dependency/`、`frameworks/secure_access_fence/os_dependency/`

## 定位

SAF OS Dependency 层封装 OpenHarmony 系统能力调用，包括权限校验、包管理、锁屏、账号、SAF 自有加密操作、CLI 工具集成和日志。服务层提供 C++ wrapper 供 C++ 代码调用，框架层提供 Rust crate 供 Rust 代码直接调用。

## 快速路由

### 源码入口

### SAF 服务层 OS Dependency（C++）

| 文件 | 头文件 | 说明 |
|------|--------|------|
| `services/secure_access_fence/os_dependency/src/system_ability_wrapper.cpp` | `inc/system_ability_wrapper.h` | SA 管理 |
| `services/secure_access_fence/os_dependency/src/system_event_wrapper.cpp` | `inc/system_event_wrapper.h` | 公共事件 |
| `services/secure_access_fence/os_dependency/src/screen_lock_wrapper.cpp` | `inc/screen_lock_wrapper.h` | 锁屏状态 |
| `services/secure_access_fence/os_dependency/src/os_account_wrapper.cpp` | `inc/os_account_wrapper.h` | 账号信息 |
| `services/secure_access_fence/os_dependency/src/crypto_wrapper.cpp` | `inc/crypto_wrapper.h` | 加密操作 |
| `services/secure_access_fence/os_dependency/src/cli_tool_wrapper.cpp` | `inc/cli_tool_wrapper.h` | CLI 工具集成 |
| `services/secure_access_fence/os_dependency/src/access_token_wrapper.cpp` | `inc/access_token_wrapper.h` | 权限校验 |
| `services/secure_access_fence/os_dependency/BUILD.gn` | — | 构建配置 |

### SAF 框架层 OS Dependency

| 模块 | 路径 | 说明 |
|------|------|------|
| 日志 | `frameworks/secure_access_fence/os_dependency/log/src/lib.rs` | SAF 日志 |
| 日志头文件 | `frameworks/secure_access_fence/os_dependency/log/inc/saf_log.h` | SAF 日志 C 头文件 |

### 测试入口

| 测试 | 路径 |
|------|------|
| SAF OS Dependency 测试 | `test/secure_access_fence/unittest/` |

## 数据流

OS Dependency wrapper 在 SAF 服务调用链中的位置：

```
ticket_operation / remote_control → 需要系统能力时调用 C++ wrapper
  → access_token_wrapper → AccessToken 服务 (权限校验)
  → screen_lock_wrapper → ScreenLock Manager (锁屏状态)
  → os_account_wrapper → OS Account (用户信息)
  → crypto_wrapper → 加密操作 (SAF 自有加密)
  → cli_tool_wrapper → CLI 工具集成
```

SAF 侧包含 crypto_wrapper（SAF 自有加密操作）和 cli_tool_wrapper（CLI 工具集成）。所有 wrapper 都通过 C++ 实现，通过 `rust_cxx` 桥接被 Rust 侧调用。wrapper 层负责错误码转换和超时处理，将系统服务的错误映射为 SAF 的内部错误码。

## 开发要点

- 权限校验统一走 access_token_wrapper，不要在业务代码中直接调用 AccessTokenKit
- 锁屏状态查询通过 screen_lock_wrapper 封装，注意设备锁屏对 Ticket 验证的影响
- SAF 自有加密操作通过 crypto_wrapper 封装，密钥操作仍走 HUKS

## 常见开发场景

- 新增系统能力包装时，在此查找现有 SAF wrapper 的实现模式和头文件
- 排查权限校验失败时，定位 access_token_wrapper 的 CheckPermission 逻辑
- 修改锁屏相关逻辑时，查看 screen_lock_wrapper 的状态查询接口

## 相关主题

- [SAF Core Service 模块](./core-service.md)
- [SAF Framework OS Dependency](../frameworks/os-dependency.md)
- [安全访问围栏架构](../architecture/service.md)
