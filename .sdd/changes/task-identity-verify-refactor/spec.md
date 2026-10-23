# Task identity 验证流程重构：公共脊柱 fetch_record_and_verify

## Intent

当前 HEAD（c09be129，"单次任务执行与验证"）中，`retrieve_auth_token_by_task_id`、`verify_task_identity`、`verify_task_identity_and_consume` 三条验证路径各自重复"取记录→过期检查→HMAC 验证"三段式；`verify_task_identity_and_consume` 编排链对同一笔数据做了 3 次 `parse_task_id`、2 次 `get_task_record`。本变更抽取公共脊柱 `fetch_record_and_verify`（返回已验证的 `TaskIdentityRecord`），统一三条路径，消除 `verification.rs` 与 `ticket.rs`，保持全部对外行为不变。

## Requirements

### R1: 公共脊柱 fetch_record_and_verify

- Given `(os_account_id, caller, task_id)` 入参（原含 `calling_uid`，自 `task-identity-bind-caller-token` 起移除，HMAC 不再拼接调用方 uid）
- When 校验与验证流程执行
- Then 依次完成：caller/路径合法性校验 → `get_task_record`（内存优先+磁盘兜底）→ 过期检查（`taskExpireSec=-1` 永不过期）→ `verify_record_hmac` → 返回已验证的 `TaskIdentityRecord`
- 任一环节失败返回对应错误码，与现行三条路径的错误语义一致（InvalidArgument / TaskIdentityNotFound / TaskIdentityExpired / TaskIdentityMismatch）
- 该函数无"消费/删除"副作用

### R2: retrieve_auth_token_by_task_id 复用脊柱

- Given 系统 SA 调用（业主同 task-level-ticket R4）
- When 调用 `retrieve_auth_token_by_task_id`
- Then 内部改为 `fetch_record_and_verify(...)` 后 `extract_auth_token`；外部签名、错误码、返回字节完全不变

### R3: verify_task_identity 合一入 mod.rs

- Given `(os_account_id, caller, task_attributes, identity, challenge)` 入参（原 `allow_disk` 参数删除，无调用方传 `false`）
- When 调用 `verify_task_identity`
- Then parse taskId → `fetch_record_and_verify` → 显式比对 `record.identity==identity && record.challenge==challenge`，不一致返回 `TaskIdentityMismatch`；一致返回 `Ok(())`（`calling_uid` 采集点已随 `task-identity-bind-caller-token` 移除）
- 校验顺序保持 get record → expire → hmac → 显式比对，与现行一致

### R4: verify_task_identity_and_consume 单次取数

- Given `(os_account_id, message_json)`（含可选 `taskIdentity` 内嵌对象）
- When 调用 `verify_task_identity_and_consume`
- Then
  - `taskIdentity` 为 Null → `Ok(())`（不处理）
  - 否则提取 `taskAttributes`/`identity`/`challenge`/`callerToken`，parse taskId（`calling_uid` 采集点已随 `task-identity-bind-caller-token` 移除，验证不再拼接调用方 uid）
  - 单次 `fetch_record_and_verify` → 比对 identity/challenge → 直接读 `record.execute_type`；INSTANT → `global_task_identity_store().delete_task_entry(...)` 消费（STORE 层，非 RAII）
- 全流程仅 1 次 `parse_task_id`、1 次 `get_task_record`

### R5: verification.rs 与 ticket.rs 移除

- Given 重构完成后
- Then `task_identity/verification.rs` 与 `task_identity/ticket.rs` 删除；`mod.rs` 中不再 `mod verification`、不再 `pub use verification::{...}`、不再 `pub(crate) mod ticket`；`verify_task_identity` 以 `pub fn` 直接定义于 `mod.rs`；`parse_task_id`/`fetch_record_and_verify`/`check_task_identity_match` 为 `pub(crate)`；`verify_task_identity_and_consume` 以 `pub(crate)` 直接定义于 `mod.rs`，lib.rs 调用路径由 `task_identity::ticket::verify_ticket_task_identity` 改为 `task_identity::verify_task_identity_and_consume`

## Out of scope

- 不改变 `generate_task_identity` / `generate_task_challenge` / `revoke_task_identities` 的生成/撤销逻辑
- 不改变 `verify_ticket_impl`（lib.rs）或 `cxx_verify_task_identity` 的 C++ 桥签名（wrapper 函数签名不含 `allow_disk`，仅内部实参变化）
- 不改变磁盘格式、LRU 策略、错误码定义、IPC/IDL 接口
- 不新增公共 C 符号

## Acceptance criteria

- [ ] AC1: `mod.rs` 新增 `pub(crate) fn fetch_record_and_verify(...) -> Result<TaskIdentityRecord>`，无副作用
- [ ] AC2: `retrieve_auth_token_by_task_id` 引用脊柱，行为/签名不变
- [ ] AC3: `verify_task_identity` 定义于 `mod.rs`，签名去掉 `allow_disk`，比对顺序不变；`wrapper.rs` 与 `ticket.rs` 相应实参更新
- [ ] AC4: `verify_task_identity_and_consume` 单次 parse + 单次 fetch；INSTANT 消费保留且幂等；consume 失败不回传错误（仅日志，保持现行）
- [ ] AC5: `verification.rs` 与 `ticket.rs` 删除，`mod.rs` 无残留声明/引用，全仓无 `verification::`/`ticket::`/`parse_task_id`/`get_execute_type`/`consume_task_identity` 悬空引用
- [ ] AC6: 全仓 grep 确认无 `allow_disk`/`get_execute_type`/`consume_task_identity`/`task_identity::ticket` 残留引用；错误码语义不变（InvalidArgument/NotFound/Expired/Mismatch）
- [ ] AC7: 编译通过（构建机验证，本机无 cargo）