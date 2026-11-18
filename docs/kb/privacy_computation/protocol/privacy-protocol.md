# 隐私计算协议库 Context

> 文档版本：v1.0
> 更新时间：2026-11-16
> 来源：`privacy_protocol/`（仓库顶层目录）

## 定位

纯 C++ 静态库（目标 `privacy_protocol`），实现 PSI/PIR 协议本体：目标编码、HE 加解密、B 侧求值、结果匹配。公开面只有 `include/` 下三个头（四个协议函数 + 类型 + 协议错误码）；哈希/随机/HE/PIR 值密码学通过 `internal::` provider 注册点可替换，不对业务调用方暴露。

## 快速路由

### 公开 API 入口

| 关注点 | 稳定路径 | 说明 |
|--------|----------|------|
| 协议函数 | `privacy_protocol/include/privacy_protocol.h` | `GenPrivacyTarget`、`PrivacySearch`、`DecryptCiphertext`、`PostProcessSearchResult` |
| 公开类型 | `privacy_protocol/include/privacy_protocol_types.h` | `PrivacyProtocol`、`CallerIdentity`、`TargetInput`、`Element`、`PrivacySearchResult`、`SearchResult`、`ProtocolType/HashAlg/DataSetSize` |
| 协议错误码 | `privacy_protocol/include/privacy_error_code.h` | 0~9 协议码 |

### 内部模块（不装进公开头）

| 子模块 | 稳定路径 | 说明 |
|--------|----------|------|
| 协议实现 | `privacy_protocol/core/` | `PrivacyProtocolImpl`（组合 codec + encoder） |
| 参数与编解码 | `privacy_protocol/protocol/` | `protocol_params.h`（BGV2048_48 参数族）、`protocol_codec.h`、`protocol_encoder.h`、`protocol_mapper.h` |
| HE 栈 | `privacy_protocol/he/` | `hifhe_adapter.h`（封装外部 hifhe）、`soft_he_engine.h`（`IHEEngine` 默认实现）、`he_key_deriver.h`（`IHeSeedDeriver` 服务侧扩展点）、`he_types.h`（`HeSeed` 析构即清零） |
| OpenSSL provider | `privacy_protocol/crypto/` | 内置哈希/随机/PIR 值加解密实现 |
| provider 注册点 | `privacy_protocol/common/` | `provider_registry.h`（`SetHashProvider`/`SetRandomSource`/`SetHEEngine`/`InstallDefaultHEEngineFactory`/`SetPirValueCrypto`）、`internal_types.h`、`protocol_evaluation.h` |

### 外部依赖入口

| 依赖方向 | 构建目标 | 说明 |
|----------|----------|------|
| 同态加密 | `hifhe:libhifhe` | `HifheAdapter` 封装 `HIFHE_Ctx`，句柄不跨 IPC/公开 API |
| 密码学 | `openhitls`、`openssl:libcrypto_shared` | provider 内置实现 |
| 安全函数 | `bounds_checking_function:libsec_shared` | |

### 测试入口

| 测试 | 稳定路径 |
|------|----------|
| 协议/编解码/客户端 | `test/asset/unittest/privacy_protocol/` |
| HE 引擎/hifhe 适配 | `test/asset/unittest/privacy_he/` |
| NAPI 层协议联动（含 hifhe mock） | `test/asset/unittest/privacy_coverage/` |

## 安全边界要点

- `HeSeed` 等秘密对象析构即清零，禁止拷贝逃逸出操作范围
- HE 句柄（`HifheText`/`HIFHE_HEText`）为操作局部对象，禁止经 IPC 或公开 API 传递；序列化仅限完整操作的密文边界
- 根密钥材料不出现在协议 API 或 IPC 中；种子派生经 `IHeSeedDeriver` 服务侧注入（见[服务侧模块](../services/privacy-service.md)）

## 常见问题定位

| 问题 | 优先查看 |
|------|----------|
| `PRIVACY_PROTOCOL_INCOMPATIBLE_HE_PARAMETERS` | `protocol_mapper.h` 的协议映射与 HE 引擎 `IsCompatible` 校验 |
| `PRIVACY_PROTOCOL_HE_ERROR` | `he/` 的 SoftHeEngine/HifheAdapter 日志 |
| 换实现/接入闭源 HE | `common/inc/provider_registry.h` 注册点（internal 命名空间，仅集成方使用） |
| PIR 附值加解密 | `crypto/` 的 `IPirValueCrypto` 与 `PostProcessSearchResult` |

## 开发要点

- 业务调用方只用 `include/` 的四个函数；内部头（common/crypto/protocol/core/he）仅对 `privacy_protocol` 自身与其单测可见
- 新协议类型（如新 FHE 参数族）从 `protocol_params.h` + `protocol_mapper.h` 入手
- 单测覆盖在 `test/asset/unittest/privacy_protocol/` 与 `privacy_he/`，协议行为改动需配套

## 相关主题

- [隐私计算架构](../architecture/overview.md)
- [服务侧模块](../services/privacy-service.md)
- [FFI/IPC 客户端](../frameworks/ipc-client.md)
- [隐私计算错误码参考](../error-codes.md)
