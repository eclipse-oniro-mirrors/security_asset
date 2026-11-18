# 隐私计算服务侧模块 Context

> 文档版本：v1.0
> 更新时间：2026-11-16
> 来源：`services/asset/privacy/`（构建目标 `asset_privacy`）

## 定位

asset_service 进程内的隐私计算实现，以 C ABI（`AssetGenPrivacyTarget`/`AssetDecryptPrivacyCiphertext`）对 Rust Stub 暴露。职责：入参校验、调用者身份解析（uid→OS account）、HUKS 根密钥准备、调用协议库。不在本层做协议运算。

## 快速路由

### 源码入口

| 关注点 | 稳定路径 | 说明 |
|--------|----------|------|
| C ABI 实现 | `services/asset/privacy/src/privacy_service.cpp` | 两个 `extern "C"` 入口 |
| C ABI 声明 | `services/asset/privacy/inc/privacy_request.h` | 带 uid 参数的服务侧契约 |
| HUKS 种子派生 | `services/asset/privacy/src/huks_seed_deriver.cpp`、`inc/huks_seed_deriver.h` | `HuksSeedDeriver : internal::IHeSeedDeriver` |
| 构建目标 | `services/asset/privacy/BUILD.gn` | 静态库 `asset_privacy` |
| 上游分发 | `services/asset/core_service/src/privacy.rs` | Rust Stub 按 IPC 码调用本模块 C ABI |

### 外部依赖入口

| 依赖 | 构建目标 | 用途 |
|------|----------|------|
| HUKS | `huks:libhukssdk` | CE 命名空间根密钥派生 |
| OS 账户 | `os_account:os_account_innerkits` | uid → userId 解析 |
| IPC | `ipc:ipc_single` | Stub 侧依赖 |

### 测试入口

| 测试 | 稳定路径 |
|------|----------|
| HUKS 种子派生 | `test/asset/unittest/privacy_root/` |
| SA 边界（null 参数/超限/错误码路径） | `test/asset/unittest/privacy_coverage/` |

## 安全边界要点

- HE 种子根密钥固定别名存于调用者 OS account 的 **HUKS CE 命名空间**，根密钥字节不出 HUKS；派生输入包含完整调用者身份的 materialHash
- 根密钥策略不对称：`genPrivacyTarget` 允许 `CREATE_IF_MISSING` 创建；`decryptCiphertext` 要求 `REQUIRE_EXISTING`，缺失即失败（防止未初始化用户解密）
- provider 注册（`internal::SetHeSeedDeriver`）用函数级静态量保证每个 SA 进程仅注册一次
- uid/userId 等身份字段日志走 `%{private}u`，不外显

## 常见问题定位

| 问题 | 优先查看 |
|------|----------|
| `PRIVACY_PROTOCOL_INTERNAL_ERROR` 且日志含身份解析失败 | `privacy_service.cpp` 的 `PrepareIdentity`（OS account 解析） |
| 解密报根密钥缺失 | `huks_seed_deriver` 的 `REQUIRE_EXISTING` 策略——先执行过 genPrivacyTarget 才可解密 |
| CE 命名空间/多用户问题 | `HuksSeedDeriver::Derive` 的 userId 选命名空间逻辑 |
| 输出超限 | `privacy_service.cpp` 的 capacity 校验与 `privacy_limits.h` 常量 |

## 数据流

```
core_service/src/privacy.rs（Rust Stub，Binder UID）
  → AssetGenPrivacyTarget / AssetDecryptPrivacyCiphertext（C ABI）
  → PrepareIdentity（uid→userId）→ PrepareRootKey（HUKS CE）
  → privacy_protocol::GenPrivacyTarget / DecryptCiphertext
  → 结果拷回 PrivacyBuffer
```

## 相关主题

- [隐私计算架构](../architecture/overview.md)
- [隐私计算协议库](../protocol/privacy-protocol.md)
- [FFI/IPC 客户端](../frameworks/ipc-client.md)
- [Asset 服务架构](../../asset/architecture/service.md)
