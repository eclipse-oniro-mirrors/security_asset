# Check: Spec（Specify 阶段检查证据）

| 项 | 值 |
|----|-----|
| Change | `.codespec/changes/20260905541319` |
| Spec | `spec.md` |
| 检查日期 | 2026-10-24 |
| 检查方式 | ohos-sdd CLI `validate --level A/B/C/D` + ohos-spec skill checklist 逐项核对 |
| 检查人 | Agent（ohos-sdd）+ Owner 人工批准 |
| Profile | none |

## Spec 编写与审校过程

1. **输入来源**：proposal.md（Approved，baseline_digest `48e4c5d2...` 一致）+ 权威 API 声明 `D:\@ohos.abilityToolAccessCtrl.d.ts` + 代码基线 `7e1109015c5941f014d9cf0421f528351a3b9c07`。
2. **代码基线核查**：确认 task identity 能力（`generateTaskChallenge`/`generateTaskIdentity`/`revokeTaskIdentity`/`RetrieveAuthTokenByTaskId`、IDL ipccode 12-15、内部码 0x71005-0x71008）在基线中**不存在**，全部为本次新增；基线仅含 ticket 链路与远程控制链路。粗糙草案（`task-level-ticket.spec.md`/`task-identity-verify-refactor.spec.md`）不作为实现事实来源。
3. **语义澄清（Owner 2026-10-24）**：
   - 锁屏豁免条件 = `allowScreenLockExecution=true` + 有效 ATL + 该 ATL 有锁屏策略 + 请求权限不在拒绝列表；**ATL3(30000)/ATL4(40000) 为有效豁免强度，当前拒绝列表为空 → 放行**；返回 24010006（`OPERATION_FAILED_UNDER_SCREEN_LOCK`，内部 0x22002）。
   - verifyTicket 链上任务校验：一次性（INSTANT）任务凭证校验并执行后不可再次通过，延时（DELAYED）可重复通过。
   - `RetrieveAuthTokenByTaskId` 为本次新增 InnerAPI（系统侧任务有效性校验）。
4. **内部实现细节治理**：从 spec 剥离 `verify_task_identity_and_consume`/`delete_task_entry`/`verify_ticket_impl`/`GetAtlPermissionPolicy` 等内部函数名与机制，仅保留三层接口（Public/System/InnerAPI）可观察语义；内部机制归 design.md。
5. **验证映射保证方式**：JS/ArkTS 入口接口由 XTS 测试用例或测试 HAP 保证；C++ 入口 InnerAPI（`RetrieveAuthTokenByTaskId`）由 TDD 单测（HWTEST_F）保证。

## Validate 结果

| 检查（针对 spec） | 结果 |
|------|------|
| Level B 结构（`# 特性规格`/`## 验收追溯`/`## API 变更分析`） | ✅ 通过 |
| Level C spec-quality（AC WHEN/THEN 可观察、验收追溯、VM-* 验证映射、API 变更分析、API 与错误码事实） | ✅ 通过 |
| Level C AC 完整性 | ✅ 20 个 AC（AC-1.1~AC-4.4）全部识别 |
| 新增 API ↔ 事实表名称一致性 | ✅ 通过 |
| 错误码事实溯源（`.d.ts` / `saf_result_defs.h:92-95` / `napi_common.cpp:52`） | ✅ 通过 |

### THEN WARN 处理记录

| AC / WARN | 结论（移入 Design / 保留外部行为 / ext-ok 豁免） | 理由与证据 | 确认人 |
|-----------|---------------------------------------------------|------------|--------|
| AC-2.4 | ext-ok 豁免（保留外部行为文案） | THEN"需解锁的 CLI/权限"中"解锁/未解锁"指**设备屏幕锁定状态**（`needUnlockScreen` 终端用户可观察语义，锁屏态行为判定），非内部加锁/互斥锁；属词法启发式误报，已在 AC-2.4 行尾添加 `<!-- ext-ok -->` 标记 | Lin Shen（Owner，2026-10-24） |

> 机器校验记录见命令输出：`ohos-sdd validate . --level C`（2026-10-24 执行）：**PASS — 0 broken / 1 warn**（AC-2.4 词法误报经 ext-ok 豁免并记录于上表）。Level A/B 结构锚点、Level C 依赖边全部通过；Level D（归档就绪）要求 `manifest.status=done/archived`，当前 `verifying` 属归档流程决策（ohos-validate 归档门禁），不阻塞 spec 有效性。历史：初始 16 broken → 修复后 spec 相关为 0；剩余 D 级门禁归归档流程。

## 审批确认（Spec 评审结论）

Owner Lin Shen 于 2026-10-24 确认以下项：

| # | 确认项 | 结论 |
|---|--------|------|
| C-1 | `@since`/目标发行版本 | 26.1.0 与版本规划一致（OH 7.1↔API 26.1.0） |
| C-2 | `interface_sdk-js` 声明落地点 | 声明已合入（MR 36745），签名与权威 `.d.ts` 一致 |
| C-3 | `allowableDuration` 边界口径 | 保持 60：约束 `(0,60]` 即最终 |
| C-4 | SIG 协同 / SELinux | SELinux 配置在 `selinux_adapter` 仓完成，非本仓交付 |
| C-5 | 锁屏豁免 ATL 扩展 | 本规格即当前版本最终规格，无补充 |

上述结论已回填 spec.md「审批确认记录」章节；spec 概述状态置为 `Approved`。

## Verdict

- [x] Approved

| 字段 | 值 |
|------|-----|
| Approver | Lin Shen（Owner） |
| Evidence | spec.md「审批确认记录」（C-1~C-5 全部确认）；ohos-sdd validate spec 相关 0 broken |
| 结论 | Spec 已批准；可进入 Design（下游依赖满足：proposal Approved + spec Approved） |