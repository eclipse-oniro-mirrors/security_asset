# Check: Design（Design 阶段检查证据）

| 项 | 值 |
|----|-----|
| Change | `.codespec/changes/20260905541319` |
| Design | `design.md` |
| 检查日期 | 2026-10-24 |
| 检查方式 | ohos-sdd CLI `validate --level A/B/C/D` + ohos-design skill checklist 逐项核对 |
| 检查人 | Agent（ohos-sdd）+ Owner 人工批准 |
| Profile | none |

## Design 编写与审校过程

1. **上游输入**：proposal.md（Approved，baseline_digest `48e4c5d2...` 一致）+ spec.md（Approved，20 AC）。
2. **代码基线**：`7e1109015c5941f014d9cf0421f528351a3b9c07`；本次新增能力由提交 `cd731e5b` + `fc582502` 承载。经 `git show 7e110901:<path>` 逐一核对基线现状，明确"基线现状 vs 本次新增"边界。
3. **基线与新增分离（审校重点修正）**：
   - 代码事实基线：仅记录基线 `7e110901` 已有现状（ticket 双 challenge HMAC、verify_ticket_impl 链路、check_screen_lock、IDL、错误码映射等 11 条），作为本次设计约束。
   - 既有模式复用：仅列基线可复用模式（5 条）并记录 no-prior-art 检索结论；本次新增抽象（StorageBackend/LruMap）归 ADR-4。
   - 已纠正初稿将本次新增实现误当作"基线事实/既有复用"的表述错误。
4. **时序设计补全**：覆盖挑战生成（T1）、身份签发双路径（T2）、任务有效性校验（T3）、requestToolPermissions 锁屏豁免（T4）、verifyTicket 执行消费（T5）五条全生命周期链路。
5. **Owner 确认**（2026-10-24）：批准设计；D-1 `threat-model.md` 生成 / D-2 `MAX_ALLOWABLE_DURATION=86400→(0,60]` 对齐 / D-3 磁盘 authToken 存储复核列为跟进项。

## Validate 结果

| 检查（针对 design） | 结果 |
|------|------|
| Level C 代码事实基线（每行 `文件:行` 引用 + 转化为设计约束） | ✅ 通过 |
| Level C 既有模式复用（文件:行 或 no prior art found + 关联 Task） | ✅ 通过 |
| Level C INV 追溯（每个 INV-* 关联 AC/Task/验证方式，且被状态机/资源/并发引用） | ✅ 通过 |
| 调用链分层 / 类图 / 状态归属 / 不涉及项承接 | ✅ 通过 |

> 机器校验记录见命令输出：`ohos-sdd validate . --level all --json`（2026-10-24 执行）。design 相关 broken = 0；全部 11 broken 属后续阶段（execution-plan 4 / spec-for-validation 3 / registry 1 / implementation 1 / design 占位已清零）。

## 跟进项（已随设计批准进入后续流程）

| # | 跟进项 | 处理 |
|---|--------|------|
| D-1 | threat-model.md 生成（命中"修改认证/授权/权限模型"判据） | ✅ 已生成 `threat-model.md`（THREAT-20260905541319，Approved，Approver=Lin Shen）；P0/P1 缓解已映射至 execution-plan Task |
| D-2 | `MAX_ALLOWABLE_DURATION=86400` → `(0,60]` 对齐（spec C-3 已定 60） | TASK-2 落地回归校验 |
| D-3 | 磁盘 authToken 明文存储复核（已确认接受） | threat-model COMP-001 案例闭环 |

## Verdict

- [x] Approved

| 字段 | 值 |
|------|-----|
| Approver | Lin Shen（Owner） |
| Evidence | design.md「设计审批」全部勾选 + 结论"通过"；ohos-sdd validate design 相关 0 broken；Owner 明确批准 |
| 结论 | Design 已批准；可进入 Plan（上游：proposal Approved + spec Approved + design Approved） |