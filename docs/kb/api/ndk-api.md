# 公共 NDK API Context

> 文档版本：v1.0
> 更新时间：2026-10-13
> 来源：`interfaces/kits/c/asset/`

## 定位

面向应用开发者的公共 C NDK 接口，库 `libasset_ndk.z.so`，Kit 名称 AssetStoreKit，起始版本 11。

## 快速路由

### 源码入口

| 文件 | 说明 |
|------|------|
| `interfaces/kits/c/asset/inc/asset_api.h` | API 函数声明 |
| `interfaces/kits/c/asset/inc/asset_type.h` | 类型定义（Tag、Blob、Attr、Result、ResultSet） |
| `interfaces/kits/c/asset/src/asset_api.c` | C 实现，调用 Rust FFI |
| `interfaces/kits/c/asset/BUILD.gn` | 构建目标 `libasset_ndk` |
| `interfaces/kits/c/asset/libasset_ndk.map` | 符号导出映射 |

### API 入口

| 函数 | 说明 |
|------|------|
| `OH_Asset_Add` | 新增关键资产 |
| `OH_Asset_Remove` | 删除关键资产 |
| `OH_Asset_Update` | 更新关键资产 |
| `OH_Asset_PreQuery` | 预查询（生成认证 Challenge） |
| `OH_Asset_Query` | 查询关键资产 |
| `OH_Asset_PostQuery` | 后查询（认证后清理） |
| `OH_Asset_QuerySyncResult` | 查询同步结果 |
| `OH_Asset_ParseAttr` | 解析属性 |
| `OH_Asset_FreeBlob` | 释放 Blob |
| `OH_Asset_FreeResultSet` | 释放结果集 |

### 关键类型

| 类型 | 定义位置 |
|------|----------|
| `Asset_Tag`、`Asset_TagType` | `asset_type.h` |
| `Asset_ResultCode` | `asset_type.h` |
| `Asset_Accessibility` | `asset_type.h` |
| `Asset_AuthType`、`Asset_SyncType`、`Asset_WrapType` | `asset_type.h` |
| `Asset_Blob`、`Asset_Attr`、`Asset_Result`、`Asset_ResultSet` | `asset_type.h` |
| `Asset_SyncResult` | `asset_type.h` |

### 调用链路

```
OH_Asset_* (C NDK) → asset_api.c → Rust FFI (interfaces/inner_kits/c/asset/src/full.rs)
  → IPC → AssetService (SA 8100) → operation_*.rs
```

### JS NAPI 对应

| NDK 函数 | NAPI 文件 |
|----------|----------|
| OH_Asset_Add | `frameworks/asset/js/napi/src/asset_napi_add.cpp` |
| OH_Asset_Remove | `frameworks/asset/js/napi/src/asset_napi_remove.cpp` |
| OH_Asset_Update | `frameworks/asset/js/napi/src/asset_napi_update.cpp` |
| OH_Asset_Query | `frameworks/asset/js/napi/src/asset_napi_query.cpp` |
| OH_Asset_PreQuery | `frameworks/asset/js/napi/src/asset_napi_pre_query.cpp` |
| OH_Asset_PostQuery | `frameworks/asset/js/napi/src/asset_napi_post_query.cpp` |
| OH_Asset_QuerySyncResult | `frameworks/asset/js/napi/src/asset_napi_query_sync_result.cpp` |

### 测试入口

| 测试 | 路径 |
|------|------|
| NDK 功能测试 | `test/asset/unittest/kits_ndk/src/asset_add_test.cpp` 等 |
| 认证查询测试 | `test/asset/unittest/kits_ndk/src/asset_auth_query_test.cpp` |

### 接口文档

- [接口文档](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-asset-store-kit/Readme-CN.md)
- [开发指导](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/security/AssetStoreKit/Readme-CN.md)

## 开发要点

- NDK API 是薄封装层，实际逻辑在 Rust FFI 和 IPC 侧，不要在 C 层添加业务逻辑
- 新增 NDK 函数需同步更新 asset_type.h 类型定义和 libasset_ndk.map 符号导出
- Asset_Blob 的 data 指针由调用方分配，length 不含终止符；释放必须用 OH_Asset_FreeBlob
- 返回值 Asset_ResultCode 与 Rust 侧 ErrCode 一一对应，新增错误码需双向同步

## 常见开发场景

- 应用开发者通过 NDK C 接口存取密码、Token 等短敏感数据
- 系统服务集成 C System API 实现批量资产管理
- 测试团队编写 NDK 接口功能/异常路径测试用例

## 相关主题

- [C System API](./system-api.md)
- [Rust SDK](./rust-sdk.md)
- [IPC 接口定义](./ipc-interfaces.md)
- [Asset 服务架构](../architecture/asset-service.md)
