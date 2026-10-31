# HPIC NAPI 绑定模块 Context

> 文档版本：v1.0
> 更新时间：2026-10-13
> 来源：`frameworks/hpic/js/napi/`

## 定位

HPIC（隐私计算）NAPI 绑定层将隐私计算能力暴露为 JavaScript API，供应用通过 `@ohos.privacyComputation` NAPI 模块调用。

## 快速路由

### 源码入口

| 文件 | 说明 |
|------|------|
| `frameworks/hpic/js/napi/src/hpic_napi.cpp` | 隐私计算 NAPI 绑定 |
| `frameworks/hpic/js/napi/inc/hpic_log.h` | 日志头文件 |
| `frameworks/hpic/js/napi/BUILD.gn` | 构建目标 `libprivacycomputation_napi` |

### 公共 NAPI（共享依赖）

| 文件 | 说明 |
|------|------|
| `frameworks/common/napi/src/napi_common.cpp` | 共享 NAPI 工具函数 |
| `frameworks/common/napi/inc/napi_common.h` | 公共头文件 |

公共 NAPI 工具（`napi_common.cpp/h`）由 Asset/SAF/HPIC NAPI 模块共享，提供通用的 JS 值转换和异步工作封装。

## 数据流

NAPI 绑定层的 HPIC 调用链路：

```
JS 应用 → @ohos.privacyComputation 模块 → hpic_napi.cpp
  → 隐私计算能力
  → 异步回调返回 JS
```

HPIC NAPI 层负责将隐私计算能力暴露为 JavaScript API，处理 JS 参数转换和异步回调返回。日志通过 `hpic_log.h` 定义的头文件统一管理。

## 开发要点

- HPIC NAPI 模块通过 hpic_napi.cpp 注册，新增 API 需在此添加
- 日志统一通过 hpic_log.h 定义的头文件
- 构建目标 libprivacycomputation_napi，修改导出符号需同步 BUILD.gn

## 常见开发场景

- 新增隐私计算 JS API 绑定时，在此查找 hpic_napi.cpp 模块注册模式
- 排查 HPIC 日志输出时，定位 hpic_log.h 的日志定义
- 修改 HPIC NAPI 导出符号时，确认 BUILD.gn 配置

## 相关主题

- [整体架构概览](../shared/overview.md)
