# Asset IPC 接口定义 Context

> 文档版本：v1.0
> 更新时间：2026-10-13
> 来源：`frameworks/asset/ipc/`

## 定位

Asset 服务的 IPC 通信协议定义，使用 Rust 枚举定义 IPC Code，并包含 AssetMap、批量结果、同步结果等数据的序列化/反序列化函数。

## 快速路由

### 源码入口

| 文件 | 说明 |
|------|------|
| `frameworks/asset/ipc/src/lib.rs` | IpcCode 枚举、SA_ID=8100、序列化/反序列化 |
| `services/asset/core_service/src/stub.rs` | IPC Stub 实现（RemoteStub） |
| `frameworks/asset/ipc/BUILD.gn` | 构建配置 |

### Asset IPC Code

| 枚举值 | Code | 说明 |
|--------|------|------|
| Add | FIRST_CALL_TRANSACTION | 新增 |
| Remove | +1 | 删除 |
| Update | +2 | 更新 |
| PreQuery | +3 | 预查询 |
| Query | +4 | 查询 |
| PostQuery | +5 | 后查询 |
| QuerySyncResult | +6 | 同步结果查询 |
| BatchAdd | +7 | 批量新增 |
| BatchRemove | +8 | 批量删除 |
| BatchUpdate | +9 | 批量更新 |

### Asset IPC 序列化函数

| 函数 | 说明 |
|------|------|
| `serialize_map` / `deserialize_map` | 单个 AssetMap 序列化 |
| `serialize_maps` / `deserialize_maps` | 多个 AssetMap 序列化 |
| `serialize_sync_result` | 同步结果序列化 |
| `serialize_batch_result` | 批量结果序列化 |
| `ipc_err_handle` | IPC 错误处理 |

### 调用链路

```
Client → serialize_map → IPC → on_remote_request(stub.rs) → IpcCode 匹配 → operation_*.rs
```

### 测试入口

| 测试 | 路径 |
|------|------|
| Asset Stub UT | `test/asset/unittest/ut_test/services/core_service/test_stub/ut_core_service_lib_stub.rs` |

## 开发要点

- IpcCode 枚举值必须与 stub.rs 中的 dispatch 顺序严格对应
- 新增 IPC 方法时需同步更新序列化函数和 stub 分发
- 序列化函数集中在 `frameworks/asset/ipc/src/lib.rs`（crate: asset_ipc），详见 [IPC 序列化模块](../frameworks/ipc.md)

## 常见开发场景

- 新增一个 Asset 操作时，需要在此处查找 IpcCode 定义和对应的序列化函数
- 排查 IPC 调用返回错误码时，定位 stub.rs 中的分发逻辑
- 修改 AssetMap 结构后，确认序列化/反序列化函数的兼容性

## 相关主题

- [Asset 服务架构](../architecture/service.md)
- [Core Service 模块](../services/core-service.md)
- [公共 NDK API](./ndk-api.md)
- [Rust SDK](./rust-sdk.md)
- [IPC 序列化模块](../frameworks/ipc.md)
