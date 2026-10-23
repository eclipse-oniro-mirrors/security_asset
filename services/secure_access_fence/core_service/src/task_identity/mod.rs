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

//! Task-level authorization: challenge / identity generation, revocation and verification.

pub(crate) mod store;
pub(crate) mod storage_backend;
pub(crate) mod validator;

pub(crate) use validator::{validate_path_segment, validate_task_id_info, validate_revoke_params};

use saf_common::JsonBuilder;
use saf_definition::{macros_lib, ErrCode, ExecuteType, Result, TaskIdentity, TaskIdentityInfo};
use saf_log::{loge, logi, logw};
use saf_utils::{JsonValue, get_compact_json_value};

use crate::JSON_KEY_CALLER_TOKEN_ID;
use crate::ticket_operation::{
    base64_decode, base64_encode, compute_hmac_sha256, create_ticket_key_manager, generate_random_bytes,
    verify_hmac_sha256,
};
use store::{global_task_identity_store, TaskIdentityRecord};

const CHALLENGE_SIZE: u32 = 32;
/// Challenge size in bytes for splitting combined challenge.
const COMBINED_CHALLENGE_SIZE: usize = 64;
/// Milliseconds per second, for sec↔ms conversion.
const MS_PER_SEC: u64 = 1000;
/// JSON key for the auth token (base64) in task attributes.
const TASK_ATTR_AUTH_TOKEN: &str = "authToken";
/// JSON key for the ATL level in task attributes.
const TASK_ATTR_ATL: &str = "atl";
/// Max task attributes JSON size in bytes (DoS prevention).
const MAX_TASK_ATTRIBUTES_SIZE: usize = 8192;
/// JSON key for the task identity object embedded in a ticket message.
const JSON_KEY_TASK_IDENTITY: &str = "taskIdentity";
/// JSON key for task attributes inside the task identity object.
const JSON_KEY_TASK_ATTRIBUTES: &str = "taskAttributes";
/// JSON key for identity inside the task identity object.
const JSON_KEY_TASK_IDENTITY_IDENTITY: &str = "identity";
/// JSON key for challenge inside the task identity object.
const JSON_KEY_TASK_CHALLENGE: &str = "challenge";

/// Verifies auth token via IAM. Returns (challenge, authTrustLevel).
/// challenge is compared with pending challenge from generateTaskChallenge.
/// authTrustLevel is written into taskAttributes.
fn iam_verify_auth_token(auth_token: &[u8], allowable_duration: i32) -> Result<(Vec<u8>, u32)> {
    if allowable_duration < 0 {
        return macros_lib::log_throw_error!(ErrCode::InvalidArgument,
            "allowableDuration must be non-negative: {}", allowable_duration);
    }
    let allowable_ms = match (allowable_duration as u64).checked_mul(MS_PER_SEC) {
        Some(v) => v,
        None => return macros_lib::log_throw_error!(ErrCode::InvalidArgument,
            "allowableDuration overflow: {} * {}", allowable_duration, MS_PER_SEC),
    };
    let token_vec: Vec<u8> = auth_token.to_vec();
    let result = crate::wrapper::ffi::VerifyAuthToken(&token_vec, allowable_ms);
    if result.result_code != 0 {
        return macros_lib::log_throw_error!(ErrCode::GeneralError,
            "IAM VerifyAuthToken failed, result_code={}", result.result_code);
    }
    Ok((result.challenge, result.auth_trust_level))
}

/// Builds the task attributes JSON that is embedded into the task identity and covered by HMAC.
/// `caller` is the callerTokenId of the generating caller; binding it here (same approach as the
/// RequestToolPermissions ticket, where callerTokenId is part of the HMAC-covered message) lets the
/// verification path recompute the HMAC from the stored attributes without splicing in a dynamic
/// caller identity.
fn build_task_attributes(info: &TaskIdentityInfo, caller: &str, atl: u32) -> Result<String> {
    let mut builder = JsonBuilder::new();
    builder.add_string("taskId", &info.task_id);
    builder.add_string(JSON_KEY_CALLER_TOKEN_ID, caller);
    builder.add_number("taskExpireTime", info.task_expire_time as i64);
    builder.add_number("executeType", info.execute_type as i64);
    builder.add_bool("allowScreenLockExecution", info.allow_screen_lock_execution);
    builder.add_number("allowableDuration", info.allowable_duration as i64);
    if !info.auth_token.is_empty() {
        let encoded = String::from_utf8_lossy(&base64_encode(&info.auth_token)?).to_string();
        builder.add_string(TASK_ATTR_AUTH_TOKEN, &encoded);
        builder.add_number(TASK_ATTR_ATL, atl as i64);
    }
    builder.build()
}

/// Generates a random challenge for the high-risk task path.
/// Writes challenge to both memory pending and disk partial record.
pub fn generate_task_challenge(
    os_account_id: i32,
    caller: &str,
    info: &TaskIdentityInfo,
) -> Result<Vec<u8>> {
    validate_task_id_info(info)?;

    let mut challenge = vec![0u8; CHALLENGE_SIZE as usize];
    generate_random_bytes(&mut challenge, CHALLENGE_SIZE)?;

    let challenge_b64 = String::from_utf8_lossy(&base64_encode(&challenge)?).to_string();
    global_task_identity_store().insert_challenge(os_account_id, caller, &info.task_id, &challenge_b64)?;

    Ok(challenge)
}

/// Verifies IAM authToken, compares with pending challenge, returns ATL.
/// Consumes pending and compares; pending is discarded after comparison.
fn verify_iam_and_get_challenge(
    os_account_id: i32,
    caller: &str,
    task_id: &str,
    auth_token: &[u8],
    allowable_duration: i32,
) -> Result<u32> {
    let (iam_challenge, atl) = iam_verify_auth_token(auth_token, allowable_duration)?;
    let pending_challenge_b64 = global_task_identity_store()
        .get_challenge(os_account_id, caller, task_id)?;
    let pending_challenge = base64_decode(pending_challenge_b64.as_bytes())?;
    if iam_challenge != pending_challenge {
        return macros_lib::log_throw_error!(ErrCode::TaskIdentityMismatch,
            "IAM challenge does not match pending challenge");
    }
    Ok(atl)
}

/// Generates challenge1 + challenge2, derives session key, computes HMAC,
/// and returns (identity_b64, challenge_b64).
/// HMAC data = taskAttributes (containing callerTokenId) + taskId + challenge2,
/// mirroring the RequestToolPermissions ticket pattern: no dynamic caller identity
/// is spliced in, so verification succeeds regardless of the verifying caller.
fn compute_identity_and_challenge(
    os_account_id: i32,
    caller: &str,
    task_attributes: &str,
    task_id: &str,
) -> Result<(String, String)> {
    let mut challenge1 = vec![0u8; CHALLENGE_SIZE as usize];
    generate_random_bytes(&mut challenge1, CHALLENGE_SIZE)?;
    let mut challenge2 = vec![0u8; CHALLENGE_SIZE as usize];
    generate_random_bytes(&mut challenge2, CHALLENGE_SIZE)?;

    let key_manager = create_ticket_key_manager(caller);
    let session_key = key_manager.derive_ticket_session_key(os_account_id, "", &challenge1)?;

    let mut data = task_attributes.as_bytes().to_vec();
    data.extend_from_slice(task_id.as_bytes());
    data.extend_from_slice(&challenge2);

    let hmac = compute_hmac_sha256(&session_key, &data)?;
    let identity = String::from_utf8_lossy(&base64_encode(&hmac)?).to_string();

    let combined = [challenge1.as_slice(), challenge2.as_slice()].concat();
    let challenge_b64 = String::from_utf8_lossy(&base64_encode(&combined)?).to_string();

    Ok((identity, challenge_b64))
}

/// Generates a task identity using dual-challenge pattern (same as batch_generate_ticket).
///
/// - challenge1 and challenge2: both freshly generated random 32 bytes
/// - challenge1: derives session_key; challenge2: appended to HMAC data
/// - HMAC data = taskAttributes (embeds callerTokenId) + taskId + challenge2
/// - Returns challenge = base64(challenge1 + challenge2), identity = base64(hmac)
pub fn generate_task_identity(
    os_account_id: i32,
    caller: &str,
    info: &TaskIdentityInfo,
) -> Result<TaskIdentity> {
    validate_task_id_info(info)?;

    let mut atl = 0u32;
    if !info.auth_token.is_empty() {
        atl = verify_iam_and_get_challenge(
            os_account_id, caller, &info.task_id, &info.auth_token, info.allowable_duration)?;
    }

    let task_attributes = build_task_attributes(info, caller, atl)?;
    if task_attributes.len() > MAX_TASK_ATTRIBUTES_SIZE {
        return macros_lib::log_throw_error!(ErrCode::InvalidArgument,
            "taskAttributes too large: {} > {}", task_attributes.len(), MAX_TASK_ATTRIBUTES_SIZE);
    }

    let (identity, challenge_b64) = compute_identity_and_challenge(
        os_account_id, caller, &task_attributes, &info.task_id)?;

    // system_time_in_millis returns u64; convert to i64 with range check.
    let now_sec = match saf_utils::system_time_in_millis() {
        Ok(ms) => match i64::try_from(ms / MS_PER_SEC) {
            Ok(v) => v,
            Err(_) => return macros_lib::log_throw_error!(ErrCode::GeneralError, "system time overflow i64"),
        },
        Err(_) => {
            return macros_lib::log_throw_error!(ErrCode::GeneralError, "get system time failed");
        }
    };
    // taskExpireTime is a duration in seconds. -1 means never expires.
    // For -1: stored as i64 -1; all readers use `as u64` which wraps to u64::MAX → never expires.
    // For positive: compute absolute expiration = now + duration, with checked_add overflow check.
    let task_expire_sec = if info.task_expire_time == -1 {
        -1
    } else {
        match now_sec.checked_add(info.task_expire_time as i64) {
            Some(v) => v,
            None => return macros_lib::log_throw_error!(ErrCode::InvalidArgument,
                "taskExpireTime overflow: now={} + expire={}", now_sec, info.task_expire_time),
        }
    };

    let record = TaskIdentityRecord {
        task_attributes: task_attributes.clone(),
        identity: identity.clone(),
        challenge: challenge_b64.clone(),
        task_expire_sec,
        execute_type: info.execute_type,
        allow_screen_lock_execution: info.allow_screen_lock_execution,
        allowable_duration: info.allowable_duration,
    };
    global_task_identity_store()
        .insert_task_entry(os_account_id, caller, &info.task_id, record)?;

    Ok(TaskIdentity {
        task_attributes,
        identity,
        challenge: challenge_b64,
    })
}

/// Revokes task identities by deleting local records (memory + disk).
pub fn revoke_task_identities(os_account_id: i32, caller: &str, task_ids: Vec<String>) -> Result<()> {
    validate_revoke_params(&task_ids)?;
    for task_id in task_ids {
        if task_id.is_empty() {
            continue;
        }
        global_task_identity_store().delete_task_entry(os_account_id, caller, &task_id)?;
    }
    Ok(())
}

/// Verifies HMAC of a task record (same as batch_verify_ticket).
/// Splits combined challenge → challenge1 (derive key) + challenge2 (HMAC data).
/// HMAC data uses the stored task_attributes (which embed the callerTokenId),
/// so no dynamic caller identity is required at verification time.
pub(crate) fn verify_record_hmac(
    os_account_id: i32,
    caller: &str,
    task_id: &str,
    record: &TaskIdentityRecord,
) -> Result<()> {
    let combined = base64_decode(record.challenge.as_bytes())?;
    if combined.len() < COMBINED_CHALLENGE_SIZE {
        return macros_lib::log_throw_error!(ErrCode::InvalidChallengeSize,
            "combined challenge size too small: {}", combined.len());
    }
    let challenge1 = &combined[..CHALLENGE_SIZE as usize];
    let challenge2 = &combined[CHALLENGE_SIZE as usize..];

    let key_manager = create_ticket_key_manager(caller);
    let session_key = key_manager.derive_ticket_session_key(os_account_id, "", challenge1)?;

    let mut data = record.task_attributes.as_bytes().to_vec();
    data.extend_from_slice(task_id.as_bytes());
    data.extend_from_slice(challenge2);

    let expected_hmac = base64_decode(record.identity.as_bytes())?;
    match verify_hmac_sha256(&session_key, &data, &expected_hmac) {
        Ok(true) => Ok(()),
        Ok(false) => macros_lib::log_throw_error!(ErrCode::TaskIdentityMismatch,
            "HMAC verification failed"),
        Err(e) => Err(e),
    }
}

/// Parses `taskId` from the task attributes JSON.
pub(crate) fn parse_task_id(task_attributes: &str) -> Result<String> {
    let json = JsonValue::from_text(task_attributes).map_err(|e| {
        macros_lib::log_and_into_saf_error!(ErrCode::TaskIdentityNotFound,
            "parse taskAttributes failed: {}", e)
    })?;
    let task_id = get_compact_json_value(&json, "taskId")
        .map(|s| s.trim_matches('"').to_string())
        .unwrap_or_default();
    if task_id.is_empty() {
        return macros_lib::log_throw_error!(ErrCode::TaskIdentityNotFound, "taskId is empty");
    }
    Ok(task_id)
}

/// Unified verification spine (same flow as batch_verify_ticket):
/// 1. caller validation → 2. path validation → 3. get record → 4. check expiration
/// → 5. verify HMAC. Returns the verified record. No side effects.
/// No dynamic caller uid is involved: the HMAC is recomputed over the stored
/// task_attributes (which embed the callerTokenId), so the verifying caller may
/// differ from the generating caller as long as the callerToken key matches.
pub(crate) fn fetch_record_and_verify(
    os_account_id: i32,
    caller: &str,
    task_id: &str,
) -> Result<TaskIdentityRecord> {
    if caller.is_empty() || caller == "0" {
        return macros_lib::log_throw_error!(ErrCode::InvalidArgument, "callerToken is invalid");
    }
    validate_path_segment(task_id)?;

    let record = global_task_identity_store().get_task_record(os_account_id, caller, task_id)?;

    let now_sec = match saf_utils::system_time_in_millis() {
        Ok(ms) => ms / MS_PER_SEC,
        Err(e) => {
            logw!("[task_identity] get system time failed: {}", e.msg);
            return macros_lib::log_throw_error!(ErrCode::GeneralError, "get system time failed");
        }
    };
    // task_expire_sec == -1 means never expires.
    // -1i64 as u64 = u64::MAX, so now_sec > u64::MAX is always false → never expires.
    if now_sec > record.task_expire_sec as u64 {
        return macros_lib::log_throw_error!(ErrCode::TaskIdentityExpired, "task identity expired");
    }

    verify_record_hmac(os_account_id, caller, task_id, &record)?;

    Ok(record)
}

/// Compares a verified record against presented identity + challenge.
pub(crate) fn check_task_identity_match(record: &TaskIdentityRecord, identity: &str, challenge: &str) -> Result<()> {
    if record.identity != identity || record.challenge != challenge {
        return macros_lib::log_throw_error!(ErrCode::TaskIdentityMismatch, "task identity mismatched");
    }
    Ok(())
}

/// Verifies a task identity via the shared spine (verify_record_hmac) and an
/// explicit identity/challenge comparison against the stored record.
pub fn verify_task_identity(
    os_account_id: i32,
    caller: &str,
    task_attributes: &str,
    identity: &str,
    challenge: &str,
) -> Result<()> {
    if caller.is_empty() || caller == "0" {
        return macros_lib::log_throw_error!(ErrCode::InvalidArgument, "callerToken is invalid");
    }
    let task_id = parse_task_id(task_attributes)?;
    let record = fetch_record_and_verify(os_account_id, caller, &task_id)?;
    check_task_identity_match(&record, identity, challenge)
}

/// Extracts authToken from taskAttributes JSON (base64 decode → raw bytes).
/// Returns empty Vec for low-risk tasks (no authToken field).
fn extract_auth_token(task_attributes: &str) -> Result<Vec<u8>> {
    let json = JsonValue::from_text(task_attributes).map_err(|e| {
        macros_lib::log_and_into_saf_error!(ErrCode::TaskIdentityMismatch,
            "parse taskAttributes for authToken failed: {}", e)
    })?;
    match get_compact_json_value(&json, TASK_ATTR_AUTH_TOKEN).ok() {
        Some(b64) if !b64.is_empty() => {
            let token = base64_decode(b64.trim_matches('"').as_bytes())?;
            Ok(token)
        }
        _ => Ok(Vec::new()),
    }
}

/// Retrieves authToken by callerToken + taskId via the shared verification spine.
/// Note: INSTANT task consumption (delete after verify) is handled by the ticket
/// verification flow (verify_ticket_impl), not by this function.
pub fn retrieve_auth_token_by_task_id(
    os_account_id: i32,
    caller: &str,
    task_id: &str,
) -> Result<Vec<u8>> {
    let record = fetch_record_and_verify(os_account_id, caller, task_id)?;
    extract_auth_token(&record.task_attributes)
}

/// Verifies the task identity embedded in a ticket message (if present).
/// Consumes INSTANT tasks after successful verification.
pub(crate) fn verify_task_identity_and_consume(os_account_id: i32, message_json: &ylong_json::JsonValue) -> Result<()> {
    let task_identity_obj = &message_json[JSON_KEY_TASK_IDENTITY];
    if task_identity_obj == &ylong_json::JsonValue::Null {
        return Ok(());
    }
    let task_attributes = get_compact_json_value(task_identity_obj, JSON_KEY_TASK_ATTRIBUTES)?;
    let identity = get_compact_json_value(task_identity_obj, JSON_KEY_TASK_IDENTITY_IDENTITY)?;
    let challenge = get_compact_json_value(task_identity_obj, JSON_KEY_TASK_CHALLENGE).unwrap_or_default();
    let caller = get_compact_json_value(message_json, JSON_KEY_CALLER_TOKEN_ID)?;
    if caller.is_empty() || caller == "0" {
        return macros_lib::log_throw_error!(ErrCode::InvalidArgument, "callerToken is invalid");
    }

    let task_id = parse_task_id(&task_attributes)?;
    let record = fetch_record_and_verify(os_account_id, &caller, &task_id)?;
    check_task_identity_match(&record, &identity, &challenge)?;
    let execute_type = record.execute_type;
    if execute_type == ExecuteType::Instant {
        logi!("VerifyTicket: INSTANT task verified, consuming task identity");
        if let Err(e) = global_task_identity_store().delete_task_entry(os_account_id, &caller, &task_id) {
            loge!("VerifyTicket: consume task identity failed, err={}", e.code);
        }
    }
    Ok(())
}