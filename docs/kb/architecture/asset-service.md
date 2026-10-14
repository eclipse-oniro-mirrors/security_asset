# Asset 服务架构 Context

> 文档版本：v1.0
> 更新时间：2026-10-13
> 来源：代码仓结构分析

## 定位

Asset 服务是 SA 8100（进程 `asset_service`），负责关键资产的加密存储、查询、更新、删除及批量操作。核心架构分为：框架层（frameworks/asset）、服务层（services/asset）、接口层（interfaces/kits、inner_kits）。

## 快速路由

### 源码入口

| 模块 | 路径 | 说明 |
|------|------|------|
| SA 入口 | `services/asset/core_service/src/lib.rs` | `AssetAbility` 实现 `Ability` trait，静态 `.init_array` 注册 SA 8100 |
| IPC Stub | `services/asset/core_service/src/stub.rs` | `RemoteStub` 实现，按 `IpcCode` 分发请求 |
| 操作分发 | `services/asset/core_service/src/operations.rs` | Add/Remove/Update/Query/PreQuery/PostQuery/QuerySyncResult/Batch* |
| 加密管理 | `services/asset/crypto_manager/src/crypto_manager.rs` | `CryptoManager` 单例，按应用缓存 Crypto |
| HUKS 包装 | `services/asset/crypto_manager/src/huks_wrapper.c` / `.h` | HUKS C API 调用（AES-GCM） |
| 数据库操作 | `services/asset/db_operator/src/database.rs` | SQLite 数据库管理 |
| SQLite 包装 | `services/asset/db_operator/src/sqlite3_wrapper.c` | sqlite3 C API 直接调用 |
| DB 密钥 | `services/asset/crypto_manager/src/db_key_operator.rs` | 按 user_id 生成/获取数据库加密密钥 |
| OS 适配 | `services/asset/os_dependency/src/` | access_token、bms、screenlock、os_account、memmgr 等包装 |
| 插件系统 | `services/asset/plugin/src/asset_plugin.rs` | `dlopen libasset_ext_ffi.z.so` 扩展事件 |
| 公共模块 | `services/asset/common/src/` | CallingInfo、Counter、TaskManager、ProcessInfo |

### API 入口

| 接口 | 路径 |
|------|------|
| IPC 定义 | `frameworks/asset/ipc/src/lib.rs`（IpcCode 枚举、SA_ID=8100、序列化） |
| 数据类型 | `frameworks/asset/definition/src/lib.rs`（Tag、AssetMap、ErrCode、AssetError） |
| 错误宏 | `frameworks/asset/definition/src/macros_lib.rs`（log_throw_error!、hisysevent!） |

### 数据流

```
App → OH_Asset_* (NDK C) / asset.* (JS NAPI)
  → Rust FFI (interfaces/inner_kits/c/asset/src/full.rs)
  → Rust SDK (interfaces/inner_kits/rs/asset/src/full.rs)
  → IPC → AssetService::on_remote_request (stub.rs)
  → operation_*.rs (业务逻辑)
  → CryptoManager → HUKS (加解密)
  → DB Operator → SQLite (密文存储)
  → DB Key Operator → HUKS (数据库密钥)
```

### 操作模块

| 操作 | 文件 |
|------|------|
| Add | `services/asset/core_service/src/operations/operation_add.rs` |
| Remove | `services/asset/core_service/src/operations/operation_remove.rs` |
| Update | `services/asset/core_service/src/operations/operation_update.rs` |
| Query | `services/asset/core_service/src/operations/operation_query.rs` |
| PreQuery | `services/asset/core_service/src/operations/operation_pre_query.rs` |
| PostQuery | `services/asset/core_service/src/operations/operation_post_query.rs` |
| QuerySyncResult | `services/asset/core_service/src/operations/operation_query_sync_result.rs` |
| BatchAdd | `services/asset/core_service/src/operations/operation_batch_add.rs` |
| BatchRemove | `services/asset/core_service/src/operations/operation_batch_remove.rs` |
| BatchUpdate | `services/asset/core_service/src/operations/operation_batch_update.rs` |

### 生命周期与事件

| 功能 | 路径 |
|------|------|
| SA 生命周期 | `services/asset/core_service/src/lib.rs`（on_start/on_active/on_idle/on_stop/on_extension） |
| 公共事件监听 | `services/asset/core_service/src/common_event.rs` 及子模块 |
| DB 升级 | `services/asset/core_service/src/upgrade_operator.rs` |
| CE 升级 | `services/asset/core_service/src/upgrade_ce.rs` |
| 使用统计 | `services/asset/core_service/src/usage_statistics.rs` |
| HiSysEvent | `services/asset/core_service/src/sys_event.rs` |
| HiTrace | `services/asset/core_service/src/trace_scope.rs` |

### 进程配置

| 文件 | 说明 |
|------|------|
| `etc/init/asset_service.cfg` | init 进程配置（uid、权限、SELinux） |
| `sa_profile/8100.json` | SA 按需启动事件配置 |
| `hisysevent.yaml` | HiSysEvent 定义（SECRET_STORE_INFO_COLLECTION、SECRET_STORE_OPERATION_FAILED） |

### 测试入口

| 测试 | 路径 |
|------|------|
| Asset NDK Add 测试 | `test/asset/unittest/kits_ndk/src/asset_add_test.cpp` |
| Asset Inner Kits Rust Add 测试 | `test/asset/unittest/inner_kits_rust/src/add.rs` |
| Asset Core Service UT | `test/asset/unittest/ut_test/services/core_service/src/lib.rs` |

## 开发要点

- 操作模块（operation_*.rs）每个对应一个 IPC Code，新增操作需同步注册
- 公共事件监听在 SA 启动时注册，修改事件需同步更新 listener.rs
- DB 升级逻辑在 upgrade_operator.rs，修改表结构需补充升级路径

## 常见开发场景

- 新增 Asset 操作时，在此查找端到端架构从 NDK 到 DB 的完整链路
- 排查 SA 启动问题时，定位 SA 入口和 on_start 生命周期回调
- 修改进程配置时，确认 init cfg、SA profile 和 hisysevent 定义

## 相关主题

- [整体架构概览](./overview.md)
- [Core Service 模块](../services/core-service.md)
- [Crypto Manager 模块](../services/crypto-manager.md)
- [DB Operator 模块](../services/db-operator.md)
- [公共 NDK API](../api/ndk-api.md)
- [IPC 接口定义](../api/ipc-interfaces.md)
