# OS Dependency 模块 Context

> 文档版本：v1.0
> 更新时间：2026-10-13
> 来源：`services/asset/os_dependency/`、`services/secure_access_fence/os_dependency/`、`frameworks/asset/os_dependency/`

## 定位

OS Dependency 层封装 OpenHarmony 系统能力调用，包括权限校验、包管理、锁屏、账号、内存管理、文件操作、数据共享、日志、安全内存和 OpenSSL 包装。

## 快速路由

### 源码入口

### Asset 服务层 OS Dependency（C++）

| 文件 | 头文件 | 说明 |
|------|--------|------|
| `services/asset/os_dependency/src/system_ability_wrapper.cpp` | `inc/system_ability_wrapper.h` | SystemAbility 管理器交互 |
| `services/asset/os_dependency/src/system_event_wrapper.cpp` | `inc/system_event_wrapper.h` | 公共事件订阅 |
| `services/asset/os_dependency/src/screen_lock_wrapper.cpp` | `inc/screen_lock_wrapper.h` | 锁屏状态查询 |
| `services/asset/os_dependency/src/os_account_wrapper.cpp` | `inc/os_account_wrapper.h` | 系统账号信息 |
| `services/asset/os_dependency/src/memory_manager_wrapper.cpp` | `inc/memory_manager_wrapper.h` | 内存管理（NotifyStatus、SetCritical、CheckMemoryMgr） |
| `services/asset/os_dependency/src/file_operator_wrapper.cpp` | `inc/file_operator_wrapper.h` | 文件操作（GetCeUpgradeInfo、GetCallingHapGroups） |
| `services/asset/os_dependency/src/data_share_wrapper.cpp` | `inc/data_share_wrapper.h` | DataShare（StoreKeyValue、QueryValue、IsDeviceLocked） |
| `services/asset/os_dependency/src/bms_wrapper.cpp` | `inc/bms_wrapper.h` | Bundle Manager 服务 |
| `services/asset/os_dependency/src/access_token_wrapper.cpp` | `inc/access_token_wrapper.h` | 权限校验（CheckPermission、CheckIsSystemHap） |
| `services/asset/os_dependency/BUILD.gn` | — | 构建配置 |

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

### Asset 框架层 OS Dependency（Rust + C）

| 模块 | 路径 | 语言 | 说明 |
|------|------|------|------|
| 文件操作 | `frameworks/asset/os_dependency/file/src/` | Rust | DE/CE 目录操作 |
| 日志 | `frameworks/asset/os_dependency/log/src/lib.rs` | Rust | 日志 |
| 日志头文件 | `frameworks/asset/os_dependency/log/inc/asset_log.h` | C | 日志 C 头文件 |
| 安全内存 | `frameworks/asset/os_dependency/memory/src/asset_mem.c` | C | 安全内存 |
| 安全内存头文件 | `frameworks/asset/os_dependency/memory/inc/asset_mem.h` | C | 安全内存 C 头文件 |
| OpenSSL 包装 | `frameworks/asset/os_dependency/openssl/src/openssl_wrapper.c` | C | OpenSSL 加密包装 |
| OpenSSL 头文件 | `frameworks/asset/os_dependency/openssl/inc/openssl_wrapper.h` | C | OpenSSL C 头文件 |

### 文件操作子模块

| 文件 | 说明 |
|------|------|
| `frameworks/asset/os_dependency/file/src/lib.rs` | 入口 |
| `frameworks/asset/os_dependency/file/src/common.rs` | DE_ROOT_PATH 等路径常量 |
| `frameworks/asset/os_dependency/file/src/de_operator.rs` | DE（Device Encrypted）目录操作 |
| `frameworks/asset/os_dependency/file/src/ce_operator.rs` | CE（Credential Encrypted）目录操作 |

### SAF 框架层 OS Dependency

| 模块 | 路径 | 说明 |
|------|------|------|
| 日志 | `frameworks/secure_access_fence/os_dependency/log/src/lib.rs` | SAF 日志 |
| 日志头文件 | `frameworks/secure_access_fence/os_dependency/log/inc/saf_log.h` | SAF 日志 C 头文件 |

### 测试入口

| 测试 | 路径 |
|------|------|
| BMS 包装测试 | `test/asset/unittest/module_test/src/bms_wrapper_test.cpp` |
| AccessToken 测试 | `test/asset/unittest/module_test/src/access_token_wrapper_test.cpp` |
| OS Account 测试 | `test/asset/unittest/module_test/src/os_account_wrapper_test.cpp` |
| SA 包装测试 | `test/asset/unittest/module_test/src/system_ability_wrapper_test.cpp` |
| 事件包装测试 | `test/asset/unittest/module_test/src/system_event_wrapper_test.cpp` |
| 内存管理测试 | `test/asset/unittest/module_test/src/memory_manager_wrapper_test.cpp` |
| 内存包装测试 | `test/asset/unittest/module_test/src/asset_mem_wrapper_test.cpp` |

## 数据流

OS Dependency wrapper 在服务调用链中的位置：

```
operation_*.rs / core_service.rs → 需要系统能力时调用 C++ wrapper
  → access_token_wrapper → AccessToken 服务 (权限校验)
  → bms_wrapper → Bundle Manager (应用信息查询)
  → screen_lock_wrapper → ScreenLock Manager (锁屏状态)
  → os_account_wrapper → OS Account (用户信息)
  → memory_manager_wrapper → MemMgr (内存状态通知)
  → data_share_wrapper → DataShare (键值存储/设备锁状态)
  → file_operator_wrapper → 文件系统 (CE/DE 目录)
```

Asset 和 SAF 各有一套独立的 OS Dependency wrapper，接口相似但实现不同。Asset 侧额外包含 data_share_wrapper（用于设备锁状态查询和键值存储）和 file_operator_wrapper（用于 CE 升级信息）。SAF 侧额外包含 crypto_wrapper（SAF 自有加密操作）和 cli_tool_wrapper（CLI 工具集成）。

所有 wrapper 都通过 C++ 实现，通过 `rust_cxx` 桥接被 Rust 侧调用。wrapper 层负责错误码转换和超时处理，将系统服务的错误映射为 Asset/SAF 的内部错误码。

## 开发要点

- 权限校验统一走 access_token_wrapper，不要在业务代码中直接调用 AccessTokenKit
- DataShare 操作通过 data_share_wrapper 封装，注意设备锁屏状态下的读写限制
- 内存管理 wrapper 的 NotifyStatus/SetCritical 接口需要与 memmgr 服务协同

## 常见开发场景

- 新增系统能力包装时，在此查找现有 wrapper 的实现模式和头文件
- 排查权限校验失败时，定位 access_token_wrapper 的 CheckPermission 逻辑
- 修改锁屏相关逻辑时，查看 screen_lock_wrapper 的状态查询接口

## 相关主题

- [Core Service 模块](./core-service.md)
- [Crypto Manager 模块](./crypto-manager.md)
- [Framework OS Dependency](../frameworks/framework-os-dependency.md)
- [Asset 服务架构](../architecture/asset-service.md)
