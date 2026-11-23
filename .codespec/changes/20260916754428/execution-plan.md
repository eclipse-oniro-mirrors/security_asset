# 执行计划

> 将 Approved Spec 拆成可独立执行、可验证、可审查的 Task。每个 Task 用稳定 ID 和章节锚点引用上游事实，不复制 Spec/Design 正文。

## Plan 元数据

| 字段 | 内容 |
|------|------|
| Plan ID | PLAN-20260916754428 |
| 关联 Feature | FEAT-20260916754428（Asset 适配用户卸载保留数据） |
| 关联文档 | proposal.md / design.md / spec.md |
| 复杂度 | 复杂 |
| 状态 | Approved（yin yaode，2026-11-13） |
| Owner | yin yaode |

## 输入状态

| 输入 | 路径 | 要求状态 |
|------|------|----------|
| Requirement | `.codespec/changes/20260916754428/proposal.md` | Approved（2026-11-13） |
| Design | `.codespec/changes/20260916754428/design.md` | Approved（2026-11-13；ADR-4 经 plan 核实修正为"直接复用"，行为语义不变） |
| Spec | `.codespec/changes/20260916754428/spec.md` | Approved（2026-11-13） |

## 执行原则

- **Spec 权威：** 实现与 AC、错误码、兼容性声明冲突时，停止并修订 spec/design/plan，不在代码里自行改语义。
- **测试/证据先行：** 每个 Task 先确认失败测试或可复现证据缺口，再做最小实现并取得 fresh PASS 证据。
- **任务小型化：** 一个 Task 只覆盖一个独立闭环。
- **文件边界：** 只修改 Task 的 Files；需要额外文件时先修订 Plan。
- **状态所有权唯一：** keepdata.cache 状态的 owner、key、创建、读取、更新、清理触发和不变量在 TASK-1 定义。
- **证据回填：** Task 完成当次会话回填追溯状态、实际文件、Commit、Review Evidence 和 Actual Result。
- **基线零侵入：** keepdata 缺失/false 路径不得进入新代码分支（spec R-6）；diff 审查必须验证这一点。

## 受影响文件全量清单

> 对照 design.md 调用链层级分析，逐层列出具体文件。

| 仓 | 层（来自 design.md） | 文件路径 | 修改类型 | 说明 |
|----|---------------------|----------|----------|------|
| security_asset | 事件入口 | `services/asset/core_service/src/common_event/start_event.rs` | 修改 | keepdata 常量与解析（宽松 bool）、`PackageInfo` 传参、`PACKAGE_UNINSTALLED_DATA_CLEARED` 路由与 want 解析 |
| security_asset | 事件入口（类型定义） | `services/asset/core_service/src/lib.rs` | 修改 | `PackageInfo`/`PackageInfoFfi` 增 keepdata 字段 |
| security_asset | 事件入口（C++ 订阅通路） | `services/asset/os_dependency/inc/system_event_wrapper.h`、`services/asset/os_dependency/src/system_event_wrapper.cpp` | 修改 | PackageInfo 结构加 keepdata 字段（对齐 Rust FFI 布局）；HandlePackageRemoved 解析并传递 keepdata（设备测试发现的第二通路，2026-11-17 补录） |
| security_asset | 事件处理 | `services/asset/core_service/src/common_event/listener.rs` | 修改 | keepdata 保留分支（顶层短路+写标记）、群组保留判定（删除前标记命中检查）、清理链路（定位→删除→密钥→群组判定→标记消费） |
| security_asset | 标记存储 | `services/asset/db_operator/src/keepdata_cache.rs` | 新增 | 独立 keepdata.cache 读写/匹配/条目消费（复用 upgrade.cache 代码模式：全局锁 + 0o640 + serde；与 upgrade.cache 用途隔离） |
| security_asset | 标记存储（注册） | `services/asset/db_operator/src/lib.rs` | 修改 | `pub mod keepdata_cache;` |
| security_asset | SA 配置 | `sa_profile/8100.json` | 修改 | events 数组新增 `usual.event.PACKAGE_UNINSTALLED_DATA_CLEARED` |
| security_asset | 单测 | `services/asset/db_operator/src/keepdata_cache.rs`（#[cfg(test)] 内联单测） | 新增 | 条目合并/刷新/匹配/消费单测（文件 IO 归 TASK-5 设备用例） |
| security_asset | 模块/端到端测试 | `test/asset/unittest/module_test/`、XTS/设备端到端脚本 | 新增 | VM-1~VM-9 对应用例 |

**检查项：**
- [x] design.md 调用链每一层（事件入口→事件处理→标记存储→SA 配置→测试）都有对应文件
- [x] core_service 构建/依赖零改动（标记存储落位 db_operator，serde 为既有依赖；BUILD.gn 为 root-only sources 模式）
- [x] 无遗漏层（配置、测试已覆盖）

## AC 到 Task 追溯

| AC | 来源 | Task | 计划代码范围 | 实际 Code Ref | Commit | Review Evidence | 验证状态 |
|----|------|------|--------------|---------------|--------|-----------------|----------|
| AC-1.1 | spec.md | TASK-1, TASK-2 | keepdata_cache.rs + listener.rs 保留分支 | 待回填 | 待回填 | 待回填 | 待验证 |
| AC-1.2 | spec.md | TASK-2 | listener.rs 保留分支（密钥不删） | 待回填 | 待回填 | 待回填 | 待验证 |
| AC-1.3 | spec.md | TASK-1, TASK-2, TASK-3 | keepdata_cache.rs 群组字段 + listener.rs 群组短路 | 待回填 | 待回填 | 待回填 | 待验证 |
| AC-1.4 | spec.md | TASK-2, TASK-5 | 分支不进入（diff 审查）+ 回归用例 | 待回填 | 待回填 | 待回填 | 待验证 |
| AC-1.5 | spec.md | TASK-5 | 验证用例（无代码改动） | 待回填 | 待回填 | 待回填 | 待验证 |
| AC-2.1 | spec.md | TASK-4 | 清理链路 owner 部分 | 待回填 | 待回填 | 待回填 | 待验证 |
| AC-2.2 | spec.md | TASK-4 | 复用既有 IS_PERSISTENT=false 删除条件 | 待回填 | 待回填 | 待回填 | 待验证 |
| AC-2.3 | spec.md | TASK-4 | developer 存活判定 + 群组标记清除 | 待回填 | 待回填 | 待回填 | 待验证 |
| AC-2.4 | spec.md | TASK-4 | 群组删除 + 密钥清理 | 待回填 | 待回填 | 待回填 | 待验证 |
| AC-2.5 | spec.md | TASK-4 | 解析失败/定位失败容错 | 待回填 | 待回填 | 待回填 | 待验证 |
| AC-2.6 | spec.md | TASK-1, TASK-2 | 条目刷新（同 key 覆盖） | 待回填 | 待回填 | 待回填 | 待验证 |
| AC-3.1 | spec.md | TASK-5 | 回归用例集 | 待回填 | 待回填 | 待回填 | 待验证 |
| AC-3.2 | spec.md | TASK-1, TASK-4 | 损坏重建 + 定位失败容错 | 待回填 | 待回填 | 待回填 | 待验证 |

## 首批实现边界

**首批必须实现：** keepdata.cache 存储模块、keepdata 保留分支、群组保留判定、清理链路全路径、sa_profile 注册、VM-1~VM-9 用例。
**可后置：** keepdata 字段名与 BMS 联调对齐后的最终确认（R-1/R-2，宽松解析先行）、设备端到端自动化脚本化。
**不建议延后：** 保留分支短路正确性（AC-1.4 基线兼容）、密钥联动（R-10）——延后导致兼容或不可解密风险。

## Task 粒度原则

- 每个 Task 对应一个可独立验收的最小能力闭环。
- 本变更按存储层/保留层/群组层/清理层/配置与测试层拆分为 5 张 Task Card。

## 禁止项

- [x] 无占位词、「视需求而定」类含糊指令、隐式依赖、自行寻上下文、无验证 AC、跨 Task 引用
- [x] 无跨 Task 隐式依赖（依赖在"前置依赖"列显式声明）
- [x] 无要求 Agent 自行寻找未列出的上下文文件
- [x] 无未映射到验证方式的 AC
- [x] 无"与 Task-N 类似"类引用（每个 Task 自包含）

## Task 列表

| Task ID | 目标 | 文件范围 | AC 映射 | 前置依赖 | 完成判据 | 验证命令 | 状态 |
|---------|------|----------|---------|----------|----------|----------|------|
| TASK-1 | keepdata.cache 存储模块 | `db_operator/src/keepdata_cache.rs`、`db_operator/src/lib.rs`（mod） | AC-1.1, AC-1.3, AC-2.6, AC-3.2 | design+spec Approved | 条目合并/刷新/匹配/消费单测通过；部件编译通过 | 单测 + 部件构建 | In Progress（代码完成，待构建机验证） |
| TASK-2 | keepdata 解析 + 保留分支 | `start_event.rs`、`lib.rs`（类型）、`listener.rs` | AC-1.1, AC-1.2, AC-1.4, AC-2.6 | TASK-1 | 保留分支短路验证（数据/密钥保留、插件不通知）；keepdata 缺失/false 不进分支 | 单测/端到端 + diff 审查 | In Progress（代码完成，diff 审查已过，待统一构建验证） |
| TASK-3 | 群组保留判定 | `listener.rs` | AC-1.3 | TASK-1 | 混合 keepdata 群组矩阵（任一带→保留；全不带→基线删除）通过 | 模块用例 | In Progress（代码完成，待统一构建验证） |
| TASK-4 | 清理链路 | `start_event.rs`、`listener.rs` | AC-2.1~2.5, AC-3.2（定位容错） | TASK-1 | 保留→重装→再卸载→清理端到端；豁免/群组判定/标记消费全通过 | 端到端 + 模块用例 | In Progress（代码完成，待统一构建验证） |
| TASK-5 | sa_profile 注册 + 测试矩阵 | `sa_profile/8100.json`、module_test/XTS/端到端 | AC-1.4, AC-1.5, AC-3.1（+全 AC 回归） | TASK-2, TASK-3, TASK-4 | 新事件可拉起 SA；VM-1~VM-9 用例通过；基线回归零差异 | 构建 + XTS/端到端 | In Progress（sa_profile+10 个单测已交付，设备端到端待执行） |

## Task 详情

### TASK-1: keepdata.cache 存储模块

| 字段 | 内容 |
|------|------|
| 任务目标 | 交付 keepdata 标记存储：条目结构、缓存文件读写（全局锁 + 0o640 + serde）、同 key 刷新、按 bundleName/developerId 匹配、groupIds 移除、条目删除、损坏重建 |
| AC 映射 | AC-1.1, AC-1.3, AC-2.6, AC-3.2 |
| 前置依赖 | design + spec Approved |
| 非目标 | 事件接入（归 TASK-2/4）；群组判定逻辑（归 TASK-3/4） |
| 完成判据 | 单测全绿：写入→读取一致；同 (appId, appIndex) 刷新；appIndex==0 条件字段；损坏文件→空表重建不 panic；条目删除/groupIds 移除正确 |
| 停止条件 | DE 目录权限或路径约束与 `upgrade.cache` 模式冲突时停止回 design |
| Design 状态/不变量 | STATE-标记位（keepdata.cache）；INV：条目存在 ⇒ 对应 owner 处于保留态 |

**状态所有权和生命周期**

| Design Ref | 状态 | Owner | Key / Index | 创建 | 读取/更新 | 清理触发 | 生命周期不变量 |
|------------|------|-------|-------------|------|-----------|----------|----------------|
| STATE-标记位 | keepdata.cache 条目 | `keepdata_cache.rs`（KEEPDATA_FILE_LOCK） | `(user_id, app_id, app_index)` | keepdata=true 卸载写入（TASK-2） | 清理链路匹配读取（TASK-4）；群组判定读取（TASK-3） | 清理事件消费（TASK-4）；条目刷新（AC-2.6） | 条目存在 ⇒ owner 数据保留；文件损坏 ⇒ 降级空表（最坏=清理遗漏） |

**任务间接口（Produces / Consumes）**

| Direction | Contract | Provider / Consumer | Dependency | Compatibility Constraint |
|-----------|----------|---------------------|------------|--------------------------|
| Produces | `add_keepdata_entry` / `get_keepdata_entries_by_bundle` / `get_keepdata_entries_by_developer` / `remove_keepdata_group_ids` / `remove_keepdata_entries` | TASK-2/3/4 | 标记读写 | 条目字段含 bundleName（ADR-2 偏差，join key）；developerId/groupIds 仅 appIndex==0 |

**Read-only Context**

| 路径 | 读取目的 |
|------|----------|
| `db_operator/src/database_file_upgrade.rs:287-341` | 缓存文件模式（路径/锁/权限/serde）复用约定 |
| `design.md#数据模型设计` | KeepdataEntry/KeepdataCache 结构 |

**Files**

| 操作 | 文件 | 说明 |
|------|------|------|
| Create | `services/asset/db_operator/src/keepdata_cache.rs` | 模块 + 内联单测（#[cfg(test)]） |
| Modify | `services/asset/db_operator/src/lib.rs` | `pub mod keepdata_cache;` 注册 |

**禁止修改文件**

| 文件/路径 | 原因 |
|-----------|------|
| `core_service/common_event/*` | 归 TASK-2/3/4 |
| `db_operator/src/database_file_upgrade.rs` | upgrade.cache 用途隔离，不扩展（Owner 评审结论） |
| `db_operator/BUILD.gn`、`db_operator/Cargo.toml` | 零改动（root-only sources + serde 为既有依赖） |

**Spec References**

- AC / Rules: `AC-1.1`, `AC-1.3`, `AC-2.6`, `AC-3.2`, `R-2`, `R-8`
- Anchors: `spec.md#规则定义`, `spec.md#验证映射`（VM-1 部分）

**Design References**

- IDs / Anchors: `ADR-2`, `ADR-7`, `design.md#数据模型设计`, `design.md#线程与并发模型`

**Steps**

- [x] 写失败单测：合并刷新/追加/匹配/条目删除/groupIds 移除（纯逻辑，已随模块交付）
- [x] 运行确认 Red（模块不存在，编译失败即 Red 信号）
- [x] 实现 keepdata_cache.rs（结构/锁/权限/serde/五个 API，复用 upgrade.cache 代码模式）
- [ ] 单测 GREEN；部件编译通过（待构建机执行）
- [ ] 回填追溯

**Verification**

| 证据类型 | 命令/路径 | Expected Result | Actual Result |
|----------|-----------|-----------------|---------------|
| 单测 | `./build.sh --product-name rk3568 --ccache --build-target rust_asset_db_operator_test` | 5 个单测全部通过 | 待回填（构建机） |
| 构建 | `./build.sh --product-name rk3568 --ccache --build-target asset_db_operator`（或部件整建） | 编译通过 | 待回填（构建机） |

**Anti-Fake Completion**

| Check | Required Evidence |
|-------|-------------------|
| AC closed | 刷新（AC-2.6）、条件字段（AC-1.1）单测证据；损坏降级（AC-3.2）设备用例归 TASK-5 |
| Scope respected | diff 仅 db_operator keepdata_cache.rs/lib.rs 注册 |
| State lifecycle complete | 条目创建→读取→消费全链单测 |

---

### TASK-2: keepdata 解析 + 保留分支

| 字段 | 内容 |
|------|------|
| 任务目标 | 事件入口解析 keepdata（宽松 bool）；`on_package_removed` 顶层分流：keepdata=true → 写标记 + 短路删除/密钥/插件通知；update_upgrade_list/clear_cryptos 照旧 |
| AC 映射 | AC-1.1, AC-1.2, AC-1.4, AC-2.6 |
| 前置依赖 | TASK-1（add_keepdata_entry） |
| 非目标 | 群组删除前标记命中检查（归 TASK-3）；清理事件（归 TASK-4） |
| 完成判据 | keepdata=true 卸载后：DE/CE 库文件与 HUKS 密钥存在、标记内容正确、hilog 无删除链路日志、插件无 OnPackageClear 调用；keepdata 缺失/false：diff 审查证明未进入新分支 |
| 停止条件 | want 中 keepdata 取值载体与宽松解析均无法覆盖（联调实测）时停止，回 spec R-1 |
| Design 状态/不变量 | STATE-标记位创建点；INV：keepdata=true 事件处理后数据与密钥均存在 |

**任务间接口（Produces / Consumes）**

| Direction | Contract | Provider / Consumer | Dependency | Compatibility Constraint |
|-----------|----------|---------------------|------------|--------------------------|
| Consumes | `add_keepdata_entry`（TASK-1） | TASK-2 | 标记写入 | 条目含 bundleName/appId/appIndex（+条件字段） |
| Produces | `PackageInfo.keepdata`（bool）传递链 | TASK-3/4 | 下游判定 | 缺失/false 统一归一化为 false |

**Read-only Context**

| 路径 | 读取目的 |
|------|----------|
| `start_event.rs:98-127` | handle_package_removed 解析模式 |
| `listener.rs:265-295` | on_package_removed 编排（短路插入点） |
| `design.md#时序设计`（keepdata 卸载保留图） | 步骤顺序 |

**Files**

| 操作 | 文件 | 说明 |
|------|------|------|
| Modify | `services/asset/core_service/src/common_event/start_event.rs` | KEEP_DATA 常量 + 宽松解析 + PackageInfo 传递 |
| Modify | `services/asset/core_service/src/lib.rs` | PackageInfo/PackageInfoFfi 增 keepdata 字段 |
| Modify | `services/asset/core_service/src/common_event/listener.rs` | on_package_removed 顶层分流 + 标记写入 + 日志 |

**禁止修改文件**

| 文件/路径 | 原因 |
|-----------|------|
| `delete_data_by_owner`/`delete_on_package_removed` 内部 | 基线删除内核零改动（ADR-1 顶层短路） |
| `db_operator/*` | 零改动 |

**Spec References**

- AC / Rules: `AC-1.1`, `AC-1.2`, `AC-1.4`, `AC-2.6`, `R-1`, `R-6`
- Anchors: `spec.md#用户故事`（US-1）, `spec.md#规则定义`

**Design References**

- IDs / Anchors: `ADR-1`, `ADR-6`, `design.md#时序设计`

**Steps**

- [ ] 写失败用例：keepdata=true 卸载后 query/文件/密钥断言（模块或端到端桩）
- [ ] 确认 Red（当前卸载即删）
- [ ] 实现解析 + 分流 + 标记写入
- [ ] GREEN；keepdata=false 全链路回归（与基线 diff 行为一致）
- [ ] 回填追溯

**Verification**

| 证据类型 | 命令/路径 | Expected Result | Actual Result |
|----------|-----------|-----------------|---------------|
| 端到端/模块 | VM-1 场景（设备或 mock 事件入口） | 库/密钥/标记保留；重装 query resolve | 待回填 |
| 回归 | keepdata 缺失/false 卸载用例 | 与基线一致（AC-1.4） | 待回填 |
| diff 审查 | 基线路径代码审查 | 新分支不进入（R-6） | 待回填 |

---

### TASK-3: 群组保留判定

| 字段 | 内容 |
|------|------|
| 任务目标 | keepdata=false 卸载且 `GetUninstallGroups` 判定群组可删时，删除前查标记：同 developerId 且 groupIds 覆盖该群组的条目存在 → 跳过该群组删除（保留数据与群组密钥） |
| AC 映射 | AC-1.3 |
| 前置依赖 | TASK-1（get_keepdata_entries_by_developer） |
| 非目标 | keepdata=true 短路（归 TASK-2，群组随整链短路）；清理事件群组判定（归 TASK-4） |
| 完成判据 | 群组矩阵通过：Hap-A(keepdata)+Hap-B(无) 全卸载→群组保留；全无 keepdata→基线删除；标记命中时群组 HUKS 密钥不删 |
| 停止条件 | GetUninstallGroups 返回语义无法支撑"逐群组跳过"时停止回 design |
| Design 状态/不变量 | INV：群组数据被保留 ⇒ 存在覆盖该群的标记条目（或群组未全卸载基线态） |

**任务间接口（Produces / Consumes）**

| Direction | Contract | Provider / Consumer | Dependency | Compatibility Constraint |
|-----------|----------|---------------------|------------|--------------------------|
| Consumes | `get_keepdata_entries_by_developer`（TASK-1） | TASK-3 | 标记命中检查 | 匹配键：developerId + groupId 交集 |

**Read-only Context**

| 路径 | 读取目的 |
|------|----------|
| `listener.rs:169-219` | construct_calling_infos 群组构造（命中检查插入点） |
| `design.md ADR-3` | 群组保留判定规则 |

**Files**

| 操作 | 文件 | 说明 |
|------|------|------|
| Modify | `services/asset/core_service/src/common_event/listener.rs` | 群组删除前标记命中检查（跳过该 calling_info 的删除与密钥清理） |

**禁止修改文件**

| 文件/路径 | 原因 |
|-----------|------|
| `db_operator/*`、`bms_wrapper.cpp` | 零改动（复用） |

**Spec References**

- AC / Rules: `AC-1.3`, `R-3`
- Anchors: `spec.md#规则定义`（R-3）

**Design References**

- IDs / Anchors: `ADR-3`, `design.md#状态归属与不变量`

**Steps**

- [ ] 写失败用例：混合 keepdata 群组场景（VM-2 扩展）
- [ ] 确认 Red（当前群组按基线删除）
- [ ] 实现命中检查
- [ ] GREEN（含全无 keepdata 回归）
- [ ] 回填追溯

**Verification**

| 证据类型 | 命令/路径 | Expected Result | Actual Result |
|----------|-----------|-----------------|---------------|
| 模块/端到端 | VM-2 场景 | 群组保留+标记存在；全无 keepdata→基线删除 | 待回填 |

---

### TASK-4: 清理链路（PACKAGE_UNINSTALLED_DATA_CLEARED）

| 字段 | 内容 |
|------|------|
| 任务目标 | 新事件路由与解析 → 按标记定位 owner → 复用 `delete_on_package_removed` 删除（IS_PERSISTENT=false 条件与 TrustedAccount 反条件已天然满足）→ DataExist 语义密钥清理 → developer 存活判定群组处理 → 标记消费 |
| AC 映射 | AC-2.1, AC-2.2, AC-2.3, AC-2.4, AC-2.5, AC-3.2（定位容错） |
| 前置依赖 | TASK-1（get_keepdata_entries_by_bundle/remove_keepdata_entries/remove_keepdata_group_ids） |
| 非目标 | sa_profile 注册与全量回归（归 TASK-5）；keepdata 解析（归 TASK-2） |
| 完成判据 | 保留→重装→再卸载→清理事件端到端：非 is_persistent 数据删除、is_persistent 保留、密钥按残留判定、群组两分支正确、标记消费、参数缺失容错 |
| 停止条件 | 清理事件 want 实际参数与设计假设（bundleName/developerId/assetAccessGroups）不符时停止，回 spec R-2 联调对齐 |
| Design 状态/不变量 | STATE-标记位消费点；INV：清理后条目删除；定位失败仅日志 |

**任务间接口（Produces / Consumes）**

| Direction | Contract | Provider / Consumer | Dependency | Compatibility Constraint |
|-----------|----------|---------------------|------------|--------------------------|
| Consumes | `get_keepdata_entries_by_bundle`/`remove_keepdata_entries`/`remove_keepdata_group_ids`（TASK-1） | TASK-4 | 定位与消费 | 定位用标记内 appId（卸载时点值） |
| Consumes | `delete_on_package_removed`（基线，listener.rs:142） | TASK-4 | 删除内核 | IS_PERSISTENT=false + TrustedAccount 反条件（listener.rs:105/144）天然满足 AC-2.2 |
| Consumes | `GetUninstallGroups`/`GetAllBundleInfoByDeveloperId`（bms_wrapper.cpp:335） | TASK-4 | developer 存活判定 | 群组两分支（AC-2.3/2.4） |

**Read-only Context**

| 路径 | 读取目的 |
|------|----------|
| `listener.rs:101-162` | 删除内核复用方式（含 IS_PERSISTENT=false 条件） |
| `design.md#时序设计`（清理事件图） | 步骤顺序 |

**Files**

| 操作 | 文件 | 说明 |
|------|------|------|
| Modify | `services/asset/core_service/src/common_event/start_event.rs` | 新事件分支 + want 解析（复用常量风格） |
| Modify | `services/asset/core_service/src/common_event/listener.rs` | `on_uninstall_data_cleared` 编排（定位/删除/密钥/群组/标记消费/容错日志） |

**禁止修改文件**

| 文件/路径 | 原因 |
|-----------|------|
| `db_operator/*`、`bms_wrapper.cpp`、`crypto_manager/*` | 零改动（全部复用） |
| `sa_profile/8100.json` | 归 TASK-5 |

**Spec References**

- AC / Rules: `AC-2.1`~`AC-2.5`, `AC-3.2`, `R-4`, `R-5`, `R-7`, `R-10`
- Anchors: `spec.md#用户故事`（US-2）, `spec.md#规则定义`

**Design References**

- IDs / Anchors: `ADR-4`（修正版）、`ADR-5`, `design.md#时序设计`（清理事件图）, `design.md#状态归属与不变量`

**Steps**

- [ ] 写失败用例：清理事件场景（VM-4/5/6/7 对应）
- [ ] 确认 Red（事件无处理分支）
- [ ] 实现路由 + 编排（含容错日志：缺失字段名输出）
- [ ] GREEN（含 is_persistent 豁免、群组两分支、标记消费断言）
- [ ] 回填追溯

**Verification**

| 证据类型 | 命令/路径 | Expected Result | Actual Result |
|----------|-----------|-----------------|---------------|
| 端到端/模块 | VM-4/5/6/7 场景 | 清理正确、豁免生效、群组判定正确、容错不中断 | 待回填 |

---

### TASK-5: sa_profile 注册 + 测试矩阵

| 字段 | 内容 |
|------|------|
| 任务目标 | 注册新事件按需拉起；补齐 VM-1~VM-9 全量用例与基线回归 |
| AC 映射 | AC-1.4, AC-1.5, AC-3.1（+全 AC 回归） |
| 前置依赖 | TASK-2, TASK-3, TASK-4 |
| 非目标 | 新功能代码（归前序） |
| 完成判据 | 清理事件可拉起 SA（配置生效）；VM-1~VM-9 全通过；现网卸载回归零差异 |
| 停止条件 | sa_profile 事件订阅格式与本仓配置体系不兼容时停止回 design |

**任务间接口（Produces / Consumes）**

| Direction | Contract | Provider / Consumer | Dependency | Compatibility Constraint |
|-----------|----------|---------------------|------------|--------------------------|
| Consumes | 全部前序 Task 交付能力 | TASK-5 | 验证对象 | 无代码契约 |

**Read-only Context**

| 路径 | 读取目的 |
|------|----------|
| `sa_profile/8100.json:14-15` | events 数组格式 |
| `spec.md#验证映射` | VM-1~VM-9 用例定义与 Red 条件 |

**Files**

| 操作 | 文件 | 说明 |
|------|------|------|
| Modify | `sa_profile/8100.json` | events 新增 `usual.event.PACKAGE_UNINSTALLED_DATA_CLEARED`（已交付） |
| Modify | `services/asset/core_service/src/common_event/start_event.rs`（#[cfg(test)] 解析单测 5 个） | keepdata 宽松解析/清理事件 want 解析单测（已交付） |
| Create | 设备端到端 VM 执行步骤（本 Task Card 内） | 事件驱动行为无法在仓内自动化（无事件触发设施），VM-1~VM-9 以设备端到端执行 |

**禁止修改文件**

| 文件/路径 | 原因 |
|-----------|------|
| `common_event/*`、`db_operator/keepdata_cache.rs` | 归前序 Task |

**Spec References**

- AC / Rules: `AC-1.4`, `AC-1.5`, `AC-3.1`, `R-6`, `R-11`
- Anchors: `spec.md#验证映射`

**Design References**

- IDs / Anchors: `design.md#构建系统影响`, `design.md#测试性设计`

**Steps**

- [x] sa_profile 注册（+1 行，git diff 验证）
- [x] start_event.rs 解析单测（keepdata true/false/缺失、群组解析、清理事件解析/缺参容错，共 5 个）
- [x] keepdata_cache.rs 存储单测（TASK-1 已交付 5 个）
- [ ] 构建验证 + 单测执行（统一上构建机）：`./build.sh --product-name rk3568 --ccache --build-target rust_asset_db_operator_test rust_asset_core_service_test`
- [ ] 设备端到端 VM 执行（下表步骤）
- [ ] 基线回归（不带 keepdata 卸载全集）
- [ ] 回填追溯与 Review Evidence

**VM 设备端到端执行步骤**（hdc，需 BMS 卸载保留特性联调版本配合）：

| VM | 步骤 |
|----|------|
| VM-1（R-1/R-2） | ① 测试 HAP add 关键资产（secret）→ ② 设备卸载并勾选保留（keepdata=true 事件）→ ③ `hdc shell ls /data/service/el1/public/asset_service/100/` 断言 `Hap_*.db` 与 `keepdata.cache` 存在 → ④ 重装 → ⑤ query 断言 resolve 且 secret 一致 |
| VM-2（R-3） | 群组场景：Hap-A（keepdata）+Hap-B（无）先后卸载 → 断言群组 `Group_*.db` 保留且 `keepdata.cache` 含 gid 条目 |
| VM-3（R-6） | keepdata 缺失/false 卸载（正常卸载不勾选）+ SANDBOX 卸载 → 断言行为与基线一致（db 删除、query 24000002、hilog 无保留日志） |
| VM-4（R-4） | VM-1 完成后重装→再次卸载（不勾选）→ 触发清理事件 → 断言 `keepdata.cache` 条目消失、重装后 query 24000002 |
| VM-5（R-4） | 库内含 `is_persistent=true` 记录的 owner 走 VM-4 → 断言该记录仍可查询、其余被清理 |
| VM-6（R-5） | 同 developer 两应用（群组共享）：A 保留卸载→B 正常卸载→触发清理事件 → B 在装时断言群组保留；B 也卸载后触发 → 断言群组清理 |
| VM-7（R-7/R-8） | 手工损坏/删除 `/data/.../keepdata.cache` → 触发事件 → 断言 hilog 记录告警、服务与 query 正常 |
| VM-8（R-11） | 保留期间第三方应用 query → 断言第三方无法获取（行为与基线一致，不钉错误码） |
| VM-9（R-3/R-4/R-5，多群组交叉矩阵） | 前置：app1（群组 A/B）与 app2（群组 A/C）同 developer，各自向所属群组写入 `IS_PERSISTENT=false` 数据。① **app1 keepdata 卸载**：断言 app1 自身数据与群组 A、B 数据及密钥保留，`keepdata.cache` 条目 groupIds=[A,B] → ② **app2 普通卸载**：断言 app2 自身数据与**群组 C** 数据及密钥删除（C 无保留凭据），**群组 A 保留**（app1 标记覆盖命中检查）→ ③ **app1 重装再卸载触发清理**：断言 app1 自身数据与群组 A、B 数据及密钥删除，标记条目消费。三步均断言 `Group_*.db`/HUKS 群组密钥与 keepdata.cache 状态（含条目 groupIds 变化） |

**Verification**

| 证据类型 | 命令/路径 | Expected Result | Actual Result |
|----------|-----------|-----------------|---------------|
| 单测 | `./build.sh --product-name rk3568 --ccache --build-target rust_asset_db_operator_test rust_asset_core_service_test` | keepdata_cache 5 用例 + start_event 5 用例全部通过 | 待回填（构建机） |
| 构建 | 部件整建 | 编译通过 | 待回填（构建机） |
| 端到端 | VM-1~VM-9（上表步骤） | 全部通过 | 待回填（设备） |
| 回归 | 现网卸载用例集 | 零差异（AC-3.1） | 待回填（设备） |

---

## 代码范围映射

> Plan 审批时填写计划范围；实现后如实回填实际范围和 Commit。

| Task ID | 计划文件/符号 | 实际文件/符号 | 操作 | Commit | Spec Compliance | Code Quality | Verification | 偏差 |
|---------|---------------|---------------|------|--------|-----------------|--------------|--------------|------|
| TASK-1 | `db_operator/src/keepdata_cache.rs`、`db_operator/src/lib.rs`（mod） | `db_operator/src/keepdata_cache.rs`（约 240 行，含 5 个内联单测）、`db_operator/src/lib.rs`（pub mod 注册） | Add/Modify | 待回填 | 待回填 | 待回填 | 待回填 | 见偏差表（实现位置经两轮评审调整） |
| TASK-2 | `start_event.rs`、`lib.rs`（类型）、`listener.rs` | `start_event.rs`（KEEP_DATA 常量/宽松解析/传参）、`lib.rs`（两结构体 +keepdata 字段）、`listener.rs`（retain_data_on_keepdata_uninstall + 守卫分支） | Add/Modify | 待回填 | 待回填 | 待回填 | 待回填 | diff 审查已过（+88/-1 零格式连带） |
| TASK-3 | `listener.rs`（群组命中检查） | `listener.rs`（is_group_retained_by_keepdata + delete_data_by_owner 循环守卫，+30 行） | Modify | 待回填 | 待回填 | 待回填 | 待回填 | 无（手术式插入，零格式连带） |
| TASK-4 | `start_event.rs`、`listener.rs`（清理链路） | `start_event.rs`（PACKAGE_UNINSTALLED_DATA_CLEARED 分支 + UninstallDataClearedWant 解析 + handle_uninstall_data_cleared）、`listener.rs`（on_uninstall_data_cleared + clean_group_data_on_data_cleared） | Modify | 待回填 | 待回填 | 待回填 | 待回填 | 无（删除内核/密钥处理/群组构造全部复用基线函数，手术式插入） |
| TASK-5 | `sa_profile/8100.json`、测试用例 | `sa_profile/8100.json`（+1 行事件注册）、`start_event.rs`（+71 行 5 个解析单测）；VM 端到端步骤落位于本 Task Card | Modify/Add | 待回填 | 待回填 | 待回填 | 待回填 | 事件驱动行为无仓内自动化设施，VM 以设备端到端执行（步骤已固化） |

## 实现偏差和开放问题

| Task ID | 类型 | 问题/偏差 | 处理状态 | 回退能力/Owner |
|---------|------|-----------|----------|----------------|
| 全部 | 外部对齐 | ~~keepdata 字段名与清理事件参数名联调核对（design R-1/R-2）~~ **已闭环（2026-11-17，对照 bundlemanager_bundle_framework-master 源码 `services/bundlemgr/src/bundle_common_event_mgr.cpp`）**：①keepdata 实际键为 **`keepData`（驼峰，bool 类型）**——两侧统一单键解析（Owner 决策：BMS 无小写定义即不兼容小写）；②清理事件 want **无 userId 只有 uid**（换算调用既有接口 `GetUserIdByUid`→`OsAccountManager::GetOsAccountLocalIdFromUid`，Rust 侧复用 `asset_common::get_user_id`，两侧已加兜底），bundleName 走 `SetElement`（C++ 侧 GetBundle 兜底已覆盖）；③事件 action 字符串 **无 CommonEventSupport 常量**——BMS 自身即本地 constexpr（cpp:74），仓内本地常量与发布方模式一致（Owner 确认方向正确）；④清理事件实际还携带 appId/appIndex（超出 Q-3 描述，当前标记定位链路不受影响，留作直连优化备选） | 已闭环 | yin yaode |
| TASK-4 | design | ADR-4 修正：豁免条件基线已存在（listener.rs:105），删除内核直接复用（行为语义不变） | 已记录 | yin yaode |
| TASK-1 | 流程 | proposal baseline_digest 占位（本地无 ohos-sdd 工具，Owner 决定忽略） | 已确认忽略 | yin yaode |
| TASK-1 | design | 标记存储实现位置经两轮 Owner 评审调整：初稿独立模块于 core_service（新增 serde 依赖、独立锁，被评"过复杂"）→ 二稿扩展 UpgradeData 混入 upgrade.cache（被否：**用途完全不同**）→ 终稿 `db_operator/keepdata_cache.rs`：独立 keepdata.cache 文件 + 完全复用既有代码模式（get_file_content/save_to_writer 同构、0o640、serde 既有依赖），core_service 构建零改动 | 已按终稿实现 | yin yaode |
| TASK-2 | design（重要） | **设备测试发现 FFI 断裂**：事件存在双通路（samgr 按需 reason→Rust start_event；C++ SystemEventHandler 订阅→system_event_wrapper.cpp 直接回调 `on_package_removed`）。TASK-2 给 Rust `PackageInfoFfi` 加 keepdata 字段时**漏改 C++ 镜像结构**（system_event_wrapper.h 的 PackageInfo 仍 6 字段）→ C++ 通路 keepdata 读越界内存（实测=false）→ 保留后仍被基线删除链清掉数据与密钥。修复：h 加 `bool keepdata;`（末字段，与 Rust 布局一致）+ cpp 解析 keepdata（GetBoolParam+字符串宽松回退，sandbox 强制 false）并传入回调结构。design 调用链层级分析漏列该 C++ 通路，已补 | 已修复（2026-11-17 设备验证发现） | yin yaode |
| TASK-2/3 | spec | **Owner 语义澄清（2026-11-17）**：基线卸载（keepdata=false）需先消费本包保留标记（需求原文"等下次该 hap 再卸载时如果 keepdata 没传，则按照老的逻辑处理"）。新增 `remove_keepdata_entries_by_bundle` API 并在 `on_package_removed` 基线路径 `delete_data_by_owner` 之前调用：①标记不指向已删数据；②本包群组保护失效回落老逻辑（其他 Hap 标记不受影响，Q-11 语义保留）；③清理事件丢失时不再泄漏群组数据/密钥/标记。spec R-3/R-4 已同步细化 | 已实现 | yin yaode |
| TASK-4 | design（重要） | **设备二轮测试发现通路缺口（2026-11-17）**：SA 已存活时（如被业务调用唤醒），samgr 按需 reason 通路不触发，事件仅经 C++ SystemEventHandler 订阅通路投递——而该通路**无 PACKAGE_UNINSTALLED_DATA_CLEARED 分支**（当 unknown event 丢弃）→ 清理事件在 SA 存活态永不执行。修复：system_event_wrapper.h/cpp 新增 `OnUninstallDataCleared` 回调（EventCallBack 末字段，Rust/C++ 两侧同步）+ `HandleUninstallDataCleared`（want 解析 userId/bundleName/developerId/groupIds）+ OnReceiveEvent 分支；listener.rs 增加 `on_uninstall_data_cleared_ffi` FFI 包装并注册。另在 C++ keepdata 解析处增加诊断日志（Keepdata parsed: bool/str 原始值） | 已实现（待设备复测） | yin yaode |
| TASK-4 | design | **分身场景三轮测试发现空删竞态（2026-11-18）**：清理事件双投递（samgr 按需+C++ 订阅器并发消费同一事件，日志双线程实证），两线程同时对空缓存执行 remove_file——败者收到 ENOENT 被旧逻辑误判为"删除失败"而兜底写回空文件（keepdata.cache 以 `{"entries":[]}` 复活）。修复：save_to_writer 空分支改 match——Ok 与 ENOENT（并发已删=目标状态）均直接返回，仅真实 IO 失败才落盘空缓存（防过期条目残留） | 已实现（待设备复测） | yin yaode |
| TASK-4 | design（重要） | **分身场景四轮测试发现清理粒度缺陷（2026-11-18）**：主应用触发清理事件时，按 bundleName 消费命中了**分身的保留条目**（其自身保留周期未结束）→ 分身数据/HUKS 密钥被误删。修复：清理按事件携带的 **appId/appIndex**（BMS 实证字段，此前被当作"备选信息"忽略）精确收敛到触发应用自身的条目，其余条目不动；C++ 侧 OnUninstallDataCleared 扩签名传 appId/appIndex，Rust 两通路解析传递并过滤 targets；事件缺省时回退 bundle 全量（旧 BMS 兼容）。review Q-002/P-002 答案同步修正 | 已实现（待设备复测） | yin yaode |
| TASK-2/3 | design（重要） | **分身场景五轮测试发现基线消费粒度缺陷（2026-11-18）**：分身1 keepdata 卸载后，同 bundle 的分身2/主应用普通卸载时基线执行 `remove_keepdata_entries_by_bundle`（按包名消费）→ **分身1 的标记被兄弟应用误消费**（缓存空→文件删除）。修复：基线消费改为 `remove_keepdata_entry(appId, appIndex)`——仅消费**本次卸载应用自身**的条目（需求原文"该 hap"粒度），同 bundle 其他应用标记与数据不受影响；keepdata_cache API 与单测同步替换。spec R-3 措辞同步（appId/appIndex 精确匹配） | 已实现（待设备复测） | yin yaode |
| TASK-3 | design（严重） | **六轮测试发现群组保留守卫从未生效（2026-11-18）**：`is_group_retained_by_keepdata` 调用了 `CallingInfo::group()`——该 API 返回 `developer_id+分隔符+group_id` 的**拼接串**（calling_info.rs:120-132），与标记中的裸 group_id 比对**恒不相等** → 群组保留检查恒 false → 群组数据照删（此前设备日志中 "Group data is retained" 从未出现，历史"群组保留"现象均来自 BMS modify=true 而非本守卫）。修复：改用 `group_id()`（裸群组 ID，calling_info.rs:113），签名适配 `Option<&Vec<u8>>`；保留诊断日志 | 已实现（待设备复测） | yin yaode |
| review | 证据 | **Q-001/002/006 证据升级（2026-11-18）**：BMS 契约证据补强为可核查级——新增 `evidence/bms-event-contract.md`（仓库身份+文件 SHA256+精确行号+grep 复验命令；keepData 驼峰/bool 实证 L53/L334/L711；清理事件 appId/appIndex L709/L712、uid 无 userId L705、bundleName 仅 SetElement L701）。遗留：本地快照无 commit hash，commit 级引用或正式契约文档需 BMS Owner 提供（已列依赖风险，非阻断） | 已交付 | yin yaode |
| review | 规格 | **Q-003 裁决（2026-11-18）**：R-10 增补失败路径例外——HUKS 删除失败时接受**孤立密钥残留**（密钥无数据=无明文泄露）+fault 事件可观测；禁止反向顺序（先删密钥将造成不可解密敏感数据残留）。"严禁数据已删而密钥残留"约束明确限定成功路径。spec R-10 已更新 | 已闭环 | yin yaode |
| review | 规格 | **Q-008 裁决（2026-11-18）**：密钥状态 oracle 固化为**数据残留矩阵+决策日志行**（spec 新增"密钥状态验收 oracle"节）——单独 query=24000002 仅证明非豁免记录不存在；密钥断言必须组合完整残留矩阵（豁免/云数据存在⇒密钥留，日志 "Do not delete..."；全无⇒密钥删，日志 "Delete ... key"） | 已闭环 | yin yaode |
| review | 措辞 | **Q-013/Q-021 口径确认（2026-11-18）**：登记为"接受残留风险"（非阻断），所有文档均以"残留"表述（数据多保留/CE 残留/孤立密钥），无任何"无残留"结论表述；复核 spec R-8/R-10 与 plan 偏差表措辞一致 | 已确认 | yin yaode |
| TASK-1 | 检视 | **代码检视两项采纳（2026-11-18，置信度 10）**：① save_to_writer 的 truncate-先清空再写存在中途失败丢原文件风险——改为**原子写**（临时文件 + fs::rename，失败时目标文件保持旧状态；写失败顺带清理 .tmp；读取时清理崩溃残留 .tmp）；② 锁仅护读/写各一段、读-改-写整体不原子（双投递并发=丢更新风险）——**锁上提至全部 6 个公开函数**覆盖完整周期，内部拆出 `_unlocked` 版本（get_file_content_unlocked / save_to_writer_unlocked，doc 注明调用方须持锁） | 已实现 | yin yaode |
