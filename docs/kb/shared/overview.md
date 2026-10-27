# ASSET 服务整体架构 Context

> 文档版本：v1.0
> 更新时间：2026-10-13
> 来源：代码仓结构分析

## 定位

关键资产存储服务（ASSET，`@ohos/asset`）是 OpenHarmony security 子系统下的短敏感数据安全存储服务。提供密码、Token、银行卡号等长度 <1024 字节数据的加密存储与访问控制能力。加解密和访问控制校验委托给 HUKS（通用密钥库系统）在安全环境（如 TEE）中完成。

## 快速路由

### 源码入口

| 模块 | 路径 |
|------|------|
| 顶层构建 | `BUILD.gn`、`config.gni`、`Cargo.toml` |
| 组件元数据 | `bundle.json`（name=`@ohos/asset`、subsystem=`security`、syscap=`SystemCapability.Security.Asset`） |
| Asset 服务（SA 8100） | `services/asset/core_service/` |
| SAF 服务（SA 66532） | `services/secure_access_fence/core_service/` |
| Asset 框架层 | `frameworks/asset/` |
| SAF 框架层 | `frameworks/secure_access_fence/` |
| 公共 NAPI 工具 | `frameworks/common/napi/` |
| 隐私计算 NAPI | `frameworks/hpic/js/napi/` |

### API 入口

| 接口类型 | 路径 |
|----------|------|
| 公共 NDK API（C） | `interfaces/kits/c/asset/inc/asset_api.h`、`asset_type.h` |
| 系统 API（C/C++） | `interfaces/inner_kits/c/asset/inc/asset_system_api.h`、`asset_system_type.h` |
| Rust SDK | `interfaces/inner_kits/rs/asset/src/` |
| SAF Agent Fence（C++） | `interfaces/inner_kits/c/secure_access_fence/agent_fence/inc/saf_agent_fence.h` |
| SAF Rust SDK | `interfaces/inner_kits/rs/secure_access_fence/src/` |
| 插件接口（Rust trait） | `interfaces/inner_kits/plugin_interface/asset/src/plugin_interface.rs` |
| SAF 插件接口 | `interfaces/inner_kits/plugin_interface/secure_access_fence/src/plugin_interface.rs` |

### 测试入口

| 测试类型 | 路径 |
|----------|------|
| NDK 测试 | `test/asset/unittest/kits_ndk/` |
| 模块测试 | `test/asset/unittest/module_test/` |
| Rust SDK 测试 | `test/asset/unittest/inner_kits_rust/` |
| UT 测试（stub） | `test/asset/unittest/ut_test/` |
| SAF Agent Fence 测试 | `test/secure_access_fence/agent_fence/` |
| SAF NDK 测试 | `test/secure_access_fence/unittest/kits_ndk/` |
| SAF UT 测试 | `test/secure_access_fence/unittest/ut_test/` |

## 两个子系统

### 1. Asset 服务（SA 8100）

进程名 `asset_service`，提供关键资产的增删改查、批量操作、预查询/后查询（用户认证场景）、同步结果查询。

数据流：App → NDK/NAPI → Rust SDK → IPC → AssetService（stub.rs） → Operations → CryptoManager（HUKS 加解密）→ DB Operator（SQLite 密文存储）

### 2. 安全访问围栏 SAF（SA 66532）

进程名 `secure_access_fence`，提供基于 Ticket 的权限围栏、远程控制（Controller/Controlled Device 包）、命令权限批量查询与授予。

数据流：App → NAPI/Agent Fence → IPC → SAFService（stub.rs） → Ticket Operation（密钥管理+防重放）→ Remote Control（分布式权限授予）

## 构建配置

| 配置项 | 说明 |
|--------|------|
| `asset_access_control_enabled` | 访问控制使能 |
| `asset_split_hap_list` | HAP 分裂列表 |
| `asset_extend_timeout` | 超时延长 |
| `asset_ce_upgrade_list` | CE 升级列表 |
| `asset_build_mode` | 构建模式（full / stub） |
| `asset_enable_privacy_computation_config` | 隐私计算使能 |

## 外部依赖

| 依赖 | 用途 |
|------|------|
| `huks` | 通用密钥库——加解密和访问控制在安全环境执行 |
| `sqlite` | 加密数据存储数据库 |
| `ipc` | 进程间通信 |
| `safwk` / `samgr` | SystemAbility 框架与服务管理 |
| `access_token` | 权限校验 |
| `bundle_framework` | 应用包管理 |
| `os_account` | 系统账号信息 |
| `screenlock_mgr` | 锁屏状态 |
| `data_share` | DataShare 键值存储 |
| `memmgr` | 内存管理 |
| `openssl` | OpenSSL 加密包装 |
| `napi` / `node` | JS NAPI 绑定 |
| `rust_cxx` | Rust/C++ FFI 桥接 |

## 开发要点

- ASSET 服务（SA 8100）和 SAF 服务（SA 66532）是独立进程，通过各自 init 配置启动
- 所有加解密操作委托给 HUKS 在安全环境执行，Asset 服务本身不持有明文密钥
- 插件系统通过 dlopen 加载扩展库，核心服务不依赖插件即可运行
- 构建配置在 config.gni 中集中管理，新增 feature 需在此注册

## 常见开发场景

- 新开发者了解 ASSET 服务整体架构和模块关系
- 架构评审时查阅两个子系统的职责边界
- 移植到新平台时评估 OS Dependency 和 HUKS 依赖

## 相关主题

- [Asset 服务架构](../asset/architecture/service.md)
- [安全访问围栏架构](../saf/architecture/service.md)
- [公共 NDK API](../asset/api/ndk-api.md)
- [Core Service 模块](../asset/services/core-service.md)
- [Crypto Manager 模块](../asset/services/crypto-manager.md)
- [SAF Agent Fence API](../saf/api/agent-fence-api.md)
