# SAF 常见问题定位 Context

> 文档版本：v1.0
> 更新时间：2026-10-13
> 来源：代码仓结构分析

## 定位

汇总安全访问围栏（SAF，SA 66532）开发中的常见问题、排查路径和注意事项。

## 常见问题定位

### 1. SAF Ticket 验证失败

| 症状 | 排查路径 |
|------|----------|
| Ticket 验证失败 | `ticket_operation/challenge_store.rs` → 检查 Challenge 是否被重放 |
| 密钥管理错误 | `ticket_key_manager.rs` → 确认使用账户模式还是本地模式 |
| 远程控制失败 | `remote_control/controlled_device.rs` / `controller_device.rs` → 检查设备包生成/验证 |
| 锁屏状态错误 | `services/secure_access_fence/os_dependency/src/screen_lock_wrapper.cpp` |

### 2. SA 启动失败

| 症状 | 排查路径 |
|------|----------|
| SA 66532 未启动 | 检查 `etc/init/secure_access_fence.cfg`，`sa_profile/66532.json` |
| `dlopen` 失败 | 检查 SAF 扩展库是否存在（`services/secure_access_fence/plugin/src/saf_plugin.rs`） |

### 3. SAF 错误码定位

| 症状 | 排查路径 |
|------|----------|
| `0x32001` (SAF_ERR_PERMISSION_DENIED) | 检查 `access_token_wrapper.cpp` 权限声明 |
| `0x22002` (SAF_ERR_SCREENLOCK_IS_LOCKED) | 检查 `screen_lock_wrapper.cpp` 锁屏状态 |
| `0x12001` (SAF_ERR_SERVICE_UNAVAILABLE) | 检查 SA 66532 是否已启动（`sa_profile/66532.json`） |

### 4. 构建问题

| 症状 | 排查路径 |
|------|----------|
| `product not found` | 确认 `BUILD.gn` 顶层目标 `saf_component` |
| `undefined reference` | 检查 `Cargo.toml` workspace 成员、`BUILD.gn` 依赖链 |
| 链接错误 | 检查各 `.map` 符号映射文件 |

## 调试入口

### 日志

| 日志类型 | 位置 |
|----------|------|
| SAF 日志 | `frameworks/secure_access_fence/os_dependency/log/` |

### 调试命令

```bash
# 编译 saf 模块
./build.sh --product-name rk3568 --ccache --build-target saf

# 查看 SA 状态
hdc shell ps -ef | grep secure_access_fence
```

## 开发要点

- SAF 排查问题优先查看 SAF 日志（`frameworks/secure_access_fence/os_dependency/log/`）
- SA 66532 启动失败首先检查 init 配置（etc/init/secure_access_fence.cfg）和 sa_profile（66532.json）
- Ticket 验证问题通过 challenge_store.rs 检查 Challenge 生命周期和防重放
- 锁屏问题通过 screen_lock_wrapper.cpp 确认设备锁屏状态

## 常见开发场景

- 开发者排查 SAF Ticket 验证或远程控制失败的原因
- 测试团队定位 SA 66532 启动失败或 IPC 通信异常
- 安全应用排查权限拒绝（0x32001）或锁屏状态（0x22002）问题

## 相关主题

- [整体架构概览](../shared/overview.md)
- [SAF Core Service 模块](./services/core-service.md)
- [错误码参考](./error-codes.md)
