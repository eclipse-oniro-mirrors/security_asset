/*
 * Copyright (c) 2023-2026 Huawei Device Co., Ltd.
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

//! This module defines the interface of the Asset Rust SDK.

use std::{
    sync::{Arc, Mutex},
    time::Instant,
};

use std::{ffi::CString, os::raw::c_char, sync::OnceLock};
pub use asset_definition::*;

use asset_log::logw;
use ipc::{parcel::{MsgParcel, MsgOption}, remote::RemoteObj};
use samgr::manage::SystemAbilityManager;

pub use asset_ipc::{
    deserialize_batch_result, deserialize_map, deserialize_maps, deserialize_sync_result, ipc_err_handle,
    serialize_map, serialize_maps, IpcCode, IPC_SUCCESS, SA_ID, SA_NAME,
};

extern "C" {
    fn GetTimeOut(timeout: *mut i32) -> i32;
    fn IsBeforeImageCreationPoint() -> bool;
    fn IsAbilityCreated() -> bool;
    fn ReportSnapshotFailure(load_interface_name: *const c_char, unload_interface_name: *const c_char);
}

const LOAD_TIMEOUT_IN_SECONDS: i32 = 4;
const SUCCESS: i32 = 0;
const MAX_ARRAY_CAPACITY: usize = 100;
static ASSET_PLUGIN_LOCK: Mutex<()> = Mutex::new(());

struct ImageInfo {
    use_before_flag: bool,
    has_notify: bool,
    calling_func: IpcCode,
    has_groupid: bool,
}

impl ImageInfo {
    fn new() -> Self {
        ImageInfo { use_before_flag: false, has_notify: false, calling_func: IpcCode::Add,
            has_groupid: false}
    }

    /// Get the single instance of Counter.
    pub fn get_instance() -> Arc<Mutex<ImageInfo>> {
        static INSTANCE: OnceLock<Arc<Mutex<ImageInfo>>> = OnceLock::new();
        INSTANCE.get_or_init(|| {
            Arc::new(Mutex::new(ImageInfo::new()))
        }).clone()
    }

    pub(crate) fn set_flag(&mut self, calling_func: IpcCode) {
        self.use_before_flag = true;
        self.calling_func = calling_func;
    }

    pub(crate) fn has_set_flag(&self) -> bool {
        self.use_before_flag
    }

    pub(crate) fn get_calling_func(&self) -> IpcCode {
        self.calling_func
    }

    pub(crate) fn has_notify(&self) -> bool {
        self.has_notify
    }

    pub(crate) fn has_groupid(&self) -> bool {
        self.has_groupid
    }

    pub(crate) fn set_has_notify(&mut self) {
        self.has_notify = true;
    }
}

fn load_asset_service() -> Result<RemoteObj> {
    let mut timeout: i32 = 0;
    let ret = unsafe { GetTimeOut(&mut timeout as *mut i32) };
    if ret != SUCCESS {
        timeout = LOAD_TIMEOUT_IN_SECONDS;
    }
    let start_time = Instant::now();
    match SystemAbilityManager::load_system_ability(SA_ID, timeout) {
        Some(remote) => Ok(remote),
        None => {
            logw!("load_asset_service time:{}s", start_time.elapsed().as_secs_f64());
            macros_lib::log_throw_error!(macros_lib::hisysevent::function!(),
                ErrCode::ServiceUnavailable, "[FATAL][RUST SDK]get remote service failed")
        },
    }
}

/// This manager provides the capabilities for life cycle management of sensitive user data (Asset) such as passwords
/// and tokens, including adding, removing, updating, and querying.
pub struct Manager {
    remote: RemoteObj,
}

impl Manager {
    /// Build and initialize the Manager.
    pub fn build() -> Result<Arc<Mutex<Manager>>> {
        static mut INSTANCE: Option<Arc<Mutex<Manager>>> = None;
        let _guard = ASSET_PLUGIN_LOCK.lock().unwrap();

        unsafe {
            if let Some(instance) = &INSTANCE {
                return Ok(instance.clone());
            }

            logw!("Create instance for Manager.");
            let remote = load_asset_service()?;
            let manager = Arc::new(Mutex::new(Manager { remote }));
            INSTANCE = Some(manager.clone());

            Ok(manager.clone())
        }
    }

    /// Add an Asset.
    pub fn add(&mut self, attributes: &AssetMap) -> Result<()> {
        self.snapshot_check_before_image(attributes, IpcCode::Add);
        self.snapshot_check_groupid();
        self.process_one_agr_request(attributes, IpcCode::Add)?;
        self.snapshot_check_after_image(IpcCode::Add);
        Ok(())
    }

    /// Add batch assets.
    pub fn batch_add(&mut self, attributes_array: &Vec<AssetMap>) -> Result<Vec<(u32, u32)>> {
        self.snapshot_check_batch_before_image(attributes_array, IpcCode::BatchAdd);
        self.snapshot_check_groupid();
        let ret = self.process_one_array_request_with_ret(attributes_array, IpcCode::BatchAdd)?;
        self.snapshot_check_after_image(IpcCode::BatchAdd);
        Ok(ret)
    }

    /// Remove one or more Assets that match a search query.
    pub fn remove(&mut self, query: &AssetMap) -> Result<()> {
        self.snapshot_check_before_image(query, IpcCode::Remove);
        self.snapshot_check_groupid();
        self.process_one_agr_request(query, IpcCode::Remove)?;
        self.snapshot_check_after_image(IpcCode::Remove);
        Ok(())
    }

    /// Remove batch assets.
    pub fn batch_remove(&mut self, attributes_array: &Vec<AssetMap>) -> Result<()> {
        self.snapshot_check_batch_before_image(attributes_array, IpcCode::BatchRemove);
        self.snapshot_check_groupid();
        self.process_one_array_request(attributes_array, IpcCode::BatchRemove)?;
        self.snapshot_check_after_image(IpcCode::BatchRemove);
        Ok(())
    }

    /// Update batch assets.
    pub fn batch_update(
        &mut self,
        attributes_array: &Vec<AssetMap>,
        attributes_to_update_array: &Vec<AssetMap>
    ) -> Result<Vec<(u32, u32)>> {
        self.snapshot_check_batch_before_image(attributes_array, IpcCode::BatchUpdate);
        self.snapshot_check_groupid();
        let ret = self.process_two_array_request_with_ret(
            attributes_array, attributes_to_update_array, IpcCode::BatchUpdate
        )?;
        self.snapshot_check_after_image(IpcCode::BatchUpdate);
        Ok(ret)
    }

    /// Update an Asset that matches a search query.
    pub fn update(&mut self, query: &AssetMap, attributes_to_update: &AssetMap) -> Result<()> {
        self.snapshot_check_before_image(query, IpcCode::Update);
        self.snapshot_check_groupid();
        self.process_two_agr_request(query, attributes_to_update, IpcCode::Update)?;
        self.snapshot_check_after_image(IpcCode::Update);
        Ok(())
    }

    /// Preprocessing for querying one or more Assets that require user authentication.
    pub fn pre_query(&mut self, query: &AssetMap) -> Result<Vec<u8>> {
        self.snapshot_check_before_image(query, IpcCode::PreQuery);
        self.snapshot_check_groupid();
        let mut reply = self.process_one_agr_request(query, IpcCode::PreQuery)?;
        let res = reply.read::<Vec<u8>>().map_err(ipc_err_handle)?;
        Ok(res)
    }

    /// Query one or more Assets that match a search query.
    pub fn query(&mut self, query: &AssetMap) -> Result<Vec<AssetMap>> {
        self.snapshot_check_before_image(query, IpcCode::Query);
        self.snapshot_check_groupid();
        let mut reply = self.process_one_agr_request(query, IpcCode::Query)?;
        let res = deserialize_maps(&mut reply)?;
        Ok(res)
    }

    /// Post-processing for querying multiple Assets that require user authentication.
    pub fn post_query(&mut self, query: &AssetMap) -> Result<()> {
        self.process_one_agr_request(query, IpcCode::PostQuery)?;
        Ok(())
    }

    /// Query the result of synchronization.
    pub fn query_sync_result(&mut self, query: &AssetMap) -> Result<SyncResult> {
        match self.process_one_agr_request(query, IpcCode::QuerySyncResult) {
            Ok(mut reply) => {
                let sync_result = deserialize_sync_result(&mut reply)?;
                Ok(sync_result)
            },
            Err(mut e) => {
                if e.code == ErrCode::InvalidArgument {
                    e.code = ErrCode::ParamVerificationFailed;
                }
                Err(e)
            },
        }
    }

    fn rebuild(&mut self) -> Result<()> {
        self.remote = load_asset_service()?;
        Ok(())
    }

    fn process_one_agr_request(&mut self, attributes: &AssetMap, ipc_code: IpcCode) -> Result<MsgParcel> {
        let mut parcel = MsgParcel::new();
        parcel.write_interface_token(self.descriptor()).map_err(ipc_err_handle)?;
        serialize_map(attributes, &mut parcel)?;
        match self.send_request(parcel, ipc_code) {
            Ok(msg) => Ok(msg),
            Err(e) => match e.code {
                ErrCode::ServiceUnavailable => {
                    logw!("ServiceUnavailable, rebuild Manager");
                    self.rebuild()?;
                    let mut parcel = MsgParcel::new();
                    parcel.write_interface_token(self.descriptor()).map_err(ipc_err_handle)?;
                    serialize_map(attributes, &mut parcel)?;
                    self.send_request(parcel, ipc_code)
                },
                _ => Err(e),
            },
        }
    }

    fn process_two_agr_request(
        &mut self,
        query: &AssetMap,
        attributes_to_update: &AssetMap,
        ipc_code: IpcCode,
    ) -> Result<MsgParcel> {
        let mut parcel = MsgParcel::new();
        parcel.write_interface_token(self.descriptor()).map_err(ipc_err_handle)?;
        serialize_map(query, &mut parcel)?;
        serialize_map(attributes_to_update, &mut parcel)?;
        match self.send_request(parcel, ipc_code) {
            Ok(msg) => Ok(msg),
            Err(e) => match e.code {
                ErrCode::ServiceUnavailable => {
                    self.rebuild()?;
                    let mut parcel = MsgParcel::new();
                    parcel.write_interface_token(self.descriptor()).map_err(ipc_err_handle)?;
                    serialize_map(query, &mut parcel)?;
                    serialize_map(attributes_to_update, &mut parcel)?;
                    self.send_request(parcel, ipc_code)
                },
                _ => Err(e),
            },
        }
    }

    fn process_one_array_request_with_ret(
        &mut self,
        attributes_array: &Vec<AssetMap>,
        ipc_code: IpcCode,
    ) -> Result<Vec<(u32, u32)>> {
        let mut parcel = MsgParcel::new();
        parcel.write_interface_token(self.descriptor()).map_err(ipc_err_handle)?;
        if attributes_array.len() > MAX_ARRAY_CAPACITY {
            return macros_lib::throw_error!( macros_lib::hisysevent::function!(),
                ErrCode::InvalidArgument, "[FATAL][IPC]The array size {} exceeds the limit", attributes_array.len() );
        }
        serialize_maps(attributes_array, &mut parcel)?;
        match self.send_request(parcel, ipc_code) {
            Ok(mut reply) => {
                let res = deserialize_batch_result(&mut reply)?;
                Ok(res)
            },
            Err(e) => match e.code {
                ErrCode::ServiceUnavailable => {
                    logw!("ServiceUnavailable, rebuild Manager");
                    self.rebuild()?;
                    let mut parcel = MsgParcel::new();
                    parcel.write_interface_token(self.descriptor()).map_err(ipc_err_handle)?;
                    serialize_maps(attributes_array, &mut parcel)?;
                    let mut reply = self.send_request(parcel, ipc_code)?;
                    let res = deserialize_batch_result(&mut reply)?;
                    Ok(res)
                },
                _ => Err(e),
            },
        }
    }

    fn process_two_array_request_with_ret(
        &mut self,
        attributes_array: &Vec<AssetMap>,
        attributes_to_update_array: &Vec<AssetMap>,
        ipc_code: IpcCode,
    ) -> Result<Vec<(u32, u32)>> {
        let mut parcel = MsgParcel::new();
        parcel.write_interface_token(self.descriptor()).map_err(ipc_err_handle)?;
        if attributes_array.len() > MAX_ARRAY_CAPACITY {
            return macros_lib::throw_error!( macros_lib::hisysevent::function!(),
                ErrCode::InvalidArgument, "[FATAL][IPC]The array size {} exceeds the limit", attributes_array.len() );
        }
        serialize_maps(attributes_array, &mut parcel)?;
        serialize_maps(attributes_to_update_array, &mut parcel)?;
        match self.send_request(parcel, ipc_code) {
            Ok(mut reply) => {
                let res = deserialize_batch_result(&mut reply)?;
                Ok(res)
            },
            Err(e) => match e.code {
                ErrCode::ServiceUnavailable => {
                    logw!("ServiceUnavailable, rebuild Manager");
                    self.rebuild()?;
                    let mut parcel = MsgParcel::new();
                    parcel.write_interface_token(self.descriptor()).map_err(ipc_err_handle)?;
                    serialize_maps(attributes_array, &mut parcel)?;
                    serialize_maps(attributes_to_update_array, &mut parcel)?;
                    let mut reply = self.send_request(parcel, ipc_code)?;
                    let res = deserialize_batch_result(&mut reply)?;
                    Ok(res)
                },
                _ => Err(e),
            },
        }
    }

    fn process_one_array_request(
        &mut self,
        attributes_array: &Vec<AssetMap>,
        ipc_code: IpcCode,
    ) -> Result<MsgParcel> {
        let mut parcel = MsgParcel::new();
        parcel.write_interface_token(self.descriptor()).map_err(ipc_err_handle)?;
        if attributes_array.len() > MAX_ARRAY_CAPACITY {
            return macros_lib::throw_error!( macros_lib::hisysevent::function!(),
                ErrCode::InvalidArgument, "[FATAL][IPC]The array size {} exceeds the limit", attributes_array.len() );
        }
        serialize_maps(attributes_array, &mut parcel)?;
        match self.send_request(parcel, ipc_code) {
            Ok(msg) => Ok(msg),
            Err(e) => match e.code {
                ErrCode::ServiceUnavailable => {
                    logw!("ServiceUnavailable, rebuild Manager");
                    self.rebuild()?;
                    let mut parcel = MsgParcel::new();
                    parcel.write_interface_token(self.descriptor()).map_err(ipc_err_handle)?;
                    serialize_maps(attributes_array, &mut parcel)?;
                    self.send_request(parcel, ipc_code)
                },
                _ => Err(e),
            },
        }
    }

    fn send_request(&self, mut parcel: MsgParcel, ipc_code: IpcCode) -> Result<MsgParcel> {
        let mut option = MsgOption::new();
        option.set_image();
        let mut reply = self.remote.send_request_ext(ipc_code as u32, &mut parcel, option).map_err(ipc_err_handle)?;
        match reply.read::<u32>().map_err(ipc_err_handle)? {
            IPC_SUCCESS => Ok(reply),
            e => {
                let msg = reply.read::<String>().map_err(ipc_err_handle)?;
                macros_lib::throw_error!(macros_lib::hisysevent::function!(), ErrCode::try_from(e)?, "{}", msg)
            },
        }
    }

    fn descriptor(&self) -> &'static str {
        SA_NAME
    }

    fn snapshot_check_batch_before_image(&self, attributes_array: &[AssetMap], ipc_code: IpcCode) {
        if let Some(first_query) = attributes_array.first() {
            self.snapshot_check_before_image(first_query, ipc_code);
        }
    }

    fn snapshot_check_before_image(&self, query: &AssetMap, ipc_code: IpcCode) {
        unsafe { if !IsBeforeImageCreationPoint() { return; } }
        let image_info = ImageInfo::get_instance();
        let mut res = image_info.lock().unwrap();
        if !res.has_set_flag() {
            res.set_flag(ipc_code);
        }
        if query.get(&Tag::GroupId).is_some() {
            res.has_groupid = true;
        }
    }

    fn snapshot_check_after_image(&self, ipc_code: IpcCode) {
        let image_info = ImageInfo::get_instance();
        let mut res = image_info.lock().unwrap();
        if res.has_notify() || (!res.has_set_flag()) { return; }

        unsafe {
            if IsAbilityCreated() {
                let load_func = res.get_calling_func().to_string();
                let load_cstr = CString::new(load_func).unwrap();
                let unload_func = ipc_code.to_string();
                let unload_cstr = CString::new(unload_func).unwrap();
                ReportSnapshotFailure(load_cstr.as_ptr(), unload_cstr.as_ptr());
                res.set_has_notify();
            }
        }
    }

    fn snapshot_check_groupid(&self) {
        let image_info = ImageInfo::get_instance();
        let mut res = image_info.lock().unwrap();
        if res.has_notify() || (!res.has_groupid()) { return; }
        unsafe {
            if IsAbilityCreated() {
                let load_func = res.get_calling_func().to_string();
                let load_cstr = CString::new(load_func).unwrap();
                let unload_func = "hasGroupId";
                let unload_cstr = CString::new(unload_func).unwrap();
                ReportSnapshotFailure(load_cstr.as_ptr(), unload_cstr.as_ptr());
                res.set_has_notify();
            }
        }
    }
}
