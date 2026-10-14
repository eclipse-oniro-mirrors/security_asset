# 错误码参考 Context

> 文档版本：v1.0
> 更新时间：2026-10-13
> 来源：`interfaces/kits/c/asset/inc/asset_type.h`、`frameworks/asset/definition/src/lib.rs`、`interfaces/inner_kits/c/secure_access_fence/common/inc/saf_result_defs.h`

## 定位

关键资产存储服务（ASSET，SA 8100）和安全访问围栏（SAF，SA 66532）的完整错误码参考。供 SDD spec 阶段编写验收标准、review 阶段校验错误处理完整性、开发阶段排查问题使用。

## 快速路由

### 源码入口

| 子系统 | 定义文件 | 语言 |
|--------|----------|------|
| Asset NDK | `interfaces/kits/c/asset/inc/asset_type.h` | C（`Asset_ResultCode` 枚举） |
| Asset Rust | `frameworks/asset/definition/src/lib.rs` | Rust（`ErrCode` 枚举） |
| Asset System API | `interfaces/inner_kits/c/asset/inc/asset_system_type.h` | C（`SEC_ASSET_RESULT_CODE` 扩展） |
| SAF | `interfaces/inner_kits/c/secure_access_fence/common/inc/saf_result_defs.h` | C 宏（`SAF_ASSIGN_RESULT_CODE_ENUM`） |
| SAF Rust | `frameworks/secure_access_fence/definition/src/lib.rs` | Rust（`ErrCode` 枚举） |

### 测试入口

| 测试 | 路径 |
|------|------|
| 错误码异常路径测试 | `test/asset/unittest/inner_kits_rust/src/*_abnormal.rs` |
| NDK 错误码测试 | `test/asset/unittest/kits_ndk/src/asset_*_test.cpp` |
| SAF 参数校验测试 | `test/secure_access_fence/unittest/kits_ndk/src/saf_agent_params_checker_test.cpp` |
| SAF 权限拒绝测试 | `test/secure_access_fence/agent_fence/src/saf_agent_fence_no_permission_test.cpp` |

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

## SAF 错误码（SafResultCode）

### 模块类型编码

SAF 错误码采用 `模块类型 | 错误序号` 的组合编码方式，模块类型占高 20 位：

| 模块类型 | 值 | 说明 |
|----------|------|------|
| `IPC` | 0x10000 | IPC 通信 |
| `BMS` | 0x11000 | Bundle Manager |
| `SAMGR` | 0x12000 | Service Manager |
| `THREAD_POOL` | 0x13000 | 线程池 |
| `OS_ACCOUNT` | 0x14000 | 系统账号 |
| `ACCESS_TOKEN` | 0x15000 | 权限令牌 |
| `HUKS` | 0x17000 | 通用密钥库 |
| `CRYPTO` | 0x18000 | 加密操作 |
| `TRUSTED_RING` | 0x19000 | 信任环 |
| `CLI_TOOL` | 0x20000 | CLI 工具 |
| `LIBDL` | 0x21000 | 动态加载 |
| `SCREENLOCK` | 0x22000 | 锁屏服务 |
| `ARGUMENT` | 0x30000 | 参数校验 |
| `MEMORY` | 0x31000 | 内存管理 |
| `PERMISSION` | 0x32000 | 权限控制 |
| `COMMON` | 0x33000 | 通用错误 |
| `PLUGIN` | 0x70000 | 插件系统 |
| `TICKET_OPERATION` | 0x71000 | Ticket 操作 |

### 通用结果码

| 名称 | 值 | 说明 |
|------|------|------|
| `SAF_SUCCESS` | 0 | 操作成功 |
| `SAF_EVALUATE_DENY` | 1 | 评估拒绝 |
| `SAF_ERROR` | 2 | 通用错误 |

### IPC 模块错误码（0x10000 | N）

| 名称 | 值 | 说明 |
|------|------|------|
| `SAF_ERR_IPC_WRITE_DATA_FAIL` | 0x10001 | IPC 写数据失败 |
| `SAF_ERR_IPC_READ_DATA_FAIL` | 0x10002 | IPC 读数据失败 |
| `SAF_ERR_IPC_SEND_REQUEST_FAIL` | 0x10003 | IPC 发送请求失败 |
| `SAF_ERR_IPC_PROXY_FAIL` | 0x10004 | IPC 获取 Proxy 失败 |
| `SAF_ERR_IPC_ERROR` | 0x10005 | IPC 通用错误 |
| `SAF_ERR_IPC_INVALID_IPC_CODE` | 0x10006 | 无效 IPC Code |

### SAMGR 模块错误码（0x12000 | N）

| 名称 | 值 | 说明 |
|------|------|------|
| `SAF_ERR_SERVICE_UNAVAILABLE` | 0x12001 | SA 66532 不可用 |
| `SAF_ERR_SERVICE_IS_STOPPING` | 0x12002 | SA 66532 正在停止 |

### ARGUMENT 模块错误码（0x30000 | N）

| 名称 | 值 | 说明 |
|------|------|------|
| `SAF_ERR_INVALID_ARRAY_LEN` | 0x30003 | 无效数组长度 |
| `SAF_ERR_NULL_PTR` | 0x30004 | 空指针 |
| `SAF_ERR_ARG_EMPTY` | 0x30019 | 参数为空 |
| `SAF_ERR_INVALID_OS_ACCOUNT_ID` | 0x3001A | 无效系统账号 ID |
| `SAF_ERR_INVALID_PLUGIN` | 0x3001B | 无效插件 |
| `SAF_ERR_ARG_INVALID` | 0x3001C | 参数无效 |
| `SAF_ERR_QUERIED_PERMISSION_NOT_EXIST` | 0x3001D | 查询的权限不存在 |

### PERMISSION 模块错误码（0x32000 | N）

| 名称 | 值 | 说明 |
|------|------|------|
| `SAF_ERR_PERMISSION_DENIED` | 0x32001 | 权限被拒绝 |
| `SAF_ERR_NOT_SYSTEM_APP` | 0x32002 | 非系统应用 |

### CRYPTO 模块错误码（0x18000 | N）

| 名称 | 值 | 说明 |
|------|------|------|
| `SAF_ERR_CRYPTO_OPERATION` | 0x18001 | 加密操作失败 |
| `SAF_ERR_INVALID_HMAC_SIZE` | 0x18002 | HMAC 大小无效 |
| `SAF_ERR_INVALID_CHALLENGE_SIZE` | 0x18003 | Challenge 大小无效 |

### TICKET_OPERATION 模块错误码（0x71000 | N）

| 名称 | 值 | 说明 |
|------|------|------|
| `SAF_ERR_TICKET_KEY_MGR_NOT_SUPPORT` | 0x71001 | Ticket 密钥管理模式不支持 |
| `SAF_ERR_TICKET_TIME_INVALID` | 0x71002 | Ticket 时间无效（过期或未到生效时间） |

### SCREENLOCK 模块错误码（0x22000 | N）

| 名称 | 值 | 说明 |
|------|------|------|
| `SAF_ERR_SCREENLOCK_SERVICE_ERROR` | 0x22001 | 锁屏服务错误 |
| `SAF_ERR_SCREENLOCK_IS_LOCKED` | 0x22002 | 设备已锁屏 |

### TRUSTED_RING 模块错误码（0x19000 | N）

| 名称 | 值 | 说明 |
|------|------|------|
| `SAF_ERR_TRUSTED_RING_NETWORK_DISCONNECTED` | 0x19003 | 网络断开 |
| `SAF_ERR_TRUSTED_RING_NOT_LOGGED_IN` | 0x19004 | 未登录 |
| `SAF_ERR_TRUSTED_RING_TIMEOUT` | 0x19006 | 操作超时 |
| `SAF_ERR_TRUSTED_RING_REMOTE_TOKEN_EXPIRED` | 0x19007 | 远程 Token 过期 |
| `SAF_ERR_TRUSTED_RING_REMOTE_DEVICE_UNTRUSTED` | 0x19008 | 远程设备不受信任 |
| `SAF_ERR_TRUSTED_RING_LACK_OF_AUTH_TOKEN` | 0x19009 | 缺少认证 Token |
| `SAF_ERR_TRUSTED_RING_AUTH_TOKEN_IS_EXPIRED` | 0x1900A | 认证 Token 已过期 |

### 其他模块错误码

| 模块 | 名称 | 值 | 说明 |
|------|------|------|------|
| LIBDL | `SAF_ERR_DLOPEN_FAIL` | 0x21001 | dlopen 加载失败 |
| LIBDL | `SAF_ERR_DLSYM_FAIL` | 0x21002 | dlsym 符号查找失败 |
| COMMON | `SAF_ERR_DATA_TYPE_MISMATCH` | 0x33001 | 数据类型不匹配 |
| COMMON | `SAF_ERR_MAP_KEY_NOT_FOUND` | 0x33002 | Map 键未找到 |
| COMMON | `SAF_ERR_BASE64_INVALID_LEN` | 0x33003 | Base64 长度无效 |
| COMMON | `SAF_ERR_BASE64_INVALID_CHAR` | 0x33004 | Base64 字符无效 |
| CLI_TOOL | `SAF_ERR_TOOL_ERROR` | 0x20001 | CLI 工具错误 |
| PLUGIN | `SAF_ERR_CREATE_PLUGIN_MGR_FAIL` | 0x70001 | 创建插件管理器失败 |
| PLUGIN | `SAF_ERR_PLUGIN_INVALID_EVENT_TYPE` | 0x70002 | 无效插件事件类型 |
| PLUGIN | `SAF_ERR_PLUGIN_NOT_SUPPORT` | 0x70003 | 插件不支持 |
| BMS | `SAF_ERR_GET_BUNDLE_NAME_BY_TOKEN_FAILED` | 0x11009 | 通过 Token 获取包名失败 |

### 错误码提取宏

```c
// 从组合错误码中提取模块类型
#define SAF_EXTRACT_ERR_CODE_MODULE(code) ((code) >> 12)
```

## 错误码使用场景

### SDD spec 阶段

编写验收标准（AC）时，THEN 子句应引用具体错误码：

- Asset 操作类 AC：`THEN 返回 ASSET_NOT_FOUND (24000002)` 或 `THEN 返回 ASSET_ACCESS_DENIED (24000004)`
- SAF 操作类 AC：`THEN 返回 SAF_ERR_PERMISSION_DENIED (0x32001)` 或 `THEN 返回 SAF_ERR_SCREENLOCK_IS_LOCKED (0x22002)`

### SDD review 阶段

审查代码时验证：
- 每个 IPC 操作都有错误码返回处理
- HUKS 调用失败映射到 `CryptoError`（Asset）或 `SAF_ERR_CRYPTO_OPERATION`（SAF）
- IPC 序列化失败映射到 `IpcError` 或 `SAF_ERR_IPC_*`
- 权限校验失败映射到 `PermissionDenied` 或 `SAF_ERR_PERMISSION_DENIED`

### 开发排查

- 遇到 `24000009` → HUKS 加解密失败 → 检查密钥是否存在（`secret_key.rs → is_exist`）
- 遇到 `24000010` → IPC 失败 → 检查 SA 8100 进程（`ps -ef | grep asset_service`）
- 遇到 `0x32001` → SAF 权限拒绝 → 检查 `access_token_wrapper.cpp` 权限声明
- 遇到 `0x22002` → 设备锁屏 → 检查 `screen_lock_wrapper.cpp` 锁屏状态

## 开发要点

- Asset 错误码在 C（`Asset_ResultCode`）和 Rust（`ErrCode`）两侧一一对应，新增错误码必须双向同步
- SAF 错误码采用模块类型 | 序号的组合编码，新增模块需在 `saf_result_defs.h` 的 `SAF_ASSIGN_MODULE_TYPE_ENUM` 中注册
- NAPI 层错误码映射在 `asset_napi_error_code.h` 中，新增 NDK 错误码需同步更新
- System API 扩展错误码（24001001 等）仅系统服务可见，不对三方应用暴露

## 常见开发场景

- SDD spec 编写 AC 时查阅错误码枚举值和触发条件
- 开发者排查 Asset/SAF 操作失败时根据错误码定位排查路径
- 代码审查时验证错误处理是否覆盖所有可能的错误码

## 相关主题

- [公共 NDK API](./ndk-api.md)
- [C System API](./system-api.md)
- [IPC 接口定义](./ipc-interfaces.md)
- [Crypto Manager 模块](../services/crypto-manager.md)
- [Core Service 模块](../services/core-service.md)
- [常见问题定位](../troubleshooting/common-issues.md)
