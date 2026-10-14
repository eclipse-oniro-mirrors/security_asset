# NAPI 绑定模块 Context

> 文档版本：v1.0
> 更新时间：2026-10-13
> 来源：`frameworks/asset/js/napi/`、`frameworks/secure_access_fence/js/napi/`、`frameworks/common/napi/`、`frameworks/hpic/js/napi/`

## 定位

NAPI 绑定层将 Rust/C++ 后端能力暴露为 JavaScript API，供应用通过 `@ohos.asset` 等 NAPI 模块调用。

## 快速路由

### 源码入口

### Asset NAPI

| 文件 | 说明 |
|------|------|
| `frameworks/asset/js/napi/src/asset_napi.cpp` | NAPI 模块初始化和注册 |
| `frameworks/asset/js/napi/src/asset_napi_add.cpp` | Add 绑定 |
| `frameworks/asset/js/napi/src/asset_napi_remove.cpp` | Remove 绑定 |
| `frameworks/asset/js/napi/src/asset_napi_update.cpp` | Update 绑定 |
| `frameworks/asset/js/napi/src/asset_napi_query.cpp` | Query 绑定 |
| `frameworks/asset/js/napi/src/asset_napi_query_sync_result.cpp` | QuerySyncResult 绑定 |
| `frameworks/asset/js/napi/src/asset_napi_pre_query.cpp` | PreQuery 绑定 |
| `frameworks/asset/js/napi/src/asset_napi_post_query.cpp` | PostQuery 绑定 |
| `frameworks/asset/js/napi/src/asset_napi_context.cpp` | NAPI 上下文/异步工作 |
| `frameworks/asset/js/napi/src/asset_napi_common.cpp` | NAPI 公共工具 |
| `frameworks/asset/js/napi/src/asset_napi_check.cpp` | 参数校验 |
| `frameworks/asset/js/napi/BUILD.gn` | 构建目标 `libasset_napi` |

头文件目录：`frameworks/asset/js/napi/inc/`（含上述各模块的 .h 文件 + `asset_napi_error_code.h`）

### SAF NAPI

| 文件 | 说明 |
|------|------|
| `frameworks/secure_access_fence/js/napi/src/agent_fence_napi.cpp` | Agent Fence NAPI 绑定 |
| `frameworks/secure_access_fence/js/napi/BUILD.gn` | 构建目标 `libabilitytoolaccessctrl_napi` |

头文件目录：`frameworks/secure_access_fence/js/napi/inc/`（`agent_fence_napi_context.h`、`agent_fence_error_codes.h`）

### 公共 NAPI

| 文件 | 说明 |
|------|------|
| `frameworks/common/napi/src/napi_common.cpp` | 共享 NAPI 工具函数 |
| `frameworks/common/napi/inc/napi_common.h` | 公共头文件 |

### 隐私计算 NAPI

| 文件 | 说明 |
|------|------|
| `frameworks/hpic/js/napi/src/hpic_napi.cpp` | 隐私计算 NAPI 绑定 |
| `frameworks/hpic/js/napi/inc/hpic_log.h` | 日志头文件 |
| `frameworks/hpic/js/napi/BUILD.gn` | 构建目标 `libprivacycomputation_napi` |

### 符号导出

| 文件 | 说明 |
|------|------|
| `libasset_napi.map` | Asset NAPI 符号映射 |
| `libabilitytoolaccessctrl_napi.map` | SAF NAPI 符号映射 |

### 测试入口

| 测试 | 路径 |
|------|------|
| Asset NDK 测试（验证 NAPI 绑定） | `test/asset/unittest/kits_ndk/` |
| SAF NDK 测试（验证 NAPI 绑定） | `test/secure_access_fence/unittest/kits_ndk/` |

## 数据流

NAPI 绑定层的调用链路：

```
JS 应用 → @ohos.asset 模块 → asset_napi_*.cpp
  → asset_napi_context.cpp (异步工作) → asset_napi_check.cpp (参数校验)
  → Rust FFI / C System API → IPC → AssetService (SA 8100)
  → 异步回调返回 JS
```

NAPI 层负责 JS 参数到 C/Rust 类型的转换、异步工作线程管理和回调返回。参数校验在 `asset_napi_check.cpp` 中集中处理，校验不通过直接返回 JS 错误对象，不发起 IPC。

## 开发要点

- NAPI 参数校验在 asset_napi_check.cpp 中集中处理，新增 API 需补充校验逻辑
- 异步操作通过 asset_napi_context.cpp 管理，注意 HandleScope 生命周期
- 新增 NAPI 方法需在 asset_napi.cpp 模块注册中添加

## 常见开发场景

- 新增 JS API 绑定时，在此查找 NAPI 模块注册和参数校验模式
- 排查 NAPI 异步回调泄漏时，定位 asset_napi_context.cpp 的上下文管理
- 修改 NAPI 导出符号时，确认 .map 文件和 BUILD.gn 配置一致

## 相关主题

- [公共 NDK API](../api/ndk-api.md)
- [SAF Agent Fence API](../api/saf-agent-fence.md)
- [Asset 服务架构](../architecture/asset-service.md)
