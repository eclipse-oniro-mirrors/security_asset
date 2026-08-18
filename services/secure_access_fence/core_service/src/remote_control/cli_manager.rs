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

//! CLI tool permission manager adapter

use saf_definition::{macros_lib, CommandInfo, ErrCode, Result};
use saf_log::loge;
use std::collection::HashSet;

const MAX_PERMISSION_BUF_SIZE: usize = 4096;

#[repr(C)]
struct CxxStr {
    ptr: *const std::ffi::c_char,
    len: i32,
}

#[repr(C)]
struct CxxCmdInfo {
    cmd_name: CxxStr,
    sub_cmd: CxxStr,
}

#[repr(C)]
struct CxxQueryResult {
    ret_code: i32,
    result_code: i32,
    perm_count: i32,
}

extern "C" {
    fn CxxBatchQueryCliPermissions(
        cmds: *const CxxCmdInfo,
        cmd_count: i32,
        out_buf: *mut std::ffi::c_char,
        buf_size: i32,
        out_result: *mut CxxQueryResult,
    ) -> i32;
}

/// Batch query CLI tool permissions
pub fn batch_query_cli_permission(
    cli_infos: &[CommandInfo],
    permissions: &mut Vec<String>
) -> Result<()> {
    if cli_infos.is_empty() {
        return Ok(());
    }

    let cxx_cmds = prepare_cxx_cmd_infos(cli_infos);
    let mut out_buf = vec![0u8; MAX_PERMISSION_BUF_SIZE];
    let mut result = CxxQueryResult {
        ret_code: 0,
        result_code: 0,
        perm_count: 0,
    };

    call_cxx_batch_query(&cxx_cmds, &mut out_buf, &mut result)?;
    parse_permissions_from_buffer(&out_buf, result.perm_count, permissions)?;

    deduplicate_permissions(permissions);

    Ok(())
}

fn prepare_cxx_cmd_infos(cli_infos: &[CommandInfo]) -> Vec<CxxCmdInfo> {
    cli_infos.iter().map(|cmd| {
        CxxCmdInfo {
            cmd_name: CxxStr {
                ptr: cmd.cmd_name.as_ptr() as *const std::ffi::c_char,
                len: cmd.cmd_name.len() as i32,
            },
            sub_cmd: CxxStr {
                ptr: cmd.sub_cmd.as_ptr() as *const std::ffi::c_char,
                len: cmd.sub_cmd.len() as i32,
            },
        }
    }).collect()
}

fn call_cxx_batch_query(
    cxx_cmds: &[CxxCmdInfo],
    out_buf: &mut [u8],
    result: &mut CxxQueryResult,
) -> Result<()> {
    let ret = unsafe {
        CxxBatchQueryCliPermissions(
            cxx_cmds.as_ptr(),
            cxx_cmds.len() as i32,
            out_buf.as_mut_ptr() as *mut std::ffi::c_char,
            out_buf.len() as i32,
            result,
        )
    };

    if ret != 0 {
        loge!("CxxBatchQueryCliPermissions failed, ret={}, result_code={}", ret, result.result_code);
        return macros_lib::log_throw_error!(ErrCode::GeneralError,
            "CxxBatchQueryCliPermissions failed");
    }

    Ok(())
}

/// Deduplicate permissions while preserving insertion order
fn deduplicate_permissions(permissions: &mut Vec<String>) {
    let mut seen = HashSet::new();
    permissions.retain(|p| seen.insert(p.clone()));
}

fn parse_permissions_from_buffer(
    out_buf: &[u8],
    perm_count: i32,
    permissions: &mut Vec<String>,
) -> Result<()> {
    const MAX_PERMISSION_COUNT: i32 = 200;
    
    if !(0..=MAX_PERMISSION_COUNT).contains(&perm_count) {
        return Err(macros_lib::log_and_into_saf_error!(ErrCode::GeneralError,
            "Invalid permission count: {}", perm_count));
    }
    
    let mut pos = 0;
    for _ in 0..perm_count {
        if pos >= out_buf.len() {
            return Err(macros_lib::log_and_into_saf_error!(ErrCode::GeneralError,
                "Permission buffer overflow: pos {} >= buf len {}", pos, out_buf.len()));
        }
        let end = out_buf[pos..].iter().position(|&b| b == 0).unwrap_or(out_buf.len() - pos);
        if pos + end > out_buf.len() {
            return Err(macros_lib::log_and_into_saf_error!(ErrCode::GeneralError,
                "Permission string exceeds buffer boundary"));
        }
        let s = std::str::from_utf8(&out_buf[pos..pos + end])
            .map_err(|_| macros_lib::log_and_into_saf_error!(ErrCode::GeneralError,
                "Invalid UTF-8 in permission"))?;
        permissions.push(s.to_string());
        pos += end + 1;
    }

    Ok(())
}