# 特性规格

> 固化用户可见行为和验收标准。复杂交互、状态机、异常流程可补充 Gherkin 场景。

> **不应包含：** 内部实现流程、中间件调用链路、框架层类名/方法名、内部性能指标。这些内容属于 design.md。
> **THEN 边界：** 每条 AC 的 `THEN` 必须能通过终端用户行为，或 Public / System / InnerAPI 三层接口边界的返回值、回调、错误码、事件或可查询结果直接判定。本特性无新增 API，可观察表面为：①既有 `@ohos.security.asset` 公共 API（query/add 等）在卸载/重装/清理后的返回值与错误码；②系统事件处理结果（数据/密钥/标记的持久化状态，经设备侧检查与 hilog 判定）。

## 概述

| 属性 | 值 |
|------|-----|
| 特性名称 | Asset 适配用户卸载保留数据 |
| 特性编号 | FEAT-20260916754428 |
| 所属 Epic | 无 |
| 优先级 | P3 |
| 目标版本 | OpenHarmony 7.1（引用 proposal.target_release） |
| SIG 归属 | 安全（Asset 子系统） |
| 状态 | Approved（yin yaode，2026-11-13） |
| 复杂度 | 复杂 |

## 本次变更范围（Delta）

> 本 spec 以本仓代码基线（`security_asset_4955`，`services/asset/`）为准。基线卸载链路（`on_package_removed`）对关键资产一律删除；**keepdata 保留与延迟清理能力在基线中不存在，全部为本次新增**。

| 类型 | 内容 | 说明 |
|------|------|------|
| ADDED | **keepdata 卸载保留**：`PACKAGE_REMOVED`（want 中 keepdata=true）时保留该 owner 的关键资产（DE/CE 库数据 + HUKS 密钥）并记录卸载保留标记 | 支撑 BMS"卸载保留数据"特性在关键资产场景的用户预期 |
| ADDED | **延迟清理链路**：订阅并处理 `PACKAGE_UNINSTALLED_DATA_CLEARED`，按标记定位并清理保留数据（`IS_PERSISTENT=true` 豁免），群组按 developer 存活判定 | 卸载保留后重装再卸载时的补偿清理 |
| ADDED | **卸载保留标记位**：独立文件持久化，记录 appId/appIndex/developerId/groupIds（后两项仅 appIndex==0 时记录） | 保留状态与清理定位的唯一依据 |
| MODIFIED | 既有 `PACKAGE_REMOVED` 处理链路条件化：keepdata 缺失/false 及 SANDBOX 变体保持基线行为完全不变 | 兼容性约束，见「兼容性声明」 |
| REMOVED | 无 | — |

## 输入文档

| 文档 | 路径 | 状态 |
|------|------|------|
| Requirement | `.codespec/changes/20260916754428/proposal.md` | Approved（Define，2026-11-13） |
| 代码基线 | `services/asset/`（事件入口 `common_event/start_event.rs`、删除链路 `common_event/listener.rs`、BMS FFI `os_dependency/bms_wrapper.cpp`、SA 订阅 `sa_profile/8100.json`） | 基线事实基准 |

> 需求基线、不涉及项、受影响子系统与仓库详见 proposal.md，本文档不重复摘录。

## 核心术语定义

| 术语 | 定义 |
|------|------|
| **keepdata 卸载** | `usual.event.PACKAGE_REMOVED` 触发的卸载，且事件 want 中 **keepData** 字段（BMS 实证驼峰键名，bool 类型，见 evidence/bms-event-contract.md L53/L334）为 true。缺失/false 或 SANDBOX 变体均视为普通卸载（走基线删除逻辑） |
| **卸载保留标记位** | Asset 内部独立文件持久化的保留状态记录，内容为 appId、appIndex、developerId、groupIds（developerId 与 groupIds 仅在 appIndex==0 时记录）。不对外暴露查询/管理接口 |
| **清理事件** | `usual.event.PACKAGE_UNINSTALLED_DATA_CLEARED`，want 携带 bundleName/developerId/groupIds；触发场景为"先以 keepdata=true 卸载保留、重装、再次卸载"，语义等同一次针对保留数据的卸载（延迟清理） |
| **开发者卸载保留** | 库内 `IS_PERSISTENT=true` 的记录（AssetMap `Tag::IsPersistent`，存量列 `IsPersistent`），表示开发者声明卸载也不删除；在清理事件中豁免删除 |
| **owner** | 资产归属标识 = `appId_appIndex`（与基线 `start_event.rs` 一致）；库文件名由其派生（`Hap_{owner}_…db` / CE `enc_` 前缀） |
| **群组** | 同 developer 下共享数据的 HAP 集合（developerId + groupId）；群组库 `Group_{developer}_{hash}.db`，判定依赖 BMS `GetUninstallGroups`/`GetAllBundleInfoByDeveloperId` |
| **基线卸载行为** | 现版本 `on_package_removed` 的全部行为：删除 DE/CE 库数据（TrustedAccount 云数据除外）、无数据残留时删除 HUKS owner/group 密钥、appIndex==0 时更新升级列表、通知插件 OnPackageClear |

## 用户故事

### US-1: keepdata 卸载保留（含群组全卸载保留）

**作为**设备用户,
**我想要**卸载应用时选择保留数据，重装后关键资产（密码、令牌等）无需重新录入,
**以便**保留数据特性在关键资产场景符合预期.

**正常流程步骤（供测试脚本编排）：**

1. Hap 应用 A（appIndex=0，属于群组 G）调用 `@ohos.security.asset` add 写入关键资产（含 secret）。
2. 卸载 A，卸载弹框选择"保留数据"（系统发出 `PACKAGE_REMOVED`，want 含 keepdata=true、bundleName、appId、appIndex、developerId、groupIds）。
3. 断言：A 的 DE/CE 库文件仍存在；HUKS owner 密钥仍存在；标记文件记录 A 的 appId/appIndex/developerId/groupIds。
4. 重装同 bundle（同 appId）。
5. 调用 asset query（同 alias）断言 resolve，secret 与卸载前一致。

| AC编号 | 验收标准 | 可观察表面 | 类型 |
|--------|----------|--------------|------|
| AC-1.1 | WHEN Hap 应用带 keepdata=true 卸载 THEN 该 owner 的 DE/CE 库数据保留、对应 HUKS owner 密钥保留，且标记文件记录 appId/appIndex（appIndex==0 时含 developerId/groupIds） | 设备侧文件/密钥状态 + 标记文件内容（内部可观察） | 正常 |
| AC-1.2 | WHEN keepdata=true 卸载后重装同 bundle，调用 asset query（原 alias） THEN resolve 并返回原记录（secret 与卸载前一致，可正常解密） | Public API（`@ohos.security.asset` query 返回值） | 正常 |
| AC-1.3 | WHEN 群组内全部 Hap 卸载，且其中任一 Hap 卸载时带 keepdata=true THEN 群组库数据与群组 HUKS 密钥保留，并记录群组保留标记 | 设备侧文件/密钥状态 + 标记文件内容 | 边界 |
| AC-1.4 | WHEN 卸载事件 keepdata 缺失或为 false（含 SANDBOX 变体） THEN 行为与基线完全一致（数据删除、密钥按基线规则清理、插件通知照常） | Public API（重装后 query 返回 24000002）+ hilog 链路日志 | 兼容 |
| AC-1.5 | WHEN 应用 A keepdata 保留期间，第三方应用 B 以自身身份 query A 的保留资产 THEN B 无法获取该资产，行为与基线完全一致（基线非回归检查，不钉具体错误码） | Public API（行为与基线一致） | 异常 |

### US-2: 延迟清理（PACKAGE_UNINSTALLED_DATA_CLEARED）

**作为**设备用户,
**我想要**清理卸载残留数据时，当初因 keepdata 保留的关键资产被一并清理（开发者声明卸载保留的除外）,
**以便**保留数据不成为无法清理的残留.

**正常流程步骤（供测试脚本编排）：**

1. 应用 A（属群组 G，同 developer 另有应用 B 在装）带 keepdata=true 卸载（保留 + 标记）。
2. 重装 A，再次卸载（keepdata 未传）→ 系统发出 `PACKAGE_UNINSTALLED_DATA_CLEARED`（want 含 bundleName/developerId/groupIds）。
3. 断言：A 的保留数据被清理（重装后 query 返回 24000002）；标记清除。
4. 库内 `IS_PERSISTENT=true` 的记录（若有）不被删除。
5. 群组 G：因同 developer 下 B 仍在装，群组数据保留且群组标记清除。

| AC编号 | 验收标准 | 可观察表面 | 类型 |
|--------|----------|--------------|------|
| AC-2.1 | WHEN 应用曾以 keepdata=true 卸载保留、重装后再次卸载触发 `PACKAGE_UNINSTALLED_DATA_CLEARED` THEN 该 owner 的保留数据被清理、标记清除；重装后 query 返回 24000002 | Public API 错误码 24000002 + 标记状态 | 正常 |
| AC-2.2 | WHEN 清理事件执行时该 owner 库内存在 `IS_PERSISTENT=true` 记录 THEN 这些记录豁免删除（其余保留数据正常清理） | Public API（重装后 query 仅返回 is_persistent 记录） | 边界 |
| AC-2.3 | WHEN 清理事件到达且同 developer 下仍有应用未卸载 THEN 群组数据保留、群组标记位清除 | 设备侧文件状态 + 标记状态 | 边界 |
| AC-2.4 | WHEN 清理事件到达且同 developer 下已无应用在装 THEN 群组数据与群组 HUKS 密钥一并清理 | 设备侧文件/密钥状态 | 正常 |
| AC-2.5 | WHEN 清理事件 want 参数缺失或按标记定位不到保留数据 THEN 记录错误日志（hilog），事件处理不中断、不影响后续事件 | hilog 日志 | 异常 |
| AC-2.6 | WHEN 重装后第二次卸载仍带 keepdata=true THEN 再次保留（标记刷新、数据不清理），后续清理事件到达时仍按本 US 规则清理 | 设备侧状态 + Public API（重装 query resolve） | 边界 |

### US-3: 基线兼容与容错

**作为**系统维护者,
**我想要**不带 keepdata 的既有卸载行为与服务稳定性完全不受本特性影响,
**以便**现网存量场景零回归.

| AC编号 | 验收标准 | 可观察表面 | 类型 |
|--------|----------|--------------|------|
| AC-3.1 | WHEN 任意不带 keepdata 的卸载（含 SANDBOX、Native 无关路径、用户删除） THEN 全链路（数据删除、群组判定、密钥清理、插件通知、升级列表）与基线一致 | 现网卸载回归用例集（query 24000002 + hilog） | 兼容 |
| AC-3.2 | WHEN 标记文件缺失或损坏 THEN Asset 服务不异常退出，查询/写入路径行为不变，卸载/清理按"无标记"处理（卸载走基线删除、清理定位失败仅记日志） | hilog + Public API 正常返回 | 异常 |

## 验收追溯

| AC | 关联规则 | 可观察表面 |
|----|----------|--------------|
| AC-1.1 | R-1, R-2, R-11 | 内部可观察（文件/密钥/标记状态） |
| AC-1.2 | R-1, R-10 | Public API query 返回值 |
| AC-1.3 | R-3 | 内部可观察（群组库/密钥/标记） |
| AC-1.4 | R-6 | Public API 24000002 + hilog |
| AC-1.5 | R-11 | Public API（行为与基线一致） |
| AC-2.1 | R-4, R-10 | Public API 24000002 + 标记状态 |
| AC-2.2 | R-4 | Public API query 返回值 |
| AC-2.3 | R-5 | 内部可观察（群组数据/标记） |
| AC-2.4 | R-5, R-10 | 内部可观察（群组数据/密钥） |
| AC-2.5 | R-7 | hilog |
| AC-2.6 | R-9 | 内部可观察 + Public API |
| AC-3.1 | R-6 | 回归用例集 |
| AC-3.2 | R-8 | hilog + Public API |

## 规则定义

| 规则ID | 类型 | 触发条件 | 预期行为 | 边界/约束 | 关联AC |
|--------|------|----------|----------|-----------|--------|
| R-1 | 行为 | `PACKAGE_REMOVED` 且 want 中 keepdata=true | 跳过基线删除链路：该 owner 的 DE/CE 库数据保留、HUKS owner 密钥保留；**不通知插件 OnPackageClear**（保留语义完整，C-1 已确认）；记录卸载保留标记 | 仅 `usual.event.PACKAGE_REMOVED`（非 SANDBOX）；keepdata 为布尔 | AC-1.1, AC-1.2 |
| R-2 | 行为 | keepdata=true 卸载且记录标记 | 标记文件记录：appId、appIndex；**仅 appIndex==0 时**记录 developerId 与 groupIds（群组归属，来自事件 want） | 标记为 Asset 内部文件，不暴露查询/管理接口；多用户按 user 隔离 | AC-1.1 |
| R-3 | 行为 | 群组内全部 Hap 已卸载，且卸载序列中任一 Hap 带 keepdata=true | 群组库数据与群组 HUKS 密钥保留，并记录群组保留标记 | 混合 keepdata（部分带、最后卸载的不带）同样保留；全部不带则走基线群组删除。**本应用自身（appId/appIndex 精确匹配）的标记在其不带 keepdata 的卸载中先行消费**（需求原文"等下次该 hap 再卸载时如果 keepdata 没传，则按照老的逻辑处理"）：自身群组保护失效、按基线老逻辑判定；**同 bundle 其他应用（如处于自身保留周期的分身）的标记与数据不受影响** | AC-1.3 |
| R-4 | 行为 | `PACKAGE_UNINSTALLED_DATA_CLEARED` 到达且按标记定位到保留数据 | 删除**触发该事件的应用自身**（按事件携带的 appId/appIndex 精确匹配保留条目）的保留数据（`IS_PERSISTENT=true` 记录豁免），并仅消费该条目 | 定位依据 = 事件 appId/appIndex（BMS 实证携带）+ 标记条目（appId/appIndex）；同 bundle 其他保留应用（如处于自身保留周期的分身）的数据与标记**不受影响**；事件缺省 appId/appIndex 时回退 bundle 全量（兼容旧 BMS）。标记亦在**同包基线卸载时被先行消费**（R-3），因此本事件消费不到时定位为空属正常（基线已处理），事件幂等 | AC-2.1, AC-2.2 |
| R-5 | 行为 | 清理事件到达且事件涉及群组 | 按 BMS 查询（`GetAllBundleInfoByDeveloperId` 链路）判定同 developer 存活应用：有存活 → 群组数据保留、群组标记清除；无存活 → 群组数据与群组 HUKS 密钥清理 | 群组判定复用现有 `GetUninstallGroups`/developer 查询能力（proposal Q-12 已确认） | AC-2.3, AC-2.4 |
| R-6 | 兼容 | keepdata 缺失/false、SANDBOX 变体、或任何不带 keepdata 的既有路径 | 与基线行为完全一致：DE/CE 删除（TrustedAccount 除外）、无残留删 HUKS 密钥、appIndex==0 更新升级列表、通知插件 OnPackageClear | 本特性不得改变基线触发条件与结果 | AC-1.4, AC-3.1 |
| R-7 | 异常 | 事件 want 参数缺失、解析失败或按标记定位不到数据 | 记录 hilog 错误日志（含事件名与缺失字段），不中断事件处理、不影响后续事件 | 事件处理为尽力而为（best-effort），不向系统返回错误 | AC-2.5 |
| R-8 | 恢复 | 标记文件缺失或损坏 | 退化处理：keepdata 卸载仍保留数据（可重新写标记）；清理事件定位失败仅记日志；查询/写入路径完全不依赖标记文件 | 标记丢失的最坏后果 = 清理遗漏（数据多保留），不得出现数据误删或服务异常 | AC-3.2 |
| R-9 | 行为 | 重装后第二次卸载仍带 keepdata=true | 再次保留：数据不清理，标记刷新（按最新卸载事件更新） | proposal Q-13 默认假设 | AC-2.6 |
| R-10 | 行为 | HUKS 密钥与数据清理的联动 | **成功路径**：keepdata 保留路径一律不删密钥；清理/基线删除后按"无数据残留才删密钥"（owner/群组密钥同理）。**失败路径例外（Owner 裁决）**：数据已删而 HUKS 密钥删除失败时，接受**孤立密钥残留**并上传 fault 事件——密钥无数据即无明文泄露；不可采用反向顺序（先删密钥后删数据失败=不可解密的敏感数据残留，危害更大），亦无自动重试（ADR-7） | "严禁数据保留而密钥已删"适用全部路径；"严禁数据已删而密钥残留"**仅适用成功路径**，HUKS 删除失败的单次故障以 fault 事件可观测即闭环 | AC-1.2, AC-2.1, AC-2.4 |
| R-11 | 行为 | 保留数据的访问控制 | 保留期间数据隔离、权限校验、加解密体系与基线完全一致（owner 身份不变；本需求零接口路径改动，隔离为基线继承行为） | 保留不改变任何访问控制语义；验收为基线非回归检查，不钉具体错误码 | AC-1.5, AC-1.2 |

### 密钥状态验收 oracle（数据残留矩阵，Q-008 闭环）

HUKS 服务密钥无公开查询接口（alias 为服务内部派生），密钥状态断言采用**数据残留矩阵 + 决策日志行**的组合证明，不得以单一 query 结果代替：

| 清理后的数据残留状态 | 密钥期望状态 | 决策日志行（hilog oracle） |
|----------------------|--------------|---------------------------|
| 无任何残留（非豁免已删、无 IS_PERSISTENT、无 TrustedAccount） | 已删除 | `No data remain in owner/group db after uninstallation. Delete owner/group key in HUKS.` |
| 存在 IS_PERSISTENT=true 记录 | 保留 | `Data remain ... Do not delete owner/group key in HUKS.` |
| 存在 TrustedAccount（SyncType）数据 | 保留 | 同上 |
| 上述豁免/云数据混合 | 保留 | 同上 |
| HUKS 删除调用失败（故障注入） | 残留（失败路径例外，R-10） | fault 事件（upload_fault_system_event） |

数据残留侧断言组合：query 返回 24000002（证明**非豁免记录**不存在）+ is_persistent 记录仍可 query（证明豁免存在）+ db 文件存在性（`ls`）。**单独的 query=24000002 只证明非豁免记录不存在，不构成密钥已删的证据。**

## 验证映射

> 每个 AC 至少关联一个可执行测试入口。`Red 条件` 描述实现前测试为什么必然失败。
> **保证方式分工：** 事件驱动行为通过**模块测试**（本仓 `test/asset/unittest/module_test`，可经导出符号/FFI 触发事件处理函数）与**设备端到端**（hdc 卸载/重装 + asset query 断言）保证。

| 编号 | 对应规格项 | 测试入口 | 验证方式 | Red 条件（实现前失败信号） | 通过标准 |
|------|------------|----------|----------|---------------------------|----------|
| VM-1 | R-1,R-2 / AC-1.1, AC-1.2 | 模块测试：keepdata=true 卸载（mock 事件入口）+ 重装后 query | 模块测试 + 设备端到端 | 未实现保留分支时，带 keepdata 卸载后数据被删，重装 query 返回 24000002 | 库文件/密钥保留；标记内容正确（appId/appIndex；appIndex==0 含 developerId/groupIds）；query resolve 且 secret 一致 |
| VM-2 | R-3 / AC-1.3 | 模块测试：群组多 Hap 全卸载（任一 keepdata） | 模块测试 | 未实现群组保留时，最后一个 Hap 卸载触发基线群组删除 | 群组库/密钥保留 + 群组标记存在 |
| VM-3 | R-6 / AC-1.4, AC-3.1 | 现网卸载回归用例集（keepdata 缺失/false、SANDBOX） | 模块测试回归 + 设备端到端 | 保留分支误伤普通卸载时，普通卸载后数据仍存在 | 与基线完全一致（数据删、密钥按规则清、插件通知照常） |
| VM-4 | R-4 / AC-2.1 | 模块测试：保留→重装→再卸载→清理事件 | 模块测试 + 设备端到端 | 未实现清理链路时，清理事件后数据仍可 query 到 | 保留数据删除，重装 query 返回 24000002；标记清除 |
| VM-5 | R-4 / AC-2.2 | 模块测试：库内含 IS_PERSISTENT=true 记录的清理 | 模块测试 | 未实现豁免时 is_persistent 记录被一并删除 | 仅非 is_persistent 数据被清理 |
| VM-6 | R-5 / AC-2.3, AC-2.4 | 模块测试：清理事件群组矩阵（同 developer 有/无存活应用） | 模块测试 | 未实现 developer 判定时群组清理规则错误 | 有存活→保留+清标记；无存活→清理（含群组密钥） |
| VM-7 | R-7,R-8 / AC-2.5, AC-3.2 | 模块测试：事件参数缺失、标记文件损坏/删除 | 模块测试 | 无容错时事件处理 panic 或服务退出 | hilog 记录错误，服务与查询路径不受影响 |
| VM-8 | R-11 / AC-1.5 | 设备端到端：保留期间第三方应用 query | 设备端到端 | 隔离破坏时第三方 query 成功 | 第三方无法获取（行为与基线一致，不钉错误码） |
| VM-9 | R-3, R-4, R-5 / AC-1.3, AC-2.1~2.4 | 设备端到端：多群组交叉矩阵——app1（群组 A/B）keepdata 卸载 → app2（群组 A/C，同 developer）普通卸载 → app1 触发清理 | 设备端到端 | 群组保留豁免未按标记命中（app2 卸载误删群组 A）或清理终局未删 A/B | ①app1 与 A/B 保留；②app2 与 C 删、A 保留（标记命中）；③app1 与 A/B 删（终局判定），标记消费 |

## 测试数据样例

> 供模块测试与设备端到端直接参数化；样例不构成对外契约。

### 事件样例

| # | 场景 | 事件与 want 样例 | 预期断言 |
|---|------|------------------|----------|
| TD-1 | keepdata 卸载（appIndex=0，属群组） | `PACKAGE_REMOVED`：userId=100, bundleName="com.example.app", appId="B3AG…", appIndex=0, developerId="…", groupIds="gid1,gid2", keepdata=true | 库/密钥保留；标记含 appId/appIndex/developerId/groupIds |
| TD-2 | keepdata 卸载（clone 应用，appIndex=1） | 同上但 appIndex=1 | 标记仅含 appId/appIndex（无 developerId/groupIds） |
| TD-3 | 普通卸载 | 同 TD-1 但 keepdata 缺失 / false；或 `SANDBOX_PACKAGE_REMOVED` | 基线删除（query 24000002） |
| TD-4 | 清理事件 | `PACKAGE_UNINSTALLED_DATA_CLEARED`：bundleName="com.example.app", developerId="…", groupIds="gid1,gid2" | 按 TD-1 标记定位并清理（is_persistent 豁免）；群组按 developer 存活判定 |
| TD-5 | 混合 keepdata 群组 | Hap-A keepdata=true 卸载，Hap-B keepdata=false 后卸载 | 群组保留 + 群组标记 |

### 错误码触发样例

| 错误码 | 触发方式 | 关联 AC |
|--------|----------|---------|
| 24000002（NotFound） | 普通卸载后重装 query；清理事件执行后重装 query。（参考：第三方查询他人保留资产按 API 作用域语义亦为 24000002，属基线行为，不作本特性 oracle） | AC-1.4, AC-2.1 |

## API 变更分析

> 开放级别：Public/System/InnerAPI。**本变更不涉及任何 API 变更**：无新增、变更或废弃 API；`@ohos.security.asset` 既有 API（add/query/update/remove 等）签名、权限、错误码语义完全不变。

### 新增 API

无。

### 变更/废弃 API

无。本特性全部行为经系统事件驱动，可观察表面为既有 Public API 的**数据可见性**变化（卸载后数据按 keepdata 语义保留/清理）与内部持久化状态（库文件、HUKS 密钥、标记文件），不存在接口层变更。

### API 与错误码事实

| 事实项 | 事实类型 | 精确数值 | 触发条件 | 对外行为 | 事实来源 | 关联 AC |
|--------|----------|----------|----------|----------|----------|---------|
| Asset query NotFound | Existing Error Code | 24000002 | 查询目标不存在（数据已被卸载/清理删除） | reject BusinessError 24000002 | `frameworks/asset/definition/src/lib.rs:204`（ErrCode::NotFound） | AC-1.4, AC-2.1 |
| Asset query AccessDenied | Existing Error Code | 24000004 | 认证失败/authToken 异常等（实证触发面）；跨 owner 查询非其实证触发（API 作用于调用者自身库，参考预期 24000002） | reject BusinessError 24000004 | `frameworks/asset/definition/src/lib.rs:210` | —（不关联 AC-1.5，隔离用例为基线非回归口径） |
| keepdata 事件字段 | New 事件事实 | want 布尔参数 keepdata | `usual.event.PACKAGE_REMOVED` | true=保留；缺失/false=基线删除 | proposal 澄清 Q-1（2026-11-13 已确认） | AC-1.1, AC-1.4 |
| PACKAGE_UNINSTALLED_DATA_CLEARED | New 事件事实 | want：bundleName/developerId/groupIds | keepdata 保留后重装再卸载 | 触发延迟清理 | proposal 澄清 Q-3（2026-11-13 已确认） | AC-2.1 |

## 接口规格

> 不适用：本变更无接口层规格（见「API 变更分析」）。事件 want 参数约束见「测试数据样例」与规则定义 R-1/R-4。

## API 规格定义

> 条件触发（`API/SDK` = 「否」，proposal 不涉及项确认）：本节不适用，无 `interface_sdk-js` 声明变更。

## 兼容性声明

- **已有 API 行为变更:** 否。`@ohos.security.asset` 全部 API 签名、权限、错误码语义不变。
- **配置文件格式变更:** 是（`sa_profile/8100.json` 新增事件订阅项 `PACKAGE_UNINSTALLED_DATA_CLEARED`），不影响既有配置语义。
- **数据存储格式变更:** 否（库 schema 不变；标记文件为新增独立文件，非库内格式）。
- **旧版本 BMS 兼容:** 旧版本卸载事件无 keepdata 字段 → 全部走基线行为，零影响。
- **升级/回退:** 升级到新版本无需数据迁移（标记文件从零开始累积）；回退到旧版本时标记文件残留无害（旧版本不读取；再次升级后清理事件按幂等处理，定位不到仅记日志）。
- **最低支持版本:** OpenHarmony 7.1（随 proposal.target_release；无新增 API，无 @since 变更）。

## 架构约束

> 设计方案见 design.md（待编制）。

| 关键约束 | 约束说明 | 影响 AC |
|----------|----------|---------|
| keepdata 判定只发生在事件入口 | 保留/清理分支仅存在于 `PACKAGE_REMOVED`/`PACKAGE_UNINSTALLED_DATA_CLEARED` 事件处理链路；asset add/query/update/remove 等业务路径零改动、零标记依赖 | AC-1.2, AC-3.2 |
| 密钥与数据清理强绑定 | 严禁"数据保留而密钥已删"（不可解密态）与"数据已删而密钥残留"；密钥清理时机与数据清理在同一链路内决策 | AC-1.1, AC-2.1, AC-2.4 |
| 标记文件为独立持久化 | 不入库、不进查询路径；写入失败不得影响卸载事件主流程（尽力而为） | AC-3.2 |
| 群组判定复用既有 BMS 能力 | developer/群组存活判定复用 `GetUninstallGroups`/`GetAllBundleInfoByDeveloperId` 链路，不新增 BMS FFI | AC-2.3, AC-2.4 |

## 非功能性需求

| 类型 | 指标/阈值 | 验证方式 | 证据 |
|------|-----------|----------|------|
| 性能 | not-applicable（事件低频冷路径，业务路径零改动） | — | — |
| 存储(ROM) | 新增事件处理与标记位代码，镜像增量评估（review-required） | ABU/冒烟 | `evidence/`（待验证） |
| 存储(磁盘) | 标记文件按卸载事件累积，体量极小（每条记录含 appId/appIndex 等） | 磁盘占用检查 | `evidence/`（待验证） |
| 内存 | 无新增常驻内存 | 代码核对 | design.md |
| 安全 | 保留数据隔离/加密不降级；密钥与数据生命周期强绑定 | 安全 review + 用例（VM-8） | threat-model.md（如触发） |
| 可测试性 | 每个 AC 有测试入口 + Red 条件（VM-1~VM-9） | 模块测试 + 设备端到端 | `evidence/reviews/` |

## 多设备适配声明

| 设备类型 | 行为差异 | 规格/约束 | 验证方式 | 证据 |
|----------|----------|-----------|----------|------|
| 全部设备 | 无差异 | 共用基线（纯系统侧事件驱动行为） | 端到端 | `evidence/reviews/` |

## 全局特性影响

| 特性 | 适用？ | 结论 | 关联场景 |
|------|--------|------|----------|
| 无障碍/大字体/深色模式/多窗口 | 否 | 纯系统侧行为，无 UI 交互面 | — |
| 多用户 | 是 | 标记文件按 user 隔离（与 DE 库目录同构）；不引入跨用户语义 | US-1 |
| 版本升级 | 是 | 新增事件订阅与标记文件；无 API/数据格式变更 | 兼容性声明 |
| 生态兼容 | 是 | 不对三方开发者开放任何新接口；行为变化仅限系统事件驱动 | — |

## 审批确认记录（Spec 评审结论）

> 评审项已于 2026-11-13 由需求方（对话）逐项确认；**Spec 已由 Owner（yin yaode）批准（2026-11-13）**。

| # | 确认项 | 结论 | 状态 |
|---|--------|------|------|
| C-1 | keepdata=true 卸载时是否通知插件 OnPackageClear | **不通知**（插件清理与保留语义矛盾）；基线路径（keepdata 缺失/false）通知行为不变 | ✅ 已确认 |
| C-2 | 二次卸载仍带 keepdata 的行为（proposal Q-13 默认假设） | **再次保留、标记刷新**，后续清理事件到达时仍按 US-2 规则清理 | ✅ 已确认 |
| C-3 | 目标发行版本与优先级 | **OpenHarmony 7.1 / P3**（需求方确认 2026-11-13；无新增 API，无 @since 变更） | ✅ 已确认 |
| C-4 | 清理事件触发场景 | **不包含**"设置中手动清理残留"场景；仅固化"keepdata 保留→重装→再卸载"触发面 | ✅ 已确认 |
