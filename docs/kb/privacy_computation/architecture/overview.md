# 隐私计算架构 Context

> 文档版本：v1.0
> 更新时间：2026-11-16
> 来源：PR 530（先知二期）合入后的代码结构分析

## 定位

隐私计算提供 PSI（隐私集合求交）与 PIR（隐私信息检索）两类协议。业务形态为四步北向流程：`genPrivacyTarget`（A 侧生成加密目标）→ `privacySearch`（B 侧本地 HE 求值，不跨进程）→ `getSearchResult`（逐批经 SA 解密 HE 密文 + 本地后处理匹配）。代码分四层：JS/NAPI 绑定、FFI/IPC 客户端、asset_service 服务侧、顶层 `privacy_protocol/` 协议库。构建期由两个部件开关控制路由与能力裁剪。行为事实以源码与测试为准。

## 模块地图

| 模块 | 稳定路径 | 职责 |
|------|----------|------|
| JS/NAPI 绑定层 | `frameworks/privacy_computation/js/napi/` | 模块 `security.privacyComputation`，三个 JS 接口 |
| FFI/IPC 客户端 | `frameworks/privacy_computation/ipc/` | C++ 协议类型 → C 视图，经 Rust SDK FFI 走 Asset IPC |
| Rust SDK FFI | `interfaces/inner_kits/c/asset/src/privacy.rs` | `asset_privacy_*` C 符号，full/empty 双模式 |
| IPC 序列化与 IPC 码 | `frameworks/asset/ipc/src/privacy.rs` | 请求/回复 Parcel 编解码，`IpcCode::PrivacyGenTarget=101`、`PrivacyDecryptCiphertext=102` |
| 服务侧实现 | `services/asset/privacy/` | asset_service 进程内 C++ 实现（目标 `asset_privacy`） |
| SA 侧 Rust 分发 | `services/asset/core_service/src/privacy.rs` | Stub 收 IPC，按 Binder UID 调 C ABI |
| 协议库 | `privacy_protocol/`（仓库顶层） | 四个公开 API、HE/编码/密码学（目标 `privacy_protocol`） |

## 路由模式（二选一，构建期决定）

| 路由 | 触发条件 | 链路 |
|------|----------|------|
| ComputationLib | `asset_enable_privacy_computation_config` 非空 | NAPI 入口直接 `dlopen` 闭源计算库分发符号 |
| AssetSA（默认） | 闭源库路径为空 | NAPI → privacy_client（C++）→ 同进程 FFI → Rust SDK → Asset IPC → asset_service → privacy_service → privacy_protocol |

`privacySearch` 恒为本地路径：NAPI 直接调协议库 `PrivacySearch`，不经 IPC。

## 数据流（AssetSA 路由）

```
JS 应用
  → privacy_computation_napi.cpp（security.privacyComputation 模块）
  → [genPrivacyTarget] privacy_client.cpp → asset_privacy_gen_target（Rust FFI）
      → Asset IPC（IpcCode 101）→ core_service/src/privacy.rs → AssetGenPrivacyTarget（C++）
      → privacy_protocol::GenPrivacyTarget（编码 + 种子派生 + HE 加密）
  → [privacySearch] 本地 privacy_protocol::PrivacySearch（B 侧 HE 求值）
  → [getSearchResult] 逐批 asset_privacy_decrypt_ciphertext（IpcCode 102，SA 侧 HUKS 种子解密）
      → 本地 privacy_protocol::PostProcessSearchResult（匹配 + PIR 附值解密）
  → 异步回调返回 JS
```

## 构建开关

| 配置项 | 默认值 | 说明 |
|--------|--------|------|
| `asset_enable_privacy_computation_config` | `""` | 闭源计算库路径；非空启用 ComputationLib 路由 |
| `asset_privacy_computation_supported` | `true` | 能力开关；关闭时三个 JS 接口直接 Reject（24000017） |

## 跨进程边界要点

- FFI 的 C 视图（`privacy_types.h`）只跨同进程 C/Rust 边界，指针不写入 Parcel；IPC 序列化在 `frameworks/asset/ipc/src/privacy.rs`。
- SA 侧每个单批操作独立捕获 Binder UID，作为协议身份（uid→OS account 解析在服务侧完成）。
- 长度上限常量在 `privacy_limits.h` 与 `asset_ipc::privacy`：目标回复 192KiB；解密明文固定为 BGV-2048 槽位（2048×uint64）。

## 常见问题定位

| 问题 | 优先查看 |
|------|----------|
| 接口 Reject 24000017 | [NAPI 绑定模块](../frameworks/napi-bindings.md) 的开关与 loader 说明 |
| IPC 链路断在哪个环节 | [FFI/IPC 客户端](../frameworks/ipc-client.md) 的逐段日志关键字 |
| SA 侧解密/生成失败 | [服务侧模块](../services/privacy-service.md) 的身份与 HUKS 根密钥检查点 |
| 协议/HE 算法问题 | [隐私计算协议库](../protocol/privacy-protocol.md) |

## 测试入口

| 测试 | 稳定路径 |
|------|----------|
| NAPI 覆盖（含 mock 平台） | `test/asset/unittest/privacy_coverage/` |
| 协议/编码/客户端 | `test/asset/unittest/privacy_protocol/` |
| HE 引擎与 hifhe 适配 | `test/asset/unittest/privacy_he/` |
| IPC 序列化（Rust） | `test/asset/unittest/privacy_ipc/` |
| HUKS 种子派生 | `test/asset/unittest/privacy_root/` |

## 相关主题

- [NAPI 绑定模块](../frameworks/napi-bindings.md)
- [FFI/IPC 客户端](../frameworks/ipc-client.md)
- [隐私计算协议库](../protocol/privacy-protocol.md)
- [服务侧模块](../services/privacy-service.md)
- [隐私计算错误码参考](../error-codes.md)
- [整体架构概览](../../shared/overview.md)
