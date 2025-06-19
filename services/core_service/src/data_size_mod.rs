/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

 //! This module is used to get Asset file size.

 use crate::sys_event::{COMPONENT, upload_data_size};
 use asset_definition::{log_throw_error, ErrCode, Result};
 use asset_file_operator::common::{get_db_dirs, should_upload_data_size};
 use asset_utils::time::system_time_in_seconds;
 use std::ffi::CString;
 use std::os::raw::c_char;


 extern "C" {
    fn GetRemainPartitionSize(partition_name: *const c_char, partition_size: *mut f64) -> i32;
    fn GetDirSize(dir: *const c_char, dir_size: *mut u64) -> i32;
 }

pub(crate) fn get_remain_partition_size(partition: &str) -> Result<f64> {
    let partition_cstr = CString::new(partition);
    let mut remain_size: f64 = 0.0;
    let ret_code: i32 = unsafe { GetRemainPartitionSize(partition_cstr.as_ptr(), &mut remain_size) };
    if ret_code != 0 {
        return log_throw_error!(ErrCode::try_from(ret_code as u32)?, "Get remain partition size failed");
    }
    Ok(remain_size)
}

 /// get all asset folders size
 pub fn get_folders_size(paths: &[String]) -> Result<Vec<u64>> {
    let mut folders_size = vec![];

    for path in path.iter() {
        let path_cstr = CString::new(path.as_str())?;
        let mut folder_size: u64 = 0;
        let ret_code: i32 = unsafe { GetDirSize(path_cstr.as_ptr(), &mut folder_size) };
        
        if ret_code != 0 {
            return log_throw_error!(ErrCode::try_from(ret_code as u32)?, "Get dir size failed!");
        }
        folders_size.push(folder_size);
    }
    Ok(folders_size)
 }

/// handle data upload
pub (crate) fn handle_data_size_upload(unix_time: u64) -> Result<()> {
    let unix_time = system_time_in_seconds()?;
    if should_upload_data_size(unix_time)? {
        let folder_path = get_db_dirs()?;
        let folders_size = get_folders_size(&folder_path)?;
        let remain_size = get_remain_partition_size(PARTITION)?;
        upload_data_size(COMPONENT, PARTITION, remain_size, folder_path, folders_size);
    }
    Ok(())
}