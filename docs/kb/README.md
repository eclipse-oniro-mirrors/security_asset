# Security Asset 知识库

本知识库基于 `security_asset` 代码仓自动生成，覆盖关键资产存储服务（ASSET，SA 8100）和安全访问围栏（SAF，SA 66532）的架构、API、服务模块、框架模块和常见问题。

## 目录结构

> 以下路径均为 `docs/kb/` 下的相对路径。

```
docs/kb/
├── README.md                           # 本文件
├── architecture/                       # 架构概览
│   ├── overview.md                      # 整体架构概览
│   ├── asset-service.md                # Asset 服务架构（SA 8100）
│   └── secure-access-fence.md          # 安全访问围栏架构（SA 66532）
├── api/                                 # API 接口
│   ├── ndk-api.md                      # 公共 NDK API（C）
│   ├── system-api.md                   # C System API（inner_kits）
│   ├── rust-sdk.md                     # Rust SDK
│   ├── saf-agent-fence.md              # SAF Agent Fence API（C++）
│   └── ipc-interfaces.md               # IPC 接口定义
├── services/                            # 服务模块
│   ├── core-service.md                 # Core Service 模块
│   ├── crypto-manager.md               # Crypto Manager 模块
│   ├── db-operator.md                  # DB Operator 模块
│   ├── os-dependency.md                # OS Dependency 模块
│   └── plugin-system.md                # 插件系统
├── frameworks/                          # 框架模块
│   ├── definition.md                   # Definition 模块（数据类型）
│   ├── ipc.md                           # IPC 序列化模块
│   ├── napi-bindings.md                # NAPI 绑定模块
│   └── framework-os-dependency.md      # Framework OS Dependency 模块
└── troubleshooting/                    # 问题排查
    └── common-issues.md                # 常见问题定位
```

## 主题索引

> 表中"路径"列均为 `docs/kb/` 下的相对路径。

| 主题 | 路径 | 关键词 |
|------|------|--------|
| 整体架构概览 | `architecture/overview.md` | ASSET, SAF, security, 两个子系统 |
| Asset 服务架构 | `architecture/asset-service.md` | SA-8100, core_service, operations |
| 安全访问围栏架构 | `architecture/secure-access-fence.md` | SA-66532, ticket, remote_control |
| 公共 NDK API | `api/ndk-api.md` | OH_Asset, asset_api.h, AssetStoreKit |
| C System API | `api/system-api.md` | SEC_ASSET, inner_kits, batch |
| Rust SDK | `api/rust-sdk.md` | full.rs, empty.rs, asset_build_mode |
| SAF Agent Fence API | `api/saf-agent-fence.md` | SafAgentFence, ticket, permission |
| IPC 接口定义 | `api/ipc-interfaces.md` | IpcCode, IDL, serialize |
| Core Service 模块 | `services/core-service.md` | AssetAbility, SAFAbility, stub |
| Crypto Manager 模块 | `services/crypto-manager.md` | HUKS, AES-GCM, secret_key |
| DB Operator 模块 | `services/db-operator.md` | SQLite, database, transaction |
| OS Dependency 模块 | `services/os-dependency.md` | access_token, bms, screenlock |
| 插件系统 | `services/plugin-system.md` | dlopen, IAssetPlugin, ext_ffi |
| Definition 模块 | `frameworks/definition.md` | Tag, AssetMap, ErrCode |
| IPC 序列化模块 | `frameworks/ipc.md` | asset_ipc, saf_ipc, serialize |
| NAPI 绑定模块 | `frameworks/napi-bindings.md` | asset_napi, agent_fence_napi |
| Framework OS Dependency | `frameworks/framework-os-dependency.md` | file_operator, ce_operator, de_operator |
| 常见问题定位 | `troubleshooting/common-issues.md` | error, debug, troubleshooting |

## 路由注册

所有 KB 页面注册在 `docs/context_registry.json` 中。

## 使用方式

AI agent 可通过 `docs/context_registry.json` 中的 `keywords` 和 `aliases` 字段检索相关 KB 页面，快速定位源码入口、API 入口和测试入口。
