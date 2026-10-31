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

//! Bounded inline/shared-memory payload transport, independent of protocol algorithms.
use super::{invalid, MAX_BYTES};
use asset_log::logi;
use crate::{ipc_err_handle, macros_lib, ErrCode, MsgParcel, Result};
use utils_rust::ashmem::{create_ashmem_instance, ffi::AshmemGetSize, PROT_READ, PROT_WRITE};

const INLINE_BYTES: usize = 32 * 1024;
const INLINE: u32 = 0;
const SHARED: u32 = 1;

fn transport_error<T>() -> Result<T> {
    macros_lib::log_throw_error!(macros_lib::hisysevent::function!(),
        ErrCode::IpcError, "Privacy shared-memory transport failed")
}

/// Writes a bounded payload. Large messages use a sealed read-only Ashmem region.
pub fn write_payload(bytes: &[u8], parcel: &mut MsgParcel) -> Result<()> {
    if bytes.len() > MAX_BYTES || bytes.len() > i32::MAX as usize {
        return invalid();
    }
    let mode = if bytes.len() <= INLINE_BYTES { INLINE } else { SHARED };
    logi!("HPIC payload send begin, mode={}, bytes={}", mode, bytes.len());
    parcel.write(&mode).map_err(ipc_err_handle)?;
    parcel.write(&(bytes.len() as u32)).map_err(ipc_err_handle)?;
    if mode == INLINE {
        return if bytes.is_empty() { Ok(()) } else { parcel.write_buffer(bytes).map_err(ipc_err_handle) };
    }
    let memory = match unsafe { create_ashmem_instance("asset_privacy", bytes.len() as i32) } {
        Some(memory) => memory,
        None => return transport_error(),
    };
    if !memory.map_read_write_ashmem() {
        return transport_error();
    }
    let written = unsafe { memory.write_to_ashmem(bytes.as_ptr().cast(), bytes.len() as i32, 0) };
    memory.unmap_ashmem();
    if !written || !memory.set_protection(PROT_READ) {
        return transport_error();
    }
    // MsgParcel retains its own file descriptor; Ashmem destruction closes the local mapping/fd.
    parcel.write_ashmem(memory).map_err(ipc_err_handle)
}

/// Reads a validated snapshot. No shared pointer escapes the mapping's lifetime.
pub fn read_payload(parcel: &mut MsgParcel) -> Result<Vec<u8>> {
    let mode = parcel.read::<u32>().map_err(ipc_err_handle)?;
    let size = parcel.read::<u32>().map_err(ipc_err_handle)? as usize;
    if size > MAX_BYTES || size > i32::MAX as usize {
        return invalid();
    }
    logi!("HPIC payload receive begin, mode={}, bytes={}", mode, size);
    if mode == INLINE {
        if size > INLINE_BYTES {
            return invalid();
        }
        return if size == 0 { Ok(Vec::new()) } else { parcel.read_buffer(size).map_err(ipc_err_handle) };
    }
    if mode != SHARED || size <= INLINE_BYTES {
        return invalid();
    }
    let memory = parcel.read_ashmem().map_err(ipc_err_handle)?;
    if memory.get_ashmem_size() != size as i32 || AshmemGetSize(memory.get_ashmem_fd()) != size as i32 ||
        memory.get_protection() & PROT_WRITE != 0 || !memory.map_read_only_ashmem() {
        return invalid();
    }
    let data = unsafe { memory.read_from_ashmem(size as i32, 0) };
    if data.is_null() {
        return transport_error();
    }
    let bytes = unsafe { std::slice::from_raw_parts(data.cast::<u8>(), size) }.to_vec();
    memory.unmap_ashmem();
    Ok(bytes)
}
