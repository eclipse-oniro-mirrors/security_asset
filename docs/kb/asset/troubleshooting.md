# Asset 常见问题定位 Context

> 文档版本：v1.0
> 更新时间：2026-10-13
> 来源：代码仓结构分析

## 定位

汇总关键资产存储服务（ASSET，SA 8100）开发中的常见问题、排查路径和注意事项。

## 常见问题定位

### 1. Asset 操作返回错误码

| 症状 | 排查路径 |
|------|----------|
| `ERR_ACCESS_CONTROL_DENIED` | 检查 `access_token_wrapper` 权限校验逻辑，确认应用是否声明了 `SystemCapability.Security.Asset` |
| `ERR_NOT_FOUND` | 检查 DB Operator 查询条件，确认 `AssetMap` 中的 Tag 是否匹配 |
| `ERR_CRYPTO_ERROR` | 检查 HUKS 密钥是否存在（`secret_key.rs` → `is_exist`），可能密钥已删除 |
| `ERR_IPC_ERROR` | 检查 SA 8100 是否已启动（`sa_profile/8100.json`），查看 `stub.rs` IPC 分发 |
| `ERR_DB_ERROR` | 检查 SQLite 数据库文件路径（`database.rs` → `get_path`），确认 CE/DE 路径 |
| `ERR_OVER_MAX_SIZE` | 检查数据长度是否超过 1024 字节限制 |
| `ERR_OUT_OF_MEMORY` | 检查 `CryptoManager` 缓存容量（CRYPTO_CAPACITY=16, CRYPTO_TOTAL_CAPACITY=32） |

### 2. SA 启动失败

| 症状 | 排查路径 |
|------|----------|
| SA 8100 未启动 | 检查 `etc/init/asset_service.cfg`（uid、权限、SELinux），`sa_profile/8100.json`（按需启动事件） |
| `dlopen` 失败 | 检查 `libasset_ext_ffi.z.so` 是否存在（`plugin/asset_plugin.rs`） |

### 3. 加解密失败

| 症状 | 排查路径 |
|------|----------|
| 加密失败 | `huks_wrapper.c` → HUKS API 调用，检查 AES-GCM 参数（TAG_SIZE=16, NONCE_SIZE=12） |
| 密钥不存在 | `secret_key.rs` → `generate` / `is_exist`，确认 HUKS 密钥别名 |
| DB 密钥获取失败 | `db_key_operator.rs` → `get_db_key(user_id, is_ce)`，检查 CE/DE 场景 |
| 用户认证未通过 | 检查 PreQuery/PostQuery 流程，确认 UserIAM 认证结果传递正确 |

### 4. 数据库问题

| 症状 | 排查路径 |
|------|----------|
| 数据库未找到 | `database.rs` → `get_path`，检查 user_id 和 CE/DE 路径 |
| 数据库升级失败 | `upgrade_operator.rs` → `check_and_split_db`，检查 DB 文件版本 |
| CE 升级失败 | `upgrade_ce.rs`，检查 CE 目录操作权限 |
| 批量操作失败 | `process_batch_data.rs`，检查事务管理（`transaction.rs`） |

### 5. 构建问题

| 症状 | 排查路径 |
|------|----------|
| `product not found` | 确认 `BUILD.gn` 顶层目标 `asset_component` |
| `undefined reference` | 检查 `Cargo.toml` workspace 成员、`BUILD.gn` 依赖链 |
| `asset_build_mode` 不对 | 检查 `config.gni` 中 `asset_build_mode` 值（full / stub） |
| 链接错误 | 检查各 `.map` 符号映射文件 |

## 调试入口

### 日志

| 日志类型 | 位置 |
|----------|------|
| Asset 日志 | `frameworks/asset/os_dependency/log/`（Rust + C 头文件） |
| HiSysEvent | `hisysevent.yaml`（SECRET_STORE_INFO_COLLECTION、SECRET_STORE_OPERATION_FAILED） |
| HiTrace | `services/asset/core_service/src/trace_scope.rs` |
| SysEvent 上报 | `services/asset/core_service/src/sys_event.rs` |

### 调试命令

```bash
# 编译 asset 模块
./build.sh --product-name rk3568 --ccache --build-target asset

# 编译 asset 测试
./build.sh --product-name rk3568 --ccache --build-target asset_bin_test

# 查看数据库
hdc shell
# 数据库路径由 database.rs get_path 决定，通常在 /data/service/asset/

# 查看 SA 状态
hdc shell ps -ef | grep asset_service
```

## 开发要点

- 排查问题优先查看 HiSysEvent（hisysevent.yaml 定义了 SECRET_STORE_INFO_COLLECTION 和 SECRET_STORE_OPERATION_FAILED）
- SA 启动失败首先检查 init 配置（etc/init/asset_service.cfg）和 sa_profile（8100.json）
- 加解密问题通过 huks_wrapper.c 日志定位，确认 HUKS API 返回值
- 数据库问题通过 database.rs 的 get_path 确认 CE/DE 路径

## 常见开发场景

- 开发者排查 Asset 操作返回错误码的原因
- 测试团队定位 SA 8100 启动失败或 IPC 通信异常
- 运维人员通过 HiSysEvent 监控资产存储服务运行状态

## 相关主题

- [整体架构概览](../shared/overview.md)
- [Core Service 模块](./services/core-service.md)
- [Crypto Manager 模块](./services/crypto-manager.md)
- [OS Dependency 模块](./services/os-dependency.md)
- [错误码参考](./error-codes.md)
