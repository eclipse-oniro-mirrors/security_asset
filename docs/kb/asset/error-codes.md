# Asset 错误码参考 Context

> 文档版本：v1.0
> 更新时间：2026-10-13
> 来源：`interfaces/kits/c/asset/inc/asset_type.h`、`frameworks/asset/definition/src/lib.rs`、`interfaces/inner_kits/c/asset/inc/asset_system_type.h`

## 定位

关键资产存储服务（ASSET，SA 8100）的完整错误码参考。供 SDD spec 阶段编写验收标准、review 阶段校验错误处理完整性、开发阶段排查问题使用。

## 快速路由

### 源码入口

| 子系统 | 定义文件 | 语言 |
|--------|----------|------|
| Asset NDK | `interfaces/kits/c/asset/inc/asset_type.h` | C（`Asset_ResultCode` 枚举） |
| Asset Rust | `frameworks/asset/definition/src/lib.rs` | Rust（`ErrCode` 枚举） |
| Asset System API | `interfaces/inner_kits/c/asset/inc/asset_system_type.h` | C（`SEC_ASSET_RESULT_CODE` 扩展） |

### 测试入口

| 测试 | 路径 |
|------|------|
| 错误码异常路径测试 | `test/asset/unittest/inner_kits_rust/src/*_abnormal.rs` |
| NDK 错误码测试 | `test/asset/unittest/kits_ndk/src/asset_*_test.cpp` |

## Asset 错误码（Asset_ResultCode / ErrCode）

### 通用错误码

| 枚举值 | 数值 | C 名称 | 触发条件 |
|--------|------|--------|----------|
| `ASSET_SUCCESS` | 0 | `ASSET_SUCCESS` | 操作成功 |
| `PermissionDenied` | 201 | `ASSET_PERMISSION_DENIED` | 应用未声明所需权限或权限校验未通过 |
| `InvalidArgument` | 401 | `ASSET_INVALID_ARGUMENT` | 参数无效（必填缺失、类型不匹配、值越界） |

### 服务级错误码（24000001-24000018）

| 枚举值 | 数值 | C 名称 | 触发条件 | 排查路径 |
|--------|------|--------|----------|----------|
| `ServiceUnavailable` | 24000001 | `ASSET_SERVICE_UNAVAILABLE` | SA 8100 未启动或不可用 | 检查 `etc/init/asset_service.cfg`、`sa_profile/8100.json` |
| `NotFound` | 24000002 | `ASSET_NOT_FOUND` | 查询条件未匹配到记录 | 检查 `db_operator/database.rs` 查询条件和 `table.rs` 表结构 |
| `Duplicated` | 24000003 | `ASSET_DUPLICATED` | 新增时别名已存在且未指定覆盖策略 | 检查 `ConflictResolution` 标签值（OVERWRITE=0 / THROW_ERROR=1） |
| `AccessDenied` | 24000004 | `ASSET_ACCESS_DENIED` | 访问控制校验未通过（Accessibility 级别不足） | 检查 `Asset_Accessibility` 值与设备锁屏状态 |
| `StatusMismatch` | 24000005 | `ASSET_STATUS_MISMATCH` | 资产状态与操作不匹配（如 PreQuery 后未 PostQuery 即查询） | 检查 PreQuery/PostQuery 配对使用 |
| `OutOfMemory` | 24000006 | `ASSET_OUT_OF_MEMORY` | CryptoManager 缓存满（CRYPTO_CAPACITY=16, CRYPTO_TOTAL_CAPACITY=32）或系统内存不足 | 检查 `crypto_manager.rs` 缓存状态和 `memory_manager_wrapper` |
| `DataCorrupted` | 24000007 | `ASSET_DATA_CORRUPTED` | 数据完整性校验失败（AES-GCM Tag 不匹配） | 检查 HUKS 密钥是否被删除后重建（`secret_key.rs`） |
| `DatabaseError` | 24000008 | `ASSET_DATABASE_ERROR` | SQLite 操作失败 | 检查 `database.rs` 路径、`sqlite3_wrapper.c` 返回值 |
| `CryptoError` | 24000009 | `ASSET_CRYPTO_ERROR` | HUKS 加解密操作失败 | 检查 `huks_wrapper.c` 返回值、密钥是否存在 |
| `IpcError` | 24000010 | `ASSET_IPC_ERROR` | IPC 通信失败 | 检查 SA 8100 进程状态、`stub.rs` 分发逻辑 |
| `BmsError` | 24000011 | `ASSET_BMS_ERROR` | Bundle Manager 服务调用失败 | 检查 `bms_wrapper.cpp` 和 bundle_framework 依赖 |
| `AccountError` | 24000012 | `ASSET_ACCOUNT_ERROR` | OS Account 服务调用失败 | 检查 `os_account_wrapper.cpp` 和 os_account 依赖 |
| `AccessTokenError` | 24000013 | `ASSET_ACCESS_TOKEN_ERROR` | Access Token 权限校验失败 | 检查 `access_token_wrapper.cpp` 的 CheckPermission/CheckIsSystemHap |
| `FileOperationError` | 24000014 | `ASSET_FILE_OPERATION_ERROR` | CE/DE 文件操作失败 | 检查 `file_operator_wrapper.cpp` 和 `de_operator.rs`/`ce_operator.rs` |
| `GetSystemTimeError` | 24000015 | `ASSET_GET_SYSTEM_TIME_ERROR` | 系统时间获取失败 | 检查 `utils/time.rs` 和 time_service 依赖 |
| `LimitExceeded` | 24000016 | `ASSET_LIMIT_EXCEEDED` | 超过限制（数据长度 >1024 字节、返回结果超限等） | 检查 `Asset_Blob` length 和 `ReturnLimit` 标签 |
| `Unsupported` | 24000017 | `ASSET_UNSUPPORTED` | 当前构建模式不支持（asset_build_mode 非 full） | 检查 `config.gni` 中 `asset_build_mode` 值 |
| `ParamVerificationFailed` | 24000018 | `ASSET_PARAM_VERIFICATION_FAILED` | 参数校验失败（NAPI 层或服务层参数校验未通过） | 检查 `asset_napi_check.cpp` 和 `operations/common.rs` |

### System API 扩展错误码

| 枚举值 | 数值 | 说明 | 来源 |
|--------|------|------|------|
| `HwAccountError` | 24001001 | HW 账号相关错误 | `frameworks/asset/definition/src/lib.rs`（Rust 侧） |

System API（`asset_system_type.h`）额外包含同步、信任环、设备管理相关错误码，具体值定义在 `SEC_ASSET_RESULT_CODE` 枚举中，参见 `interfaces/inner_kits/c/asset/inc/asset_system_type.h`。

## 错误码使用场景

### SDD spec 阶段

编写验收标准（AC）时，THEN 子句应引用具体错误码：

- Asset 操作类 AC：`THEN 返回 ASSET_NOT_FOUND (24000002)` 或 `THEN 返回 ASSET_ACCESS_DENIED (24000004)`

### SDD review 阶段

审查代码时验证：
- 每个 IPC 操作都有错误码返回处理
- HUKS 调用失败映射到 `CryptoError`
- IPC 序列化失败映射到 `IpcError`
- 权限校验失败映射到 `PermissionDenied`

### 开发排查

- 遇到 `24000009` → HUKS 加解密失败 → 检查密钥是否存在（`secret_key.rs → is_exist`）
- 遇到 `24000010` → IPC 失败 → 检查 SA 8100 进程（`ps -ef | grep asset_service`）

## 开发要点

- Asset 错误码在 C（`Asset_ResultCode`）和 Rust（`ErrCode`）两侧一一对应，新增错误码必须双向同步
- NAPI 层错误码映射在 `asset_napi_error_code.h` 中，新增 NDK 错误码需同步更新
- System API 扩展错误码（24001001 等）仅系统服务可见，不对三方应用暴露

## 常见开发场景

- SDD spec 编写 AC 时查阅错误码枚举值和触发条件
- 开发者排查 Asset 操作失败时根据错误码定位排查路径
- 代码审查时验证错误处理是否覆盖所有可能的错误码

## 相关主题

- [公共 NDK API](./api/ndk-api.md)
- [C System API](./api/system-api.md)
- [IPC 接口定义](./api/ipc.md)
- [Crypto Manager 模块](./services/crypto-manager.md)
- [Core Service 模块](./services/core-service.md)
- [常见问题定位](./troubleshooting.md)
