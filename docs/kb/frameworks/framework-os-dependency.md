# Framework OS Dependency 模块 Context

> 文档版本：v1.0
> 更新时间：2026-10-13
> 来源：`frameworks/asset/os_dependency/`、`frameworks/secure_access_fence/os_dependency/`

## 定位

框架层 OS Dependency 提供文件操作（CE/DE 目录）、日志、安全内存和 OpenSSL 包装，与服务层 OS Dependency（C++ 系统能力包装）互补。详见 [OS Dependency 模块](../services/os-dependency.md)。

## 快速路由

### 源码入口

### Asset 框架层 OS Dependency

| 模块 | crate 名 | 路径 | 语言 | 说明 |
|------|----------|------|------|------|
| 文件操作 | `asset_file_operator` | `frameworks/asset/os_dependency/file/src/` | Rust | DE/CE 目录操作 |
| 日志 | `asset_log` | `frameworks/asset/os_dependency/log/src/lib.rs` | Rust | 日志 |
| 安全内存 | — | `frameworks/asset/os_dependency/memory/src/asset_mem.c` | C | 安全内存分配/清零 |
| OpenSSL | — | `frameworks/asset/os_dependency/openssl/src/openssl_wrapper.c` | C | OpenSSL 加密包装 |

### 文件操作子模块

| 文件 | 说明 |
|------|------|
| `frameworks/asset/os_dependency/file/src/lib.rs` | 入口 |
| `frameworks/asset/os_dependency/file/src/common.rs` | DE_ROOT_PATH 等路径常量 |
| `frameworks/asset/os_dependency/file/src/de_operator.rs` | DE（Device Encrypted）目录操作 |
| `frameworks/asset/os_dependency/file/src/ce_operator.rs` | CE（Credential Encrypted）目录操作 |

### Asset 工具模块

| 文件 | 说明 |
|------|------|
| `frameworks/asset/utils/src/lib.rs` | 入口（crate: asset_utils） |
| `frameworks/asset/utils/src/time.rs` | 时间工具 |
| `frameworks/asset/utils/src/hasher.rs` | 哈希工具 |

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
| 安全内存包装测试 | `test/asset/unittest/module_test/src/asset_mem_wrapper_test.cpp` |
| HUKS 包装测试 | `test/asset/unittest/module_test/src/huks_wrapper_test.cpp` |

## 数据流

框架层 OS Dependency 在 Asset 服务调用链中的位置：

```
operation_*.rs → 需要文件操作时调用 asset_file_operator crate
  → de_operator.rs / ce_operator.rs → 系统文件 API
operation_*.rs → 需要日志时调用 asset_log crate → hilog
crypto_manager.rs → 需要非密钥加密时调用 openssl_wrapper.c → OpenSSL
所有模块 → 需要安全内存时调用 asset_mem.c → 安全分配/清零
```

框架层 OS Dependency 与服务层 OS Dependency 互补：框架层提供 Rust crate 供 Rust 代码直接调用，服务层提供 C++ wrapper 供 C++ 代码调用。两者都封装了相同的系统能力（文件、日志、内存等），但调用路径不同。

## 开发要点

- CE/DE 路径常量在 common.rs 中统一定义，不要在业务代码中硬编码路径
- 安全内存操作必须使用 asset_mem.c 提供的接口，不要直接调用 malloc/free
- OpenSSL 包装仅用于非密钥类加密（如哈希），密钥操作必须走 HUKS

## 常见开发场景

- 新增 CE/DE 文件操作时，在此查找路径常量和 operator 实现
- 需要安全分配和清零内存时，定位 asset_mem.c 的接口
- 添加非密钥类哈希/加密时，使用 openssl_wrapper 而非自行实现

## 相关主题

- [OS Dependency 模块（服务层）](../services/os-dependency.md)
- [Definition 模块](./definition.md)
- [Asset 服务架构](../architecture/asset-service.md)
