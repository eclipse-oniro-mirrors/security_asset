# SAF Agent Fence API Context

> 文档版本：v1.0
> 更新时间：2026-10-13
> 来源：`interfaces/inner_kits/c/secure_access_fence/agent_fence/`

## 定位

安全访问围栏的 C++ 客户端 API，提供 `SafAgentFence` 类，封装 Ticket 生成/验证、远程控制、命令权限查询与授予等能力。

## 快速路由

### 源码入口

| 文件 | 说明 |
|------|------|
| `interfaces/inner_kits/c/secure_access_fence/agent_fence/inc/saf_agent_fence.h` | `SafAgentFence` 类声明 |
| `interfaces/inner_kits/c/secure_access_fence/agent_fence/BUILD.gn` | 构建目标 `libsaf_agent_fence` |
| `interfaces/inner_kits/c/secure_access_fence/agent_fence/libsaf_agent_fence.map` | 符号导出映射 |
| `interfaces/inner_kits/c/secure_access_fence/common/inc/saf_result_defs.h` | SAF_ASSIGN_MODULE_TYPE_ENUM / SAF_ASSIGN_RESULT_CODE_ENUM 宏 |
| `interfaces/inner_kits/c/secure_access_fence/common/inc/saf_result_code.h` | SafModuleType、SafResultCode 枚举 |
| `frameworks/secure_access_fence/inner_api/agent_fence/src/saf_agent_fence.cpp` | 客户端实现 |
| `frameworks/secure_access_fence/inner_api/agent_fence/src/saf_agent_params_checker.cpp` | 参数校验 |

### API 入口

`SafAgentFence` 类方法：

| 方法 | 说明 |
|------|------|
| `BatchQueryCommandPermission` | 批量查询命令权限 |
| `BatchGenerateTicket` | 批量生成 Ticket |
| `BatchVerifyTicket` | 批量验证 Ticket |
| `VerifyTicket` | 单个验证 Ticket |
| `RequestToolPermissions` | 请求工具权限 |
| `GrantToolPermissionsByUser` | 用户授予工具权限 |
| `GenerateControlledDevicePackage` | 生成受控设备包 |
| `VerifyControlledDevicePackage` | 验证受控设备包 |
| `GenerateControllerDevicePackage` | 生成控制端设备包 |
| `VerifyControllerDevicePackage` | 验证控制端设备包 |
| `GetRemoteGrantStatus` | 获取远程授权状态 |
| `UpdateRemoteGrantStatus` | 更新远程授权状态 |

### JS NAPI 对应

| NAPI 文件 | 说明 |
|----------|------|
| `frameworks/secure_access_fence/js/napi/src/agent_fence_napi.cpp` | JS 绑定 |
| `frameworks/secure_access_fence/js/napi/inc/agent_fence_napi_context.h` | NAPI 上下文 |
| `frameworks/secure_access_fence/js/napi/inc/agent_fence_error_codes.h` | 错误码 |
| `frameworks/secure_access_fence/js/napi/BUILD.gn` | 构建目标 `libabilitytoolaccessctrl_napi` |

### 测试入口

| 测试 | 路径 |
|------|------|
| Ticket 生成/验证 | `test/secure_access_fence/agent_fence/src/saf_agent_fence_gen_and_ver_ticket_test.cpp` |
| 锁屏集成 | `test/secure_access_fence/agent_fence/src/saf_agent_fence_screen_lock_test.cpp` |
| 锁屏+HAP 身份 | `test/secure_access_fence/agent_fence/src/saf_agent_fence_screen_lock_hap_identity_test.cpp` |
| 压力测试 | `test/secure_access_fence/agent_fence/src/saf_agent_fence_stress_test.cpp` |
| 无权限测试 | `test/secure_access_fence/agent_fence/src/saf_agent_fence_no_permission_test.cpp` |
| 命令权限查询 | `test/secure_access_fence/agent_fence/src/saf_agent_fence_query_cmd_permission_test.cpp` |
| 请求工具权限 | `test/secure_access_fence/agent_fence/src/saf_agent_fence_request_tool_permissions_local_test.cpp` |
| 用户授予权限 | `test/secure_access_fence/agent_fence/src/saf_agent_fence_grant_tool_permissions_by_user_local_test.cpp` |
| IPC 测试 | `test/secure_access_fence/unittest/kits_ndk/src/saf_agent_fence_ipc_test.cpp` |
| 参数校验测试 | `test/secure_access_fence/unittest/kits_ndk/src/saf_agent_params_checker_test.cpp` |
| 批量查询测试 | `test/secure_access_fence/unittest/kits_ndk/src/saf_agent_fence_batch_query_cmd_test.cpp` |
| 远程控制参数测试 | `test/secure_access_fence/unittest/kits_ndk/src/saf_agent_fence_remote_control_params_test.cpp` |

## 数据流

```
应用 C++ 代码 → SafAgentFence::BatchGenerateTicket 等
  → IPC 序列化 (saf_ipc crate) → send IPC to SA 66532
  → SAFService::on_remote_request (stub.rs) → ticket_operation / remote_control
  → 返回结果通过 IPC 反序列化回到 Agent Fence 客户端
```

SafAgentFence 是 C++ 客户端类，所有方法通过 IPC 调用 SA 66532 服务端。参数校验在 `saf_agent_params_checker.cpp` 中集中处理，校验通过后才发起 IPC 请求。

## 开发要点

- SafAgentFence 是 C++ 客户端类，所有方法通过 IPC 调用 SA 66532 服务端
- 参数校验在 saf_agent_params_checker.cpp 中集中处理，新增方法需补充校验
- Ticket 验证依赖 Challenge Store 防重放，不要在客户端缓存 Challenge
- 远程控制包格式变更需同时更新 controlled_device.rs 和 controller_device.rs

## 常见开发场景

- 系统工具应用通过 Agent Fence 申请工具权限和验证 Ticket
- 分布式设备通过远程控制包实现跨设备权限授予
- 安全应用通过 BatchVerifyTicket 批量验证多个操作权限

## 相关主题

- [安全访问围栏架构](../architecture/secure-access-fence.md)
- [IPC 接口定义](./ipc-interfaces.md)
- [公共 NDK API](./ndk-api.md)
