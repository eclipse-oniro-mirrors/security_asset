# C System API Context

> 文档版本：v1.0
> 更新时间：2026-10-13
> 来源：`interfaces/inner_kits/c/asset/`、`frameworks/asset/c/system_api/`

## 定位

面向系统服务调用的 C/C++ 内部接口，起始版本 26，在公共 NDK API 基础上增加批量操作和更多 SEC_ASSET_TAG 标签（USER_ID、同步、信任环、设备管理等）。

## 快速路由

### 源码入口

| 文件 | 说明 |
|------|------|
| `interfaces/inner_kits/c/asset/inc/asset_system_api.h` | 系统级 API 函数声明（C++ vector 重载） |
| `interfaces/inner_kits/c/asset/inc/asset_system_type.h` | SEC_ASSET_* 标签/类型/错误码枚举 |
| `interfaces/inner_kits/c/asset/src/lib.rs` | Rust FFI 入口，按 build_mode 选择 full.rs / empty.rs |
| `interfaces/inner_kits/c/asset/src/full.rs` | FFI 完整实现 |
| `interfaces/inner_kits/c/asset/src/empty.rs` | 空桩实现（非 full 构建） |
| `frameworks/asset/c/system_api/src/asset_system_api.c` | C 包装层，调用 Rust FFI |
| `frameworks/asset/c/system_api/src/asset_batch.cpp` | C++ 批量 API 包装 |
| `frameworks/asset/c/system_api/BUILD.gn` | 构建目标 `libasset_sdk` |
| `interfaces/inner_kits/c/asset/BUILD.gn` | 构建目标 `libasset_sdk_ffi` |
| `interfaces/inner_kits/c/asset/libasset_sdk_ffi.map` | 符号导出映射 |

### API 入口

| 函数 | 说明 |
|------|------|
| `AssetAdd` | 新增关键资产（含 vector 重载） |
| `AssetRemove` | 删除关键资产 |
| `AssetUpdate` | 更新关键资产 |
| `AssetPreQuery` | 预查询 |
| `AssetQuery` | 查询 |
| `AssetPostQuery` | 后查询 |
| `AssetQuerySyncResult` | 查询同步结果 |
| `AssetParseAttr` | 解析属性 |
| `AssetFreeBlob` / `AssetFreeResultSet` | 释放资源 |
| `AssetBatchAdd` | 批量新增 |
| `AssetBatchRemove` | 批量删除 |
| `AssetBatchUpdate` | 批量更新 |

### 扩展标签（SEC_ASSET_TAG_*）

相比公共 NDK API，System API 额外支持：
- `SEC_ASSET_TAG_USER_ID` — 用户 ID
- 同步/信任环相关标签
- 设备管理相关标签
- 更多错误码（同步失败、信任环、设备管理）

### 测试入口

| 测试 | 路径 |
|------|------|
| 系统 API 测试 | `test/asset/unittest/module_test/src/asset_system_api_test.cpp` |
| 无权限测试 | `test/asset/unittest/module_test/src/asset_system_api_without_permission_test.cpp` |
| 批量 API 测试 | `test/asset/unittest/module_test/src/asset_system_batch_api_test.cpp` |

## 数据流

```
系统服务 C/C++ → AssetAdd/AssetBatchAdd 等
  → asset_system_api.c (C 包装层) → Rust FFI (full.rs)
  → IPC → AssetService (SA 8100) → operation_*.rs
  → 批量操作走 operation_batch_*.rs → process_batch_data.rs
```

System API 在 NDK 基础上增加批量操作和系统级标签（USER_ID、同步、信任环）。C++ vector 重载版本通过 `asset_batch.cpp` 处理，最终走 Rust FFI 发起 IPC。

## 开发要点

- System API 在 NDK 基础上增加批量操作（BatchAdd/BatchRemove/BatchUpdate）和 SEC_ASSET_TAG_USER_ID 等系统级标签
- C++ vector 重载版本仅在系统服务内部使用，不对三方应用暴露
- asset_build_mode 为 stub 时，full.rs 替换为 empty.rs，所有系统 API 返回未实现
- 新增 SEC_ASSET_TAG 需同步更新 asset_system_type.h 和 Rust 侧 Tag 枚举

## 常见开发场景

- 系统服务通过 System API 实现跨用户资产管理
- 设备管理器通过 SEC_ASSET_TAG 同步和信任环标签管理分布式资产
- 系统应用通过批量接口高效操作多条资产记录

## 相关主题

- [公共 NDK API](./ndk-api.md)
- [Rust SDK](./rust-sdk.md)
- [IPC 接口定义](./ipc.md)
- [Asset 服务架构](../architecture/service.md)
