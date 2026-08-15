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

pub mod ticket;
pub mod remote_control;

pub use ticket::{
    deserialize_verify_ticket_infos,
    serialize_verify_ticket_infos,
    serialize_cli_infos,
};

pub use remote_control::{
    deserialize_permission_queries,
    deserialize_remote_info,
    deserialize_remote_auth_packages,
    deserialize_remote_user_auth_results_vec,
    serialize_bool_vec,
    serialize_remote_auth_packages,
};