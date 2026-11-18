# 隐私计算 FFI/IPC 客户端 Context

> 文档版本：v1.0
> 更新时间：2026-11-16
> 来源：`frameworks/privacy_computation/ipc/`、`interfaces/inner_kits/c/asset/src/privacy.rs`

## 定位

应用侧桥接层：把 C++ 协议类型（`privacy_protocol` 的 `TargetInput`/`PrivacySearchResult`）转成 C 视图结构，经同进程 Rust SDK FFI 走 Asset IPC 到 SA（SA 8100）。`privacy_client` 本身不做加解密，只做视图转换、长度校验与批处理编排（`getSearchResult` 逐批解密后本地调用 `PostProcessSearchResult`）。

## 快速路由

### 源码入口

| 关注点 | 稳定路径 | 说明 |
|--------|----------|------|
| 客户端实现 | `frameworks/privacy_computation/ipc/src/privacy_client.cpp` | `GenPrivacyTargetRemote`、`GetSearchResultRemote` |
| C++ 侧接口 | `frameworks/privacy_computation/ipc/inc/privacy_client.h` | |
| FFI 声明（C ABI） | `frameworks/privacy_computation/ipc/inc/privacy_client_ffi.h` | `asset_privacy_gen_target`、`asset_privacy_decrypt_ciphertext` |
| C 视图类型 | `frameworks/privacy_computation/ipc/inc/privacy_types.h` | `PrivacyBlob/PrivacyProtocol/PrivacyTargetRequest/PrivacyDecryptRequest/PrivacyBuffer` |
| 长度上限 | `frameworks/privacy_computation/ipc/inc/privacy_limits.h` | 目标回复 192KiB；解密明文 2048×uint64 |
| 构建目标 | `frameworks/privacy_computation/ipc/BUILD.gn` | 静态库 `privacy_client`，依赖 `asset_sdk_ffi` |

### Rust SDK FFI（符号提供方）

| 关注点 | 稳定路径 | 说明 |
|--------|----------|------|
| FFI 实现 | `interfaces/inner_kits/c/asset/src/privacy.rs` | full 模式走 `Manager` 发 IPC；empty 模式（stub）直接返回 UNSUPPORTED(2) |
| 符号导出 | `interfaces/inner_kits/c/asset/libasset_sdk_ffi.map` | |

### IPC 序列化与 IPC 码

| 关注点 | 稳定路径 | 说明 |
|--------|----------|------|
| 序列化模块 | `frameworks/asset/ipc/src/privacy.rs` | 请求/回复 Parcel 编解码、长度常量 |
| IPC 码定义 | `frameworks/asset/ipc/src/lib.rs` | `PrivacyGenTarget=101`、`PrivacyDecryptCiphertext=102` |

### 服务侧对端

| 关注点 | 稳定路径 | 说明 |
|--------|----------|------|
| SA 分发 | `services/asset/core_service/src/privacy.rs` | 按 IPC 码反序列化，捕获 Binder UID 调 C ABI |
| C++ 实现 | `services/asset/privacy/` | 详见[服务侧模块](../services/privacy-service.md) |

## 数据流

```
privacy_client.cpp（C++）
  → PrivacyTargetRequest/PrivacyDecryptRequest（C 视图，指针仅同进程有效）
  → asset_privacy_gen_target / asset_privacy_decrypt_ciphertext（Rust FFI）
  → MsgParcel 序列化（asset_ipc::privacy）→ Asset IPC → SA
```

## 常见问题定位

| 问题 | 优先查看 |
|------|----------|
| 返回 UNSUPPORTED(2) | `asset_build_mode=stub` 时 FFI 为 empty 实现，不发 IPC |
| `PRIVACY_PROTOCOL_CODEC_ERROR` | `privacy_client.cpp` 的回复长度校验日志（回复超限/明文长度不符） |
| 批处理中断 | `GetSearchResultRemote` 逐批日志 `batch=N`，看断在哪一批 |
| IPC 序列化失败 | SA 侧 `PRIVACY_COMPUTATION Stub` 前缀日志（`core_service/src/privacy.rs`） |

## 开发要点

- FFI 视图指针**只跨同进程 C/Rust 边界**，不得写入 Parcel；跨进程一律走 `asset_ipc::privacy` 的值拷贝序列化
- 新增跨进程隐私请求需同步四处：FFI 声明/实现、`asset_ipc::privacy` 序列化、`IpcCode`、SA 侧分发
- 错误码在 C++/Rust/协议三侧均为协议码（0~9），到 NAPI 层才映射为 24000001~24000018

## 相关主题

- [隐私计算架构](../architecture/overview.md)
- [NAPI 绑定模块](napi-bindings.md)
- [服务侧模块](../services/privacy-service.md)
- [隐私计算错误码参考](../error-codes.md)
