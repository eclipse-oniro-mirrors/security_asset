# AGENTS.md — security_asset 仓代理指南

OpenHarmony security 子系统 `@ohos/asset` 部件仓（part_name=`asset`，subsystem=`security`）。代理在本仓工作时遵循本指南；与用户指令冲突时以用户指令为准。

## 仓库定位

包含三块能力：

1. **Asset 关键资产存储服务（SA 8100）**：短敏感数据（<1024 字节）加密存储与访问控制，加解密委托 HUKS。进程名 `asset_service`。
2. **安全访问围栏 SAF（SA 66532）**：基于 Ticket 的权限围栏、远程控制。进程名 `secure_access_fence`。
3. **隐私计算（privacy_computation）**：PSI/PIR 能力，NAPI 模块 `security.privacyComputation`，协议库在仓库顶层 `privacy_protocol/`，服务侧后端在 `services/asset/privacy/`。

## 目录地图

| 路径 | 内容 |
|------|------|
| `services/asset/`、`services/secure_access_fence/` | 两侧 SA 服务实现（core_service/crypto_manager/db_operator/os_dependency/plugin/privacy） |
| `frameworks/asset/`、`frameworks/secure_access_fence/`、`frameworks/privacy_computation/` | NDK/NAPI/IPC/definition 等框架层 |
| `frameworks/common/napi/` | 公共 NAPI 工具（现仅 SAF 使用） |
| `privacy_protocol/` | 隐私计算协议库（core/protocol/crypto/he/common） |
| `interfaces/` | 对外 API：`kits/c`（NDK）、`inner_kits/c|rs`（系统 API/Rust SDK）、`plugin_interface/` |
| `config.gni` + `bundle.json` + 根 `BUILD.gn` | 部件注册与编译开关三件套 |
| `test/` | 单元测试（见"测试"节） |
| `docs/kb/` + `docs/context_registry.json` | 知识库与路由注册 |
| `.codespec/` | ODK 变更管理（见"变更管理"节） |

## 构建与开关

- 本仓**不能独立编译**：需 OpenHarmony 源码树（本仓路径 `//base/security/asset`），门禁跑多形态编译（LiteOS/Linux/标准/部件编译等）。
- 新增部件级开关的标准三步：`config.gni` `declare_args()` 声明 → `bundle.json` `component.features` 注册 → 目标 BUILD.gn 按开关注入宏（参考 `frameworks/privacy_computation/js/napi/BUILD.gn` 的双开关模式）。产品侧在 vendor 配置覆写。
- 常用开关：`asset_build_mode`（full/stub）、`asset_enable_privacy_computation_config`（闭源计算库路径）、`asset_privacy_computation_supported`（隐私计算能力开关，关闭时三个 JS 接口 Reject 24000017）。
- Rust 侧依赖看 `Cargo.toml`（ylong_runtime 等）。

## 代码约定

- 所有源文件带 Apache 2.0 许可头，版权行 `Huawei Device Co., Ltd.`。
- C++：C++17、`-Wall -WWerror` 之外按各 BUILD.gn 既有 sanitize 配置（CFI/integer_overflow/boundary_sanitize/ubsan）保持，不擅自删减。
- 错误码：Asset 侧 `ErrCode`（24000001 起，`interfaces/kits/c/asset/inc/asset_type.h` 与 `frameworks/asset/definition`）；隐私计算侧 24000001~24000018（`frameworks/privacy_computation/js/napi/inc/privacy_computation_napi_common.h`）；SAF 侧 `SafResultCode`（`saf_result_defs.h`）。三套不要混用。
- 日志：各模块自带 HiLog 封装（`*_log.h`）。**敏感数据（密文、authToken、secret 值）不得打印**，只打长度/错误码等非敏感信息。
- NAPI：异步操作统一走各模块 context/async work 封装；注意 HandleScope 生命周期，避免 NAPI 泄漏。
- GN 中 `-D` 宏注入走 `cflags`/`cflags_cc`；条件块用 `if (flag)` 判布尔、字符串开关判 `!= ""`。

## 测试

| 路径 | 覆盖 |
|------|------|
| `test/asset/unittest/kits_ndk/`、`module_test/`、`inner_kits_rust/`、`ut_test/` | Asset NDK/模块/Rust SDK/stub UT |
| `test/asset/unittest/privacy_coverage/`、`privacy_protocol/`、`privacy_he/`、`privacy_ipc/`、`privacy_root/` | 隐私计算 NAPI/协议/HE/IPC/root |
| `test/secure_access_fence/` | SAF agent_fence/credential/gui_fence/unittest |

改动需配套对应目录的 UT；行为开关类改动需覆盖开启/关闭两种路径。

## 知识库与变更管理

- **知识库**：`docs/kb/` 按子系统分目录，路由注册在 `docs/context_registry.json`（用 `keywords`/`aliases` 检索）。改代码前先查 KB 定位模块入口。KB 是路由提示不是事实源——内容以源码为准；更新 KB 时不写行号、不贴代码片段；仓内没有 `kb_search.py`/`validate_context.py`，检索用文件搜索 + registry。
- **变更管理（.codespec，ODK 约定）**：新增任何变更须登记 `.codespec/registry.md` 并建 `changes/<id>/` 目录（proposal/spec/design/execution-plan 等工件，参考既有变更结构）。声称完成前跑对应 review/validate 纪律。

## 常见任务快速路由

| 任务 | 入口 |
|------|------|
| 新增隐私计算 JS API | `frameworks/privacy_computation/js/napi/src/privacy_computation_napi.cpp` 的 `Register()` 导出表 |
| 新增/调整部件编译开关 | `config.gni` + `bundle.json` features + 目标 BUILD.gn 宏注入 |
| 新增 Asset NDK API | `interfaces/kits/c/asset/` + `frameworks/asset/js/napi/` + `test/asset/unittest/kits_ndk/` |
| 闭源库接入/排查 | `frameworks/privacy_computation/js/napi/src/privacy_computation_napi_loader.cpp` |
| 错误码增改 | 对应子系统 definition 头文件 + `docs/kb/*/error-codes.md` 页面 |
