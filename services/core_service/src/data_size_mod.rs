/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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
 use asset_definition::Result;
 use asset_file_operator::common::{get_db_dirs, should_upload_data_size};
 use crate::sys_event::upload_data_size;
 use std::os::raw::c_char;
 use std::ffi::CString;

 /// Partition name.
 pub const PARTITION: &str = "/data";
 /// Component name.
 pub const COMPONENT: &str = "Asset";

 extern "C" {
    fn GetRemainPartitionSize(path: *const c_char) -> f64;
    fn GetDirSize(paths: *const c_char) -> u64;
 }

 /// get all asset folders size
 pub fn get_folders_size(paths: Vec<String>) -> Result<Vec<u64>> {
    let mut folders_size = vec![];
    for path in path.iter() {
        let path_cstr = CString::new(path.as_str()).expect("CString conversion failed");
        let folder_size: u64 = unsafe { GetDirSize(path_cstr.as_ptr()) };
        folders_size.push(folder_size);
    }
    Ok(folders_size)
 }

/// handle data upload
pub (crate) fn handle_data_size_upload(unix_time: u64) -> Result<()> {
    if should_upload_data_size(unix_time)? {
        let folder_path = get_db_dirs()?;
        let folder_path_clone = folder_path.clone();
        let folders_size = get_folders_size(folder_path_clone)?;
        let remain_size = get_remain_partition_size(PARTITION)?;
        upload_data_size(COMPONENT, PARTITION, remain_size, folder_path, folders_size);
    }
    Ok(())
}