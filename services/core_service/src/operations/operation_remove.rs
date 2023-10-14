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

use crate::{
    calling_info::CallingInfo,
    operations::operation_common::{
        construct_params_with_default,
        db_adapter::{remove_data_once, construct_db_data},
    },
    definition_inner::OperationCode
};
use asset_ipc_interface::IpcCode;

use asset_common::{definition::{AssetMap, Result}, logi};

pub(crate) fn remove(input: &AssetMap, calling_info: &CallingInfo) -> Result<()> {

    let input_new = construct_params_with_default(input, &IpcCode::Remove)?;

    let data_vec = construct_db_data(&input_new, calling_info, &OperationCode::Remove)?;
    let remove_num = remove_data_once(calling_info, &data_vec)?;
    logi!("remove {} data", remove_num);
    Ok(())
}
