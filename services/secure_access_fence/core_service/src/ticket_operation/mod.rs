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

//! This module implements ticket operation functionality.

mod ticket_key_manager;
mod local_based_ticket_key_manager;
mod account_based_ticket_key_manager;

pub use ticket_key_manager::TicketKeyManager;
pub use local_based_ticket_key_manager::LocalBasedTicketKeyManager;
pub use account_based_ticket_key_manager::AccountBasedTicketKeyManager;

use std::ffi::CString;
use std::os::raw::c_char;
use saf_definition::{ErrCode, Result, macros_lib};
use saf_ipc::VerifyTicketInfo;
use crate::notify_error;

const CHALLENGE_SIZE: usize = 32;
const HMAC_SHA256_SIZE: usize = 32;
const SAF_SUCCESS: i32 = 0;

#[repr(C)]
struct Uint8Buff {
    buf: *mut u8,
    size: u32,
}

#[repr(C)]
struct Uint8BuffConst {
    buf: *const u8,
    size: u32,
}

extern "C" {
    fn GenerateRandomBytes(buf: *mut Uint8Buff) -> i32;
    fn ComputeHmacSha256(key: *const Uint8BuffConst, data: *const Uint8BuffConst, hmac: *mut Uint8Buff) -> i32;
    fn VerifyHmacSha256(key: *const Uint8BuffConst, data: *const Uint8BuffConst, expectedHmac: *const Uint8BuffConst) -> i32;
    fn Base64Encode(input: *const Uint8BuffConst, output: *mut Uint8Buff) -> i32;
    fn Base64Decode(input: *const Uint8BuffConst, output: *mut Uint8Buff) -> i32;
    fn CheckBatchGenerateTicketParamsC(osAccountId: i32, callerId: *const c_char, messagesCount: usize) -> i32;
    fn CheckBatchVerifyTicketParamsC(osAccountId: i32, callerId: *const c_char, verifyInfosCount: usize) -> i32;
}

fn generate_challenge() -> Result<Vec<u8>> {
    let mut buf = vec![0u8; CHALLENGE_SIZE];
    let mut buff = Uint8Buff {
        buf: buf.as_mut_ptr(),
        size: CHALLENGE_SIZE as u32,
    };
    let ret = unsafe { GenerateRandomBytes(&mut buff) };
    if ret != SAF_SUCCESS {
        macros_lib::log_throw_error!(ErrCode::try_from(ret as u32)?, "generate challenge failed")
    } else {
        Ok(buf)
    }
}

fn compute_hmac_sha256(key: &[u8], data: &[u8]) -> Result<Vec<u8>> {
    let mut hmac = vec![0u8; HMAC_SHA256_SIZE];
    let key_buff = Uint8BuffConst {
        buf: key.as_ptr(),
        size: key.len() as u32,
    };
    let data_buff = Uint8BuffConst {
        buf: data.as_ptr(),
        size: data.len() as u32,
    };
    let mut hmac_buff = Uint8Buff {
        buf: hmac.as_mut_ptr(),
        size: HMAC_SHA256_SIZE as u32,
    };
    let ret = unsafe { ComputeHmacSha256(&key_buff, &data_buff, &mut hmac_buff) };
    if ret != SAF_SUCCESS {
        macros_lib::log_throw_error!(ErrCode::try_from(ret as u32)?, "compute hmac failed")
    } else {
        Ok(hmac)
    }
}

fn verify_hmac_sha256(key: &[u8], data: &[u8], expected_hmac: &[u8]) -> Result<bool> {
    if expected_hmac.len() != HMAC_SHA256_SIZE {
        return macros_lib::log_throw_error!(ErrCode::InvalidHmacSize, "invalid hmac size");
    }
    let key_buff = Uint8BuffConst {
        buf: key.as_ptr(),
        size: key.len() as u32,
    };
    let data_buff = Uint8BuffConst {
        buf: data.as_ptr(),
        size: data.len() as u32,
    };
    let expected_hmac_buff = Uint8BuffConst {
        buf: expected_hmac.as_ptr(),
        size: HMAC_SHA256_SIZE as u32,
    };
    let ret = unsafe { VerifyHmacSha256(&key_buff, &data_buff, &expected_hmac_buff) };
    if ret == SAF_SUCCESS {
        Ok(true)
    } else {
        macros_lib::log_throw_error!(ErrCode::try_from(ret as u32)?, "verify hmac failed")
    }
}

fn base64_encode(input: &[u8]) -> Result<Vec<u8>> {
    let expected_len = 4 * ((input.len() + 2) / 3) + 1;
    let mut output = vec![0u8; expected_len];
    let input_buff = Uint8BuffConst {
        buf: input.as_ptr(),
        size: input.len() as u32,
    };
    let mut output_buff = Uint8Buff {
        buf: output.as_mut_ptr(),
        size: expected_len as u32,
    };
    let ret = unsafe { Base64Encode(&input_buff, &mut output_buff) };
    if ret != SAF_SUCCESS {
        macros_lib::log_throw_error!(ErrCode::try_from(ret as u32)?, "base64 encode failed")
    } else {
        unsafe { output.set_len(output_buff.size as usize) };
        Ok(output)
    }
}

fn base64_decode(input: &[u8]) -> Result<Vec<u8>> {
    let expected_len = 3 * input.len() / 4;
    let mut output = vec![0u8; expected_len];
    let input_buff = Uint8BuffConst {
        buf: input.as_ptr(),
        size: input.len() as u32,
    };
    let mut output_buff = Uint8Buff {
        buf: output.as_mut_ptr(),
        size: expected_len as u32,
    };
    let ret = unsafe { Base64Decode(&input_buff, &mut output_buff) };
    if ret != SAF_SUCCESS {
        macros_lib::log_throw_error!(ErrCode::try_from(ret as u32)?, "base64 decode failed")
    } else {
        unsafe { output.set_len(output_buff.size as usize) };
        Ok(output)
    }
}

pub fn create_ticket_key_manager(caller_id: &str) -> Box<dyn TicketKeyManager> {
    if !caller_id.is_empty() {
        Box::new(AccountBasedTicketKeyManager::new())
    } else {
        Box::new(LocalBasedTicketKeyManager::new())
    }
}

pub fn batch_generate_ticket(os_account_id: i32, caller_id: &str, domain_id: &str, messages: &[String],) ->
    Result<Vec<VerifyTicketInfo>> {
    let caller_id_cstr = CString::new(caller_id).unwrap_or_default();
    let check_result = unsafe {
        CheckBatchGenerateTicketParamsC(os_account_id , caller_id_cstr.as_ptr(), messages.len())
    };
    if check_result != SAF_SUCCESS {
        return macros_lib::log_throw_error!(ErrCode::try_from(check_result as u32)?, 
            "batch_generate_ticket params check failed");
    }

    let challenge1 = generate_challenge()?;

    let key_manager = create_ticket_key_manager(caller_id);
    let session_key = key_manager.derive_ticket_session_key(os_account_id , domain_id, &challenge1)?;

    let mut results = Vec::with_capacity(messages.len());

    for (index, message) in messages.iter().enumerate() {
        let ticket_info = (|| {
            if message.is_empty() {
                return macros_lib::log_throw_error!(ErrCode::ArgEmpty, "message is empty");
            }
            let challenge2 = generate_challenge()?;

            let mut data = message.as_bytes().to_vec();
            data.extend_from_slice(&challenge2);

            let hmac_result = compute_hmac_sha256(&session_key, &data)?;

            let combined_challenge = [challenge1.as_slice(), challenge2.as_slice()].concat();

            Ok(VerifyTicketInfo {
                message: message.clone(),
                challenge: String::from_utf8_lossy(&base64_encode(&combined_challenge)?).to_string(),
                ticket: String::from_utf8_lossy(&base64_encode(&hmac_result)?).to_string(),
            })
        })().unwrap_or_else(|e| {
            let error_msg = format!("Ticket idx[{}]: generate failed, message len={}, err={}",
                index, message.len(), e.code);
            macros_lib::loge!("{}", error_msg);
            notify_error(
                error_msg.to_string(),
                e.code as i32,
                os_account_id,
                "batch_generate_ticket_sub".to_string()
            );
            VerifyTicketInfo {
                message: message.clone(),
                challenge: String::new(),
                ticket: String::new(),
            }
        });

        results.push(ticket_info);
    }

    Ok(results)
}

pub fn batch_verify_ticket(
    os_account_id: i32,
    caller_id: &str,
    domain_id: &str,
    verify_infos: &[VerifyTicketInfo],
) -> Result<Vec<i32>> {
    let caller_id_cstr = CString::new(caller_id).unwrap_or_default();
    let check_result = unsafe {
        CheckBatchVerifyTicketParamsC(os_account_id , caller_id_cstr.as_ptr(), verify_infos.len())
    };
    if check_result != SAF_SUCCESS {
        return macros_lib::log_throw_error!(ErrCode::try_from(check_result as u32)?,
            "batch_verify_ticket params check failed");
    }

    let key_manager = create_ticket_key_manager(caller_id);

    let challenge = generate_challenge()?;
    let _session_key = key_manager.derive_ticket_session_key(os_account_id , domain_id, &challenge)?;

    let mut results = Vec::with_capacity(verify_infos.len());

    for (index, verify_info) in verify_infos.iter().enumerate() {
        let combined_challenge = match base64_decode(verify_info.challenge.as_bytes()) {
            Ok(v) => v,
            Err(e) => {
                macros_lib::loge!("VerifyTicket idx[{}]: base64_decode challenge failed, err={}", index, e.code);
                results.push(e.code as i32);
                continue;
            }
        };
        if combined_challenge.len() < CHALLENGE_SIZE * 2 {
            macros_lib::loge!("VerifyTicket idx[{}]: combined_challenge len invalid, len={}",
                index, combined_challenge.len());
            results.push(ErrCode::InvalidChallengeSize as i32);
            continue;
        }

        let challenge1 = &combined_challenge[..CHALLENGE_SIZE as usize];
        let challenge2 = &combined_challenge[CHALLENGE_SIZE as usize..];

        let session_key = match key_manager.derive_ticket_session_key(os_account_id , domain_id, challenge1) {
            Ok(key) => key,
            Err(e) => {
                macros_lib::loge!("VerifyTicket idx[{}]: derive_ticket_session_key failed, err={}", index, e.code);
                results.push(e.code as i32);
                continue;
            }
        };

        let mut data = verify_info.message.as_bytes().to_vec();
        data.extend_from_slice(challenge2);

        let expected_hmac = match base64_decode(verify_info.ticket.as_bytes()) {
            Ok(v) => v,
            Err(e) => {
                macros_lib::loge!("VerifyTicket idx[{}]: base64_decode ticket failed, err={}", index, e.code);
                results.push(e.code as i32);
                continue;
            }
        };

        match verify_hmac_sha256(&session_key, &data, &expected_hmac) {
            Ok(_) => results.push(ErrCode::Success as i32),
            Err(e) => {
                let error_msg = format!(
                    "VerifyTicket idx:[{}] message_len:[{}] challenge_len:[{}] ticket_len:[{}] failed, err={}",
                    index, 
                    verify_info.message.len(),
                    verify_info.challenge.len(),
                    verify_info.ticket.len(),
                    e.code
                );
                macros_lib::loge!("{}", error_msg);
                notify_error(
                    error_msg.to_string(),
                    e.code as i32,
                    os_account_id,
                    "batch_verify_ticket_sub".to_string()
                );
                results.push(e.code as i32);
            }
        }
    }

    Ok(results)
}