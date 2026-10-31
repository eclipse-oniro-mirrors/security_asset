# Crypto Manager 模块 Context

> 文档版本：v1.0
> 更新时间：2026-10-13
> 来源：`services/asset/crypto_manager/`

## 定位

Crypto Manager 负责 Asset 服务的加解密操作和密钥管理。通过 HUKS（通用密钥库系统）在安全环境（TEE）中执行 AES-GCM 加解密，并管理按应用缓存的 Crypto 实例和数据库加密密钥。

## 快速路由

### 源码入口

| 文件 | 说明 |
|------|------|
| `services/asset/crypto_manager/src/lib.rs` | 模块声明、FFI 结构体（HksBlob、OutBlob、KeyId） |
| `services/asset/crypto_manager/src/crypto_manager.rs` | `CryptoManager` 单例（按应用桶式缓存） |
| `services/asset/crypto_manager/src/crypto.rs` | `Crypto` 结构体（encrypt/decrypt/init/exec/drop 生命周期） |
| `services/asset/crypto_manager/src/secret_key.rs` | 密钥管理（generate/delete/is_exist/rename） |
| `services/asset/crypto_manager/src/db_key_operator.rs` | DB 密钥管理（get_db_key） |
| `services/asset/crypto_manager/src/huks_wrapper.h` | HUKS C 包装头文件 |
| `services/asset/crypto_manager/src/huks_wrapper.c` | HUKS C 包装实现 |
| `services/asset/crypto_manager/BUILD.gn` | 构建配置 |

### HUKS 包装 API

| 函数 | 说明 |
|------|------|
| `GenerateKey` | 生成密钥 |
| `DeleteKey` | 删除密钥 |
| `IsKeyExist` | 检查密钥是否存在 |
| `EncryptData` | 加密数据（AES-GCM） |
| `DecryptData` | 解密数据 |
| `InitKey` | 初始化加密会话 |
| `ExecCrypt` | 执行加密操作 |
| `Drop` | 释放加密会话 |
| `RenameKeyAlias` | 重命名密钥别名 |

### CryptoManager 缓存

| 常量 | 值 | 说明 |
|------|----|------|
| `CRYPTO_CAPACITY` | 16 | 每应用最大缓存数 |
| `CRYPTO_TOTAL_CAPACITY` | 32 | 全局最大缓存数 |

CryptoManager 方法：`add`（添加缓存）、`find`（查找缓存）、`remove_expired_crypto`（移除过期缓存）

### DB Key Operator

`db_key_operator.rs` 中的 `get_db_key(user_id, is_ce)`：
- 按 user_id 生成/获取数据库加密密钥
- 区分 CE（Credential Encrypted）和 DE（Device Encrypted）场景
- 密钥通过 HUKS 在安全环境生成和管理

### 加密参数

| 参数 | 值 | 说明 |
|------|----|------|
| `TAG_SIZE` | 16 | AES-GCM 认证标签大小 |
| `NONCE_SIZE` | 12 | AES-GCM Nonce 大小 |

### 调用链路

```
operation_add/update → CryptoManager.find/add → Crypto.encrypt
  → huks_wrapper.c → HUKS API (hks_api.h) → TEE 加密
  → 返回密文 → DB Operator 存储

operation_query → DB Operator 读取密文 → CryptoManager.find → Crypto.decrypt
  → huks_wrapper.c → HUKS API → TEE 解密 → 返回明文
```

### 安全特性

- 加解密在 HUKS 安全环境执行，即使系统被攻破也不泄露密钥
- 支持 UserIAM 用户认证（锁屏密码/指纹/人脸），认证通过后才解密
- 每应用独立密钥，应用间数据隔离

### 测试入口

| 测试 | 路径 |
|------|------|
| Crypto Manager 测试 | `test/asset/unittest/module_test/src/crypto_manager/lib.rs` |
| HUKS 包装测试 | `test/asset/unittest/module_test/src/huks_wrapper_test.cpp` |

## 开发要点

- CryptoManager 按 CallingInfo 缓存 Crypto 实例，缓存满时 LRU 淘汰（CRYPTO_CAPACITY=16）
- HUKS 密钥别名按应用+用户生成，删除应用时需同步清理密钥（secret_key.rs delete）
- DB 密钥（db_key_operator.rs）区分 CE/DE 场景，CE 密钥需用户解锁后才可获取
- AES-GCM 参数固定（TAG_SIZE=16, NONCE_SIZE=12），不要在业务代码中修改

## 常见开发场景

- 新增资产操作时通过 CryptoManager 加解密数据
- 排查加解密失败问题（密钥不存在、HUKS 不可用、认证未通过）
- 用户切换时清理旧用户密钥缓存

## 相关主题

- [Core Service 模块](./core-service.md)
- [DB Operator 模块](./db-operator.md)
- [Asset 服务架构](../architecture/service.md)
- [OS Dependency 模块](./os-dependency.md)
