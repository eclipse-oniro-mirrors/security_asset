# 隐私计算错误码参考 Context

> 文档版本：v1.0
> 更新时间：2026-11-16
> 来源：`privacy_protocol/include/privacy_error_code.h`、`frameworks/privacy_computation/js/napi/inc/privacy_computation_napi_common.h`

## 定位

隐私计算有两套错误码：协议码（C++/Rust/服务侧内部传递，0~9）与 NAPI 码（JS 侧可见，24000001~24000018）。两层之间的映射在 NAPI 公共层完成。注意与 Asset 侧 `ErrCode`、SAF 侧 `SafResultCode` 区分，三套不混用。

## 快速路由

### 错误码定义入口

| 层 | 稳定路径 | 说明 |
|----|----------|------|
| 协议码 | `privacy_protocol/include/privacy_error_code.h` | `PRIVACY_PROTOCOL_*`，0~9 |
| NAPI 码 | `frameworks/privacy_computation/js/napi/inc/privacy_computation_napi_common.h` | `PRIVACY_COMPUTATION_*`，24000001~24000018 |
| 协议码→NAPI 码映射 | `frameworks/privacy_computation/js/napi/src/privacy_computation_napi_common.cpp` | `MapProtocolError` |
| JS 错误消息 | 同上 | `GetErrorMessage` |

## NAPI 错误码表

| 码 | 常量 | 语义 |
|----|------|------|
| 24000001 | `PRIVACY_COMPUTATION_SERVICE_ERROR` | 服务错误（协议码兜底映射） |
| 24000006 | `PRIVACY_COMPUTATION_OUT_OF_MEMORY` | 内存分配失败 |
| 24000007 | `PRIVACY_COMPUTATION_DATA_CORRUPTED` | 数据格式/编解码错误 |
| 24000009 | `PRIVACY_COMPUTATION_CRYPTO_ERROR` | 密码学运算失败 |
| 24000012 | `PRIVACY_COMPUTATION_ACCOUNT_ERROR` | OS 账户身份获取失败 |
| 24000017 | `PRIVACY_COMPUTATION_UNAVAILABLE` | 能力不可用（开关关闭 / 闭源库未配置 / 协议或 HE 参数不支持） |
| 24000018 | `PRIVACY_COMPUTATION_INVALID_ARGUMENT` | 参数非法 |

## 协议码 → NAPI 码映射

| 协议码 | NAPI 码 |
|--------|---------|
| SUCCESS(0) | 0 |
| INVALID_ARGUMENT(1) | 24000018 |
| UNSUPPORTED_PROTOCOL(2) / INCOMPATIBLE_HE_PARAMETERS(3) / HE_ERROR(7) | 24000017 |
| CODEC_ERROR(4) | 24000007 |
| HASH_ERROR(5) / RANDOM_ERROR(6) / CRYPTO_ERROR(8) | 24000009 |
| 其他（INTERNAL_ERROR 等） | 24000001 |

## 常见问题定位

| 问题 | 优先查看 |
|------|----------|
| 24000017 出现场景 | 开关 `asset_privacy_computation_supported=false`；闭源库加载失败（loader `dlsym` 日志）；stub 模式 FFI 返回 UNSUPPORTED(2) |
| 24000007 | 编解码路径（`privacy_client.cpp` 长度校验、SA 侧 capacity 校验） |
| 24000009 | `privacy_protocol/crypto/` provider 实现日志 |
| 24000012 | 服务侧 `PrepareIdentity` 的 OS account 解析 |

## 开发要点

- 新增错误码需改协议头或 NAPI 公共头两处之一，并同步 `MapProtocolError`/`GetErrorMessage`；240000xx 号段与 Asset 侧共用大段，注意不与既有码冲突
- Rust FFI 层只透传协议码（`interfaces/inner_kits/c/asset/src/privacy.rs` 的 `map_privacy_error` 把 SDK `ErrCode` 折叠为协议码 1/2/9）
- 日志只打错误码与长度，不打密文/明文/种子内容

## 相关主题

- [隐私计算架构](architecture/overview.md)
- [NAPI 绑定模块](frameworks/napi-bindings.md)
- [Asset 错误码参考](../asset/error-codes.md)
- [SAF 错误码参考](../secure_access_fence/error-codes.md)
