/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

//! This crate implements the asset

mod crypto_adapter;
pub(crate) mod db_adapter;
mod default_params;
mod extra_params;
mod file_operator;

pub(crate) use crypto_adapter::{encrypt, decrypt, init_decrypt};

pub(crate) use file_operator::create_user_db_dir;
pub(crate) use default_params::construct_params_with_default;
