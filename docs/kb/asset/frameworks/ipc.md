# Asset IPC 序列化模块 Context

> 文档版本：v1.0
> 更新时间：2026-10-13
> 来源：`frameworks/asset/ipc/`

## 定位

Asset IPC 模块负责 AssetMap、批量结果、同步结果等数据的 IPC 序列化/反序列化，以及 IPC Code 常量和错误处理。详见 [IPC 接口定义](../api/ipc.md)。

## 快速路由

### 源码入口

| 文件 | 说明 |
|------|------|
| `frameworks/asset/ipc/src/lib.rs` | IpcCode 枚举、SA_ID=8100、SA_NAME="security_asset_service" |
| `frameworks/asset/ipc/BUILD.gn` | 构建配置（crate: asset_ipc） |

序列化函数：`serialize_map`、`deserialize_map`、`serialize_maps`、`deserialize_maps`、`serialize_sync_result`、`serialize_batch_result`、`ipc_err_handle`

### 测试入口

| 测试 | 路径 |
|------|------|
| Asset Stub UT | `test/asset/unittest/ut_test/services/core_service/test_stub/ut_core_service_lib_stub.rs` |

## 数据流

IPC 序列化模块在 Asset 调用链中的位置：

```
调用方构造 AssetMap → asset_ipc::serialize_map → IPC 传输
  → SA stub.rs → asset_ipc::deserialize_map → 还原为 AssetMap
  → operation_*.rs 处理 → 结果通过 serialize_map/serialize_maps 返回
```

Asset IPC 使用 Rust 枚举 IpcCode 标识操作类型（Add/Remove/Update/Query 等），序列化函数将 AssetMap 转换为 IPC 兼容格式。

## 开发要点

- IpcCode 枚举值必须与 stub.rs 中的 dispatch 顺序严格对应
- 新增 IPC 方法时需同步更新序列化函数和 stub 分发
- 序列化函数集中在 `frameworks/asset/ipc/src/lib.rs`，新增序列化需考虑前后兼容

## 常见开发场景

- 新增 IPC 序列化函数时，在此查找现有的 serialize/deserialize 模式
- 排查 IPC 通信问题时，定位 SA_ID 常量和序列化入口
- 修改 AssetMap 结构后，确认序列化/反序列化函数的兼容性

## 相关主题

- [IPC 接口定义](../api/ipc.md)
- [Definition 模块](./definition.md)
- [Asset 服务架构](../architecture/service.md)
