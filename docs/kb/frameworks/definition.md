# Definition 模块 Context

> 文档版本：v1.0
> 更新时间：2026-10-13
> 来源：`frameworks/asset/definition/`、`frameworks/secure_access_fence/definition/`

## 定位

Definition 模块定义两个子系统的核心数据类型：Tag 枚举、AssetMap、错误码、错误类型、同步结果、辅助宏和扩展类型。是所有其他模块共享的数据基础。

## 快速路由

### 源码入口

### Asset Definition

| 文件 | 说明 |
|------|------|
| `frameworks/asset/definition/src/lib.rs` | Tag 枚举、DataType、AssetMap、ErrCode、AssetError、SyncResult、Accessibility、Conversion trait |
| `frameworks/asset/definition/src/macros.rs` | impl_enum_trait!、impl_tag_trait! 宏 |
| `frameworks/asset/definition/src/macros_lib.rs` | log_throw_error!、log_and_into_asset_error!、hisysevent! 函数 |
| `frameworks/asset/definition/src/extension.rs` | Extension 类型定义 |
| `frameworks/asset/definition/BUILD.gn` | 构建配置 |

### SAF Definition

| 文件 | 说明 |
|------|------|
| `frameworks/secure_access_fence/definition/src/lib.rs` | SAF Tag/DataType、VerifyTicketInfo、CliInfo、PermissionQuery、RemoteAuthPackage、RemoteUserAuthResults、RemoteInfo、ErrCode |
| `frameworks/secure_access_fence/definition/src/macros.rs` | SAF 宏 |
| `frameworks/secure_access_fence/definition/src/macros_lib.rs` | SAF 日志/错误宏 |
| `frameworks/secure_access_fence/definition/src/extension.rs` | SAF 扩展类型 |
| `frameworks/secure_access_fence/definition/BUILD.gn` | 构建配置 |

### 关键类型

#### Asset

| 类型 | 说明 |
|------|------|
| `Tag` | 资产标签枚举（对应 Asset_Tag） |
| `AssetMap` | 标签到值的映射 |
| `ErrCode` | 错误码枚举 |
| `AssetError` | 错误类型 |
| `SyncResult` | 同步结果 |
| `Accessibility` | 访问控制级别 |
| `Conversion` | 类型转换 trait |

#### SAF

| 类型 | 说明 |
|------|------|
| `VerifyTicketInfo` | Ticket 验证信息 |
| `CliInfo` | CLI 信息 |
| `PermissionQuery` | 权限查询 |
| `RemoteAuthPackage` | 远程认证包 |
| `RemoteUserAuthResults` | 远程用户认证结果 |
| `RemoteInfo` | 远程信息 |

### 错误处理宏

| 宏 | 定义位置 | 说明 |
|----|----------|------|
| `log_throw_error!` | `macros_lib.rs` | 记录日志并抛出错误 |
| `log_and_into_asset_error!` | `macros_lib.rs` | 记录日志并转换为 AssetError |
| `hisysevent!` | `macros_lib.rs` | HiSysEvent 上报 |

### 测试入口

| 测试 | 路径 |
|------|------|
| Module Test（使用 Definition 类型） | `test/asset/unittest/module_test/src/crypto_manager/lib.rs` |
| Inner Kits Rust 公共测试类型 | `test/asset/unittest/inner_kits_rust/src/common.rs` |

## 数据流

Definition 模块定义的类型在整个 IPC 链路中流转：

```
调用方构造 AssetMap（Tag→Value 映射）
  → asset_ipc 序列化为 IPC MessageParcel
  → SA stub.rs 反序列化回 AssetMap
  → operation_*.rs 读取 Tag 值执行业务逻辑
  → 结果构造为 AssetMap/ResultSet → 序列化返回
```

Tag 枚举与 C 侧 `Asset_Tag` 一一对应，新增 Tag 需同步更新 `asset_type.h`（NDK）、`asset_system_type.h`（System API）和 Rust `lib.rs`（Definition）。ErrCode 枚举与 C 侧 `Asset_ResultCode` 对应，用于 IPC 错误传递。

## 开发要点

- Tag 枚举新增值时必须同步更新 NDK asset_type.h 和 System API asset_system_type.h
- 错误码新增后需更新 hisysevent.yaml 中的事件定义
- AssetMap 是核心传递类型，新增字段需考虑 IPC 序列化兼容性

## 常见开发场景

- 新增资产标签时，在此查找 Tag 枚举定义和相关的类型映射
- 添加错误码时，定位 ErrCode 枚举和对应的 hisysevent 上报宏
- 修改 AssetMap 结构时，评估对 IPC 序列化和上下游模块的影响

## 相关主题

- [IPC 接口定义](../api/ipc-interfaces.md)
- [IPC 序列化模块](./ipc.md)
- [Asset 服务架构](../architecture/asset-service.md)
