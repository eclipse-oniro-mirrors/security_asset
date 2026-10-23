/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//! Sub-modules for IPC serialization.

pub mod remote_control;
pub mod task_identity;
pub mod ticket;

pub use ticket::{deserialize_verify_ticket_infos, serialize_cli_infos, serialize_verify_ticket_infos};

pub use remote_control::{
    deserialize_permission_queries, deserialize_remote_auth_packages, deserialize_remote_info,
    deserialize_remote_user_auth_results_vec, serialize_bool_vec, serialize_remote_auth_packages,
};

pub use task_identity::{
    deserialize_task_identity,
    deserialize_task_identity_info,
    deserialize_i32_u8_list,
    serialize_task_identity,
    serialize_u8_to_i32_list,
};