# 隐私计算 NAPI 绑定模块 Context

> 文档版本：v2.0
> 更新时间：2026-11-16
> 来源：`frameworks/privacy_computation/`、`privacy_protocol/`（原 `frameworks/hpic/`，模块已更名迁移）

## 定位

隐私计算（privacy computation，旧称 HPIC）JS 绑定层，NAPI 模块名 `security.privacyComputation`，构建目标 `privacycomputation_napi`。对外提供 `genPrivacyTarget`、`privacySearch`、`getSearchResult` 三个 JS 接口和 `ProtocolType`（PSI/PIR）、`DataSetSize`、`HashAlg` 枚举。运行时按构建开关路由：配置闭源库路径时经 loader `dlopen` 分发；未配置时 `genPrivacyTarget`/`getSearchResult` 走 AssetSA（SA 8100）IPC，`privacySearch` 走本地协议库。行为事实以源码与测试为准。

## 快速路由

### 源码入口

| 关注点 | 稳定路径 | 说明 |
|--------|----------|------|
| NAPI 注册与三个接口 | `frameworks/privacy_computation/js/napi/src/privacy_computation_napi.cpp` | 模块注册、`Register()` 导出表 |
| NAPI 公共设施 | `frameworks/privacy_computation/js/napi/src/privacy_computation_napi_common.cpp` | 错误码、异步工作、JS 值转换 |
| 闭源库加载 | `frameworks/privacy_computation/js/napi/src/privacy_computation_napi_loader.cpp` | `dlopen` 闭源计算库与符号分发 |
| 日志 | `frameworks/privacy_computation/js/napi/inc/privacy_computation_log.h` | HiLog 封装 |
| IPC/FFI 客户端 | `frameworks/privacy_computation/ipc/` | `privacy_client.cpp`（SA 路由）、`privacy_client_ffi.h` |
| 协议库 | `privacy_protocol/`（仓库顶层） | core/protocol/crypto/he/common 子模块，PSI/PIR 编解码与 HE |
| 服务侧后端 | `services/asset/privacy/` | `privacy_service.cpp`、`privacy_request.h`、`huks_seed_deriver` |
| 构建 | `frameworks/privacy_computation/js/napi/BUILD.gn` | 目标 `privacycomputation_napi` |

### 构建开关

| 配置项 | 默认值 | 说明 |
|--------|--------|------|
| `asset_enable_privacy_computation_config` | `""` | 闭源计算库路径；非空时注入 `ASSET_ENABLE_PRIVACY_COMPUTATION_CONFIG` 并启用 loader 分发 |
| `asset_privacy_computation_supported` | `true` | 隐私计算能力开关；关闭时三个 JS 接口直接 Reject（24000017） |

### 测试入口

| 测试 | 稳定路径 |
|------|----------|
| NAPI 覆盖测试 | `test/asset/unittest/privacy_coverage/` |
| 协议/HE/IPC 测试 | `test/asset/unittest/privacy_protocol/`、`test/asset/unittest/privacy_he/`、`test/asset/unittest/privacy_ipc/` |
| root 场景测试 | `test/asset/unittest/privacy_root/` |

## 数据流

```
JS 应用 → security.privacyComputation 模块 → privacy_computation_napi.cpp
  ├─ 闭源库已配置：privacy_computation_napi_loader.cpp（dlopen）→ 闭源计算库
  ├─ genPrivacyTarget / getSearchResult：privacy_client（IPC/FFI）
  │   → services/asset/privacy（asset_service 侧）→ privacy_protocol（HE 解密、编解码）
  └─ privacySearch：本地 privacy_protocol 处理
  → 异步回调返回 JS
```

## 常见问题定位

| 问题 | 优先查看 |
|------|----------|
| 接口 Reject 返回 24000017 | `asset_privacy_computation_supported` 被关闭，或闭源库未配置/加载失败（loader） |
| 闭源库符号分发失败 | `privacy_computation_napi_loader.cpp` 的 `dlsym` 错误日志 |
| 错误码语义与 JS 消息 | `privacy_computation_napi_common.h`、`privacy_computation_napi_common.cpp` |

## 开发要点

- 新增 JS API 在 `privacy_computation_napi.cpp` 的 `Register()` 导出表添加，异步生命周期参考 `PrivacyComputationBaseContext`（parse/execute/resolve 模式）
- 错误码与 JS 错误消息映射集中在 `privacy_computation_napi_common.cpp`
- 调整构建开关需同步三处：`config.gni` 声明、`bundle.json` features 注册、目标 BUILD.gn 宏注入

## 相关主题

- [隐私计算架构](../architecture/overview.md)
- [FFI/IPC 客户端](ipc-client.md)
- [隐私计算协议库](../protocol/privacy-protocol.md)
- [隐私计算错误码参考](../error-codes.md)
- [整体架构概览](../../shared/overview.md)
- [Asset 服务架构](../../asset/architecture/service.md)
