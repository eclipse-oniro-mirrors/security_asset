# SAF NAPI 绑定模块 Context

> 文档版本：v1.0
> 更新时间：2026-10-13
> 来源：`frameworks/secure_access_fence/js/napi/`、`frameworks/common/napi/`

## 定位

SAF NAPI 绑定层将 C++ 后端的 SAF Agent Fence 能力暴露为 JavaScript API，供应用通过 `@ohos.abilityToolAccessCtrl` NAPI 模块调用。

## 快速路由

### 源码入口

| 文件 | 说明 |
|------|------|
| `frameworks/secure_access_fence/js/napi/src/agent_fence_napi.cpp` | Agent Fence NAPI 绑定 |
| `frameworks/secure_access_fence/js/napi/BUILD.gn` | 构建目标 `libabilitytoolaccessctrl_napi` |

头文件目录：`frameworks/secure_access_fence/js/napi/inc/`（`agent_fence_napi_context.h`、`agent_fence_error_codes.h`）

### 公共 NAPI（共享依赖）

| 文件 | 说明 |
|------|------|
| `frameworks/common/napi/src/napi_common.cpp` | 共享 NAPI 工具函数 |
| `frameworks/common/napi/inc/napi_common.h` | 公共头文件 |

公共 NAPI 工具（`napi_common.cpp/h`）由 Asset/SAF/HPIC NAPI 模块共享，提供通用的 JS 值转换和异步工作封装。

### 符号导出

| 文件 | 说明 |
|------|------|
| `libabilitytoolaccessctrl_napi.map` | SAF NAPI 符号映射 |

### 测试入口

| 测试 | 路径 |
|------|------|
| SAF NDK 测试（验证 NAPI 绑定） | `test/secure_access_fence/unittest/kits_ndk/` |

## 数据流

NAPI 绑定层的 SAF 调用链路：

```
JS 应用 → @ohos.abilityToolAccessCtrl 模块 → agent_fence_napi.cpp
  → agent_fence_napi_context.h (异步上下文)
  → IPC → SAFService (SA 66532)
  → 异步回调返回 JS
```

SAF NAPI 层负责 JS 参数到 C++ 类型的转换、异步工作线程管理和回调返回。错误码映射在 `agent_fence_error_codes.h` 中。

## 开发要点

- SAF NAPI 参数校验和错误码映射在 agent_fence_error_codes.h 中，新增 API 需补充
- 异步操作通过 agent_fence_napi_context.h 管理，注意 HandleScope 生命周期
- 新增 NAPI 方法需在 agent_fence_napi.cpp 模块注册中添加

## 常见开发场景

- 新增 SAF JS API 绑定时，在此查找 NAPI 模块注册和错误码映射模式
- 排查 SAF NAPI 异步回调泄漏时，定位 agent_fence_napi_context.h 的上下文管理
- 修改 SAF NAPI 导出符号时，确认 .map 文件和 BUILD.gn 配置一致

## 相关主题

- [SAF Agent Fence API](../api/agent-fence-api.md)
- [安全访问围栏架构](../architecture/service.md)
