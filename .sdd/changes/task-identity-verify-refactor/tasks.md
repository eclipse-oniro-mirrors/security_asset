# Tasks

- [x] T1 — `mod.rs` 新增 `pub(crate) fn fetch_record_and_verify`(caller 校验 → validate_path_segment → get_task_record → 过期 → verify_record_hmac → 返回 record) 与 `pub(crate) fn parse_task_id`;`verify_task_identity` 迁入 `mod.rs`(去 allow_disk,签名改 5 参);`retrieve_auth_token_by_task_id` 改调脊柱 (AC1, AC2, AC3)
- [x] T2 — `verify_task_identity_and_consume`(原 `verify_ticket_task_identity`)迁入 `mod.rs`:parse → 单次脊柱 → 比对 identity/challenge → 读 record.execute_type → INSTANT 直接 delete_task_entry;删除 `ticket.rs`;lib.rs 调用路径更新 (AC4, AC5)
- [x] T3 — `wrapper.rs` `cxx_verify_task_identity` 内部调用去掉 `true` 实参 (AC3)
- [x] T4 — 删除 `verification.rs`;`mod.rs` 移除 `mod verification;`/`pub use verification::{...}`/`pub(crate) mod ticket` (AC5)

## Verification evidence

| Criterion | Command or check | Result |
| --- | --- | --- |
| AC1(脊柱) | 代码审阅 | 通过;`fetch_record_and_verify` 在 mod.rs:295,顺序与 retrieve 一致 |
| AC2(retrieve 复用) | 代码审阅 + ripgrep | 通过;mod.rs:379 调脊柱后 extract_auth_token,签名不变 |
| AC3(verify_task_identity) | 代码审阅 + ripgrep | 通过;mod.rs:336 5 参签名;wrapper.rs:447 实参已更新,无 `allow_disk` 残留 |
| AC4(ticket 单次取数) | 代码审阅 | 通过;1 次 parse_task_id + 1 次 fetch_record_and_verify;INSTANT 消费走 store.delete_task_entry,失败仅日志 |
| AC5(verification.rs/ticket.rs 删除) | git status + ripgrep | 通过;两文件已删,`mod verification`/`pub use verification`/`mod ticket` 已移除,无 `verification::`/`ticket::`/`get_execute_type`/`consume_task_identity` 残留 |
| AC6(错误码语义) | ripgrep + 代码审阅 | 通过;caller 校验(InvalidArgument)仍先于 parse(TaskIdentityNotFound);identity/challenge 显式比对经 check_task_identity_match 保留 → TaskIdentityMismatch;fetch 自带 get_task_record(内存+磁盘兜底) |
| AC7(编译) | 构建机 | 阻塞:本机无 cargo/rustc,需 OHOS 构建机编译 |

## Blocked or deferred

- 编译/单测:需构建机(OHOS GN/hb 环境),本机无 cargo/rustc
- 现有 task-level-ticket 遗留知识:`task_identity_test_cases.xlsx` 128 条用例待更新(与本次无关)