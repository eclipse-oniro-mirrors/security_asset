# Check: Proposal（Define 阶段检查证据）

| 项 | 值 |
|----|-----|
| Change | `.codespec/changes/20260905541319` |
| Proposal | `proposal.md` |
| 检查日期 | 2026-10-24 |
| 检查方式 | ohos-sdd CLI `validate --level A/B/C` + ohos-propose skill checklist 逐项核对 |
| 检查人 | Agent（ohos-sdd） + Owner 人工批准 |
| Profile | none（待 Owner/SIG 最终确认后更新 manifest） |

## Proposal 初始检视发现（2026-10-24）

对用户提交的原始 proposal.md（ODK 模板风格）执行文档级检视，发现：

| 严重度 | 问题 |
|--------|------|
| P0 | 无基线批准记录（`status: Draft`，缺 `phase_status`/`approval`/`baseline_digest` 结构） |
| P0 | API/SDK=是 但未记录 `interface_sdk-js` 声明仓路径到 Agent Scope Guard 知识源授权 |
| P1 | 开放问题 Q-1（指令风险等级分级口径）直击核心价值（目标 2/AC-4），未给出来源方案 |
| P1 | 跨 SIG 待确认未关闭 |
| P1 | 下游 spec/design/execution-plan 为模板骨架，未填内容 |
| P1 | 与 ohos-sdd 机器校验契约不匹配（缺 `phase_status` 等字段，Level B/C 会 FAIL） |
| P2 | 无背景证据引用、未与既有 task 规格建立追溯、无 manifest.md 等 |

## 修复动作

1. **模板迁移**：proposal.md 由 ODK 模板迁移到新版 ohos-sdd 模板（需求输入/需求基线结构），全部原有实质内容保留。
2. **补齐 frontmatter**：`phase_status`（AwaitingApproval → approved）、`approval`（status/approver/evidence/approved_at/baseline_digest）、`target_release` object 结构。
3. **补齐待确认项**（Owner 提供，2026-10-24）：
   - `interface_sdk-js` 声明仓：`https://gitcode.com/openharmony/interface_sdk-js`
   - Q-1 分级口径来源：useriam `AuthTrustLevel`（`js-apis-useriam-userauth.md#authtrustlevel8`）
   - 基线确认：Owner/确认人 Lin Shen，依据"功能设计评审"，结论"通过"
   - 用户反馈来源：贾维斯EWP
4. **审批**：Owner Lin Shen 于 2026-10-24 明确批准 Define；`approval.baseline_digest = 48e4c5d21eda88551dd1178c7e06d2f448dc1900c9418d91a8840efaf0e3935e`（粗糙草案引用清理后重算）。
5. **死占位清理**：附录"待补充"改写为真实状态表述（Q-2/Q-3 保持待澄清语义）。

## Validate 结果对比

| 检查 | 修复前 | 修复后 |
|------|--------|--------|
| Level A 结构 | 5 present / 3 missing（required 0 缺失） | 同左 |
| Level B proposal | **7 项 broken**（缺结构标题×3、phase_status 非法、approval 记录缺失等） | **0 broken** |
| Level C proposal→spec | Define 未批准、digest 缺失、需求基线计数=0 | 仅剩取决于下游 spec 状态的项请以最新 validate 为准 |
| Level D proposal 占位 | `['待定', '待补充']` | 已消除 |
| 总计 | 16 broken | 14 broken（proposal 相关清零，剩余为 spec/design/execution-plan/spec-for-validation/registry 骨架状态） |

> 最新机器校验记录见命令输出：`ohos-sdd validate . --level all --json`（2026-10-24 执行）。

## Verdict

- [x] Approved

| 字段 | 值 |
|------|-----|
| Approver | Lin Shen（Owner） |
| Evidence | 功能设计评审（2026-10-24）；ohos-sdd validate 前后对比；用户明确批准 Define |
| 结论 | Proposal 需求基线已批准；进入 Specify 前状态机= `phase_status: approved` |