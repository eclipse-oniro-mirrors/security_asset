# Security Asset 知识库

本知识库基于 `security_asset` 代码仓自动生成，覆盖关键资产存储服务（ASSET，SA 8100）和安全访问围栏（SAF，SA 66532）的架构、API、服务模块、框架模块和常见问题。

## 目录结构

> 以下路径均为 `docs/kb/` 下的相对路径。按子系统组织：`asset/`（SA 8100）、`saf/`（SA 66532）、`hpic/`（隐私计算 NAPI），`shared/` 放跨子系统内容。

```
docs/kb/
├── README.md                           # 本文件
├── shared/                             # 跨子系统
│   └── overview.md                      # 整体架构概览（ASSET + SAF）
├── asset/                              # 关键资产存储服务（SA 8100）
│   ├── architecture/
│   │   └── service.md                  # Asset 服务架构
│   ├── api/
│   │   ├── ndk-api.md                  # 公共 NDK API（C）
│   │   ├── system-api.md               # C System API（inner_kits）
│   │   ├── rust-sdk.md                 # Asset Rust SDK
│   │   └── ipc.md                      # Asset IPC 接口定义
│   ├── services/
│   │   ├── core-service.md             # Asset Core Service 模块
│   │   ├── crypto-manager.md           # Crypto Manager 模块
│   │   ├── db-operator.md              # DB Operator 模块
│   │   ├── os-dependency.md            # Asset OS Dependency 模块
│   │   └── plugin-system.md            # Asset 插件系统
│   ├── frameworks/
│   │   ├── definition.md               # Asset Definition 模块（数据类型）
│   │   ├── ipc.md                      # Asset IPC 序列化模块
│   │   ├── napi-bindings.md            # Asset NAPI 绑定模块
│   │   └── os-dependency.md            # Asset Framework OS Dependency 模块
│   ├── error-codes.md                  # Asset 错误码参考
│   └── troubleshooting.md              # Asset 常见问题定位
├── saf/                                # 安全访问围栏（SA 66532）
│   ├── architecture/
│   │   └── service.md                  # SAF 架构
│   ├── api/
│   │   ├── agent-fence-api.md          # SAF Agent Fence API（C++）
│   │   ├── rust-sdk.md                 # SAF Rust SDK
│   │   └── ipc.md                      # SAF IPC 接口定义
│   ├── services/
│   │   ├── core-service.md             # SAF Core Service 模块
│   │   ├── os-dependency.md            # SAF OS Dependency 模块
│   │   └── plugin-system.md            # SAF 插件系统
│   ├── frameworks/
│   │   ├── definition.md               # SAF Definition 模块（数据类型）
│   │   ├── ipc.md                      # SAF IPC 序列化模块
│   │   ├── napi-bindings.md            # SAF NAPI 绑定模块
│   │   └── os-dependency.md            # SAF Framework OS Dependency 模块
│   ├── error-codes.md                  # SAF 错误码参考
│   └── troubleshooting.md              # SAF 常见问题定位
└── hpic/                               # 隐私计算
    └── napi-bindings.md                # HPIC NAPI 绑定模块
```

## 主题索引

> 表中"路径"列均为 `docs/kb/` 下的相对路径。

| 主题 | 路径 | 关键词 |
|------|------|--------|
| 整体架构概览 | `shared/overview.md` | ASSET, SAF, security, 两个子系统 |
| Asset 服务架构 | `asset/architecture/service.md` | SA-8100, core_service, operations |
| 安全访问围栏架构 | `saf/architecture/service.md` | SA-66532, ticket, remote_control |
| 公共 NDK API | `asset/api/ndk-api.md` | OH_Asset, asset_api.h, AssetStoreKit |
| C System API | `asset/api/system-api.md` | SEC_ASSET, inner_kits, batch |
| Asset Rust SDK | `asset/api/rust-sdk.md` | full.rs, empty.rs, asset_build_mode |
| Asset IPC 接口定义 | `asset/api/ipc.md` | IpcCode, SA_ID=8100, serialize |
| SAF Agent Fence API | `saf/api/agent-fence-api.md` | SafAgentFence, ticket, permission |
| SAF Rust SDK | `saf/api/rust-sdk.md` | libsaf_sdk, saf_ipc |
| SAF IPC 接口定义 | `saf/api/ipc.md` | IDL, SecureAccessFenceType, sub_modules |
| Asset Core Service 模块 | `asset/services/core-service.md` | AssetAbility, stub, operations |
| SAF Core Service 模块 | `saf/services/core-service.md` | SAFAbility, ticket_operation, remote_control |
| Crypto Manager 模块 | `asset/services/crypto-manager.md` | HUKS, AES-GCM, secret_key |
| DB Operator 模块 | `asset/services/db-operator.md` | SQLite, database, transaction |
| Asset OS Dependency 模块 | `asset/services/os-dependency.md` | access_token, bms, screenlock |
| SAF OS Dependency 模块 | `saf/services/os-dependency.md` | screen_lock, crypto_wrapper, cli_tool |
| Asset 插件系统 | `asset/services/plugin-system.md` | dlopen, IAssetPlugin, ext_ffi |
| SAF 插件系统 | `saf/services/plugin-system.md` | dlopen, SAFPlugin, saf_plugin |
| Asset Definition 模块 | `asset/frameworks/definition.md` | Tag, AssetMap, ErrCode |
| SAF Definition 模块 | `saf/frameworks/definition.md` | VerifyTicketInfo, RemoteAuthPackage, ErrCode |
| Asset IPC 序列化模块 | `asset/frameworks/ipc.md` | asset_ipc, serialize_map |
| SAF IPC 序列化模块 | `saf/frameworks/ipc.md` | saf_ipc, IDL, sub_modules |
| Asset NAPI 绑定模块 | `asset/frameworks/napi-bindings.md` | asset_napi, asset_napi_check |
| SAF NAPI 绑定模块 | `saf/frameworks/napi-bindings.md` | agent_fence_napi, agent_fence_error_codes |
| HPIC NAPI 绑定模块 | `hpic/napi-bindings.md` | hpic_napi, libprivacycomputation_napi |
| Asset Framework OS Dependency | `asset/frameworks/os-dependency.md` | file_operator, ce_operator, de_operator |
| SAF Framework OS Dependency | `saf/frameworks/os-dependency.md` | saf_log, saf_utils |
| Asset 错误码参考 | `asset/error-codes.md` | Asset_ResultCode, ErrCode, 24000001 |
| SAF 错误码参考 | `saf/error-codes.md` | SafResultCode, SAF_ASSIGN_MODULE_TYPE_ENUM |
| Asset 常见问题定位 | `asset/troubleshooting.md` | error, debug, asset_service |
| SAF 常见问题定位 | `saf/troubleshooting.md` | ticket, screenlock, secure_access_fence |

## 路由注册

所有 KB 页面注册在 `docs/context_registry.json` 中。

## 使用方式

AI agent 可通过 `docs/context_registry.json` 中的 `keywords` 和 `aliases` 字段检索相关 KB 页面，快速定位源码入口、API 入口和测试入口。
