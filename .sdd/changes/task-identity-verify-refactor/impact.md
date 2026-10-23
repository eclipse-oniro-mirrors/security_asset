# Impact

## Affected surfaces

| Surface | Current behavior | Required change | Compatibility |
| --- | --- | --- | --- |
| `task_identity/mod.rs` | verify_record_hmac (pub(crate)), retrieve_auth_token_by_task_id 自带三段式 | 新增 `fetch_record_and_verify` + `parse_task_id`;`verify_task_identity` 迁入(去 allow_disk);retrieve 改调脊柱 | 无公共符号变更;`verify_task_identity` 仍 `pub`(crate 内使用) |
| `task_identity/verification.rs` | 4 个函数 (parse_task_id/verify/get_execute_type/consume) | 删除文件 | 兼容(内部模块) |
| `task_identity/ticket.rs` | 编排链 3 次 parse + 2 次 fetch | 并入 `mod.rs` 改为 `verify_task_identity_and_consume`:脊柱单次取数,读 record.execute_type + delete_task_entry;文件删除 | 签名不变 `pub(crate) fn verify_task_identity_and_consume(os_account_id, &JsonValue)` |
| `wrapper.rs` | `cxx_verify_task_identity` 内部调用传 `true` (allow_disk) | 去掉 `true` 实参 | C++ 桥签名不变(map 导出不变) |

## Data and migration

None — 无持久化格式、磁盘路径、IPC/IDL、错误码变化。

## Risks

| Risk | Mitigation or required decision |
| --- | --- |
| 校验顺序/错误语义漂移 | 脊柱严格按 retrieve 现行顺序:caller 校验 → validate_path_segment → get_task_record → expire → hmac;verify_task_identity 在脊柱后追加显式比对,与现行一致 |
| verify_ticket 消费幂等 | 沿用 `delete_task_entry`(幂等);consume 失败仅日志不回传(保持现行) |
| 本机无 cargo/rustc,编译未验证 | 代码审阅 + 全仓 grep 核对引用;构建机验证标记为阻塞项 |