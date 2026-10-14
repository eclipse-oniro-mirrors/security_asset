# IPC 序列化模块 Context

> 文档版本：v1.0
> 更新时间：2026-10-13
> 来源：`frameworks/asset/ipc/`、`frameworks/secure_access_fence/inner_api/ipc/`

## 定位

IPC 模块负责 AssetMap、批量结果、同步结果等数据的 IPC 序列化/反序列化，以及 IPC Code 常量和错误处理。详见 [IPC 接口定义](../api/ipc-interfaces.md)。

## 快速路由

### 源码入口

### Asset IPC

| 文件 | 说明 |
|------|------|
| `frameworks/asset/ipc/src/lib.rs` | IpcCode 枚举、SA_ID=8100、SA_NAME="security_asset_service" |
| `frameworks/asset/ipc/BUILD.gn` | 构建配置（crate: asset_ipc） |

序列化函数：`serialize_map`、`deserialize_map`、`serialize_maps`、`deserialize_maps`、`serialize_sync_result`、`serialize_batch_result`、`ipc_err_handle`

### SAF IPC

| 文件 | 说明 |
|------|------|
| `frameworks/secure_access_fence/inner_api/ipc/ISecureAccessFence.idl` | IDL 接口定义 |
| `frameworks/secure_access_fence/inner_api/ipc/SecureAccessFenceType.idl` | IDL 类型定义 |
| `frameworks/secure_access_fence/inner_api/ipc/src/lib.rs` | SA_ID 常量 |
| `frameworks/secure_access_fence/inner_api/ipc/src/remote_message_wrapper.rs` | 远程消息包装 |
| `frameworks/secure_access_fence/inner_api/ipc/src/sub_modules/mod.rs` | 子模块入口 |
| `frameworks/secure_access_fence/inner_api/ipc/src/sub_modules/ticket.rs` | Ticket 序列化 |
| `frameworks/secure_access_fence/inner_api/ipc/src/sub_modules/remote_control.rs` | 远程控制序列化 |
| `frameworks/secure_access_fence/inner_api/ipc/BUILD.gn` | 构建配置（crate: saf_ipc） |

### 测试入口

| 测试 | 路径 |
|------|------|
| Asset Stub UT | `test/asset/unittest/ut_test/services/core_service/test_stub/ut_core_service_lib_stub.rs` |
| SAF Stub UT | `test/secure_access_fence/unittest/ut_test/services/core_service/test_stub/ut_core_service_lib_stub.rs` |

## 数据流

IPC 序列化模块在调用链中的位置：

```
调用方构造 AssetMap → asset_ipc::serialize_map → IPC 传输
  → SA stub.rs → asset_ipc::deserialize_map → 还原为 AssetMap
  → operation_*.rs 处理 → 结果通过 serialize_map/serialize_maps 返回
```

Asset IPC 使用 Rust 枚举 IpcCode 标识操作类型（Add/Remove/Update/Query 等），序列化函数将 AssetMap 转换为 IPC 兼容格式。SAF IPC 使用 IDL 文件定义接口和类型，通过 codegen 生成序列化代码。

## 开发要点

- IpcCode 枚举值必须与 stub.rs 中的 dispatch 顺序严格对应
- 新增 IPC 方法时需同步更新序列化函数和 stub 分发
- SAF 使用 IDL 文件定义接口，修改 IDL 后需重新生成 IPC 代码

## 常见开发场景

- 新增 IPC 序列化函数时，在此查找现有的 serialize/deserialize 模式
- 排查 IPC 通信问题时，定位 SA_ID 常量和序列化入口
- 修改 SAF IDL 类型定义后，确认对应的 Rust 序列化实现

## 相关主题

- [IPC 接口定义](../api/ipc-interfaces.md)
- [Definition 模块](./definition.md)
- [Asset 服务架构](../architecture/asset-service.md)
- [安全访问围栏架构](../architecture/secure-access-fence.md)
