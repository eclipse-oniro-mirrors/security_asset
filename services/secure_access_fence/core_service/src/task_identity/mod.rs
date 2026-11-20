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

pub(crate) mod constants;
pub(crate) mod store;
pub(crate) mod storage_backend;
pub(crate) mod validator;

pub(crate) use constants::TaskIdentityKeys;
pub(crate) use validator::{is_from_initial_wake_up, validate_path_segment};

use saf_common::JsonBuilder;
use saf_definition::{macros_lib, ErrCode, ExecuteType, Result, TaskIdentity, TaskIdentityInfo};
use saf_log::{loge, logi, logw};
use saf_utils::{JsonValue, get_compact_json_value, to_hex};

use crate::JSON_KEY_CALLER_UID;
use crate::ticket_operation::{
    base64_decode, base64_encode, compute_hmac_sha256, constant_time_eq, create_ticket_key_manager,
    generate_random_bytes, verify_hmac_sha256,
};
use constants::{
    CHALLENGE_SIZE, COMBINED_CHALLENGE_SIZE, MS_PER_SEC, MAX_TASK_ATTRIBUTES_SIZE,
    INITIAL_WAKE_UP_FILE_PREFIX, INITIAL_WAKE_UP_FILENAME_DOMAIN,
};
use store::{global_task_identity_store, TaskIdentityRecord};
use TaskIdentityKeys as K;

/// Converts encoded bytes into a UTF-8 string without lossy replacement.
fn encoded_to_string(bytes: Vec<u8>) -> Result<String> {
    String::from_utf8(bytes).map_err(|e| {
        macros_lib::log_and_into_saf_error!(ErrCode::GeneralError, "encoded data is not valid UTF-8: {}", e)
    })
}

/// Derives a non-reversible file name stem from the challenge:
/// `InitialWakeUpChallenge_<hex(HMAC-SHA256(domain, challenge))>`.
fn initial_challenge_file_stem(challenge: &[u8]) -> Result<String> {
    let digest = compute_hmac_sha256(INITIAL_WAKE_UP_FILENAME_DOMAIN, challenge)?;
    Ok(format!("{}{}", INITIAL_WAKE_UP_FILE_PREFIX, encoded_to_string(to_hex(&digest))?))
}

/// Verifies auth token via IAM. Returns (challenge, authTrustLevel).
/// challenge is compared with pending challenge from generateTaskChallenge.
/// authTrustLevel is written into taskAttributes.
/// allowableDuration is in milliseconds, per IAM's constraint (0, 86400000].
fn iam_verify_auth_token(auth_token: &[u8], allowable_duration_ms: i32) -> Result<(Vec<u8>, u32)> {
    if allowable_duration_ms <= 0 {
        return macros_lib::log_throw_error!(ErrCode::InvalidArgument,
            "allowableDuration must be positive: {}", allowable_duration_ms);
    }
    let token_vec: Vec<u8> = auth_token.to_vec();
    let result = crate::wrapper::ffi::VerifyAuthToken(&token_vec, allowable_duration_ms as u64);
    if result.result_code != 0 {
        return macros_lib::log_throw_error!(ErrCode::GeneralError,
            "IAM VerifyAuthToken failed, result_code={}", result.result_code);
    }
    Ok((result.challenge, result.auth_trust_level))
}

/// Redeems a scheduled auth token into a short-lived real-time auth token via IAM.
/// A fresh random challenge is generated internally and bound to the redeemed token.
/// Returns (redeem_challenge, real_time_token): the caller compares the redeem-time
/// random challenge with the challenge returned by VerifyAuthToken.
fn redeem_scheduled_auth_token(schedule_token: &[u8]) -> Result<(Vec<u8>, Vec<u8>)> {
    let mut challenge = vec![0u8; CHALLENGE_SIZE as usize];
    generate_random_bytes(&mut challenge, CHALLENGE_SIZE)?;

    let schedule_token_vec: Vec<u8> = schedule_token.to_vec();
    let result = crate::wrapper::ffi::RedeemScheduledAuthToken(&challenge, &schedule_token_vec);
    if result.result_code != 0 {
        return macros_lib::log_throw_error!(ErrCode::GeneralError,
            "IAM RedeemScheduledAuthToken failed, result_code={}", result.result_code);
    }
    if result.token.is_empty() {
        return macros_lib::log_throw_error!(ErrCode::GeneralError,
            "IAM RedeemScheduledAuthToken returned empty token");
    }
    Ok((challenge, result.token))
}

/// Computes absolute expiration time (epoch seconds).
/// -1 means never expires; positive duration is added to current time.
fn compute_task_expire_sec(task_expire_time: i32) -> Result<i64> {
    if task_expire_time == -1 {
        return Ok(-1);
    }
    let now_sec = match saf_utils::system_time_in_millis() {
        Ok(ms) => match i64::try_from(ms / MS_PER_SEC) {
            Ok(v) => v,
            Err(_) => return macros_lib::log_throw_error!(ErrCode::GeneralError, "system time overflow i64"),
        },
        Err(_) => {
            return macros_lib::log_throw_error!(ErrCode::GeneralError, "get system time failed");
        }
    };
    now_sec.checked_add(task_expire_time as i64).ok_or_else(|| {
        macros_lib::SAFError::new(ErrCode::InvalidArgument,
            format!("taskExpireTime overflow: now={} + expire={}", now_sec, task_expire_time))
    })
}

/// Builds the task attributes JSON that is embedded into the task identity and covered by HMAC.
/// `caller` is the generating application's uid.
fn build_task_attributes(info: &TaskIdentityInfo, caller: u32, atl: u32) -> Result<String> {
    let task_expire_sec = compute_task_expire_sec(info.task_expire_time)?;
    let mut builder = JsonBuilder::new();
    builder.add_string(K::TASK_ID, &info.task_id);
    builder.add_string(JSON_KEY_CALLER_UID, &caller.to_string());
    builder.add_number(K::TASK_EXPIRE_TIME, info.task_expire_time as i64);
    builder.add_number(K::TASK_EXPIRE_SEC, task_expire_sec);
    builder.add_number(K::EXECUTE_TYPE, info.execute_type as i64);
    builder.add_bool(K::ALLOW_SCREEN_LOCK_EXECUTION, info.allow_screen_lock_execution);
    builder.add_number(K::ALLOWABLE_DURATION, info.allowable_duration as i64);
    if !info.auth_token.is_empty() {
        let encoded = encoded_to_string(base64_encode(&info.auth_token)?)?;
        builder.add_string(K::AUTH_TOKEN, &encoded);
        builder.add_number(K::ATL, atl as i64);
    }
    builder.build()
}

/// Generates a random challenge for the high-risk task path.
/// Writes challenge to both memory pending and disk partial record.
pub fn generate_task_challenge(
    os_account_id: i32,
    caller: u32,
    info: &mut TaskIdentityInfo,
) -> Result<Vec<u8>> {
    let mut challenge = vec![0u8; CHALLENGE_SIZE as usize];
    generate_random_bytes(&mut challenge, CHALLENGE_SIZE)?;

    let challenge_b64 = encoded_to_string(base64_encode(&challenge)?)?;
    if is_from_initial_wake_up(&info.task_id) {
        let file_stem = initial_challenge_file_stem(&challenge)?;
        global_task_identity_store().insert_initial_challenge(
            os_account_id, caller, &file_stem, &challenge_b64)?;
    } else {
        global_task_identity_store().insert_challenge(
            os_account_id, caller, &info.task_id, &challenge_b64)?;
    }

    Ok(challenge)
}

/// Verifies IAM authToken and returns ATL.
/// DELAYED tasks: verify the scheduled token.
/// INSTANT tasks: verify the authToken directly.
/// Both path compare the IAM-returned challenge with the pending challenge stored by generateTaskChallenge
/// (consumed on read), falling back to the reusable initial-wakeup challenge.
fn verify_iam_and_get_challenge(
    os_account_id: i32,
    caller: u32,
    info: &TaskIdentityInfo,
) -> Result<u32> {
    let (iam_challenge, atl) = iam_verify_auth_token(&info.auth_token, info.allowable_duration)?;
    verify_against_stored_challenge(os_account_id, caller, &info.task_id, &iam_challenge)?;
    Ok(atl)
}

/// Compares the IAM-returned challenge with the pending challenge stored by
/// generateTaskChallenge (consumed on read). Falls back to the reusable
/// initial-wakeup challenge when no task-bound challenge exists (INSTANT only).
fn verify_against_stored_challenge(
    os_account_id: i32,
    caller: u32,
    task_id: &str,
    iam_challenge: &[u8],
) -> Result<()> {
    let store = global_task_identity_store();
    match store.get_challenge(os_account_id, caller, task_id) {
        Ok(pending_challenge_b64) => {
            // Task-bound challenge takes precedence and is consumed.
            let pending_challenge = base64_decode(pending_challenge_b64.as_bytes())?;
            if !constant_time_eq(iam_challenge, &pending_challenge)? {
                return macros_lib::log_throw_error!(ErrCode::TaskIdentityMismatch,
                    "IAM challenge does not match pending challenge");
            }
            Ok(())
        },
        Err(e) if e.code == ErrCode::TaskIdentityNotFound => {
            // No task-bound challenge: fall back to a reusable initial-wakeup challenge.
            let iam_challenge_b64 = encoded_to_string(base64_encode(iam_challenge)?)?;
            let file_stem = initial_challenge_file_stem(iam_challenge)?;
            store.check_initial_challenge(os_account_id, caller, &file_stem, &iam_challenge_b64)
        },
        Err(e) => Err(e),
    }
}

/// Generates challenge1 + challenge2, derives session key, computes HMAC,
/// and returns (identity_b64, challenge_b64).
/// HMAC data = taskAttributes (containing callerUid) + taskId + challenge2,
/// mirroring the RequestToolPermissions ticket pattern: no dynamic caller identity
/// is spliced in, so verification succeeds regardless of the verifying caller.
fn compute_identity_and_challenge(
    os_account_id: i32,
    caller: u32,
    task_attributes: &str,
    task_id: &str,
) -> Result<(String, String)> {
    let mut challenge1 = vec![0u8; CHALLENGE_SIZE as usize];
    generate_random_bytes(&mut challenge1, CHALLENGE_SIZE)?;
    let mut challenge2 = vec![0u8; CHALLENGE_SIZE as usize];
    generate_random_bytes(&mut challenge2, CHALLENGE_SIZE)?;

    let key_manager = create_ticket_key_manager(&caller.to_string());
    let session_key = key_manager.derive_ticket_session_key(os_account_id, "", &challenge1)?;

    let mut data = task_attributes.as_bytes().to_vec();
    data.extend_from_slice(task_id.as_bytes());
    data.extend_from_slice(&challenge2);

    let hmac = compute_hmac_sha256(&session_key, &data)?;
    let identity = encoded_to_string(base64_encode(&hmac)?)?;

    let combined = [challenge1.as_slice(), challenge2.as_slice()].concat();
    let challenge_b64 = encoded_to_string(base64_encode(&combined)?)?;

    Ok((identity, challenge_b64))
}

/// Generates a task identity using dual-challenge pattern (same as batch_generate_ticket).
///
/// - challenge1 and challenge2: both freshly generated random 32 bytes
/// - challenge1: derives session_key; challenge2: appended to HMAC data
/// - HMAC data = taskAttributes (embeds callerUid) + taskId + challenge2
/// - Returns challenge = base64(challenge1 + challenge2), identity = base64(hmac)
pub fn generate_task_identity(
    os_account_id: i32,
    caller: u32,
    info: &mut TaskIdentityInfo,
) -> Result<TaskIdentity> {
    if is_from_initial_wake_up(&info.task_id) {
        return macros_lib::log_throw_error!(ErrCode::InvalidArgument,
            "InitialWakeUp is a reserved task id");
    }
    let mut atl = 0u32;
    if !info.auth_token.is_empty() {
        atl = verify_iam_and_get_challenge(os_account_id, caller, info)?;
    }

    let task_attributes = build_task_attributes(info, caller, atl)?;
    if task_attributes.len() > MAX_TASK_ATTRIBUTES_SIZE {
        return macros_lib::log_throw_error!(ErrCode::InvalidArgument,
            "taskAttributes too large: {} > {}", task_attributes.len(), MAX_TASK_ATTRIBUTES_SIZE);
    }

    let (identity, challenge_b64) = compute_identity_and_challenge(
        os_account_id, caller, &task_attributes, &info.task_id)?;

    let task_expire_sec = compute_task_expire_sec(info.task_expire_time)?;
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
pub fn revoke_task_identities(os_account_id: i32, caller: u32, task_ids: Vec<String>) -> Result<()> {
    for task_id in task_ids {
        if task_id.is_empty() {
            continue;
        }
        if is_from_initial_wake_up(&task_id) {
            return macros_lib::log_throw_error!(ErrCode::InvalidArgument,
                "InitialWakeUp is a reserved task id");
        }
        global_task_identity_store().delete_task_entry(os_account_id, caller, &task_id)?;
    }
    Ok(())
}

/// Verifies HMAC of a task record (same as batch_verify_ticket).
/// Splits combined challenge → challenge1 (derive key) + challenge2 (HMAC data).
/// HMAC data uses the stored task_attributes (which embed the callerUid),
/// so no dynamic caller identity is required at verification time.
pub(crate) fn verify_record_hmac(
    os_account_id: i32,
    caller: u32,
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

    let key_manager = create_ticket_key_manager(&caller.to_string());
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
    let task_id = get_compact_json_value(&json, K::TASK_ID)
        .map(|s| s.trim_matches('"').to_string())
        .unwrap_or_default();
    if task_id.is_empty() {
        return macros_lib::log_throw_error!(ErrCode::TaskIdentityNotFound, "taskId is empty");
    }
    Ok(task_id)
}

/// Unified verification spine (same flow as batch_verify_ticket):
/// Returns the verified record.
pub(crate) fn fetch_record_and_verify(
    os_account_id: i32,
    caller: u32,
    task_id: &str,
) -> Result<TaskIdentityRecord> {
    if caller == 0 {
        return macros_lib::log_throw_error!(ErrCode::InvalidArgument, "caller is invalid");
    }
    if is_from_initial_wake_up(task_id) {
        return macros_lib::log_throw_error!(ErrCode::InvalidArgument,
            "InitialWakeUp is a reserved task id");
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
        // Async delete: memory first, then disk file
        let os_id = os_account_id;
        let task_id_owned = task_id.to_string();
        ylong_runtime::spawn_blocking(move || {
            let _ = global_task_identity_store().delete_task_entry(os_id, caller, &task_id_owned);
        });
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
    caller: u32,
    task_attributes: &str,
    identity: &str,
    challenge: &str,
) -> Result<()> {
    if caller == 0 {
        return macros_lib::log_throw_error!(ErrCode::InvalidArgument, "caller is invalid");
    }
    let task_id = parse_task_id(task_attributes)?;
    let record = fetch_record_and_verify(os_account_id, caller, &task_id)?;
    check_task_identity_match(&record, identity, challenge)?;
    verify_task_auth_token(&record)
}

/// Verifies the authToken stored in the task record.
/// INSTANT tasks: verifies the stored normal authToken directly.
/// DELAYED tasks: redeems the stored scheduled authToken first,
/// then verifies the redeemed real-time authToken.
fn verify_task_auth_token(record: &TaskIdentityRecord) -> Result<()> {
    let stored_token = extract_auth_token(&record.task_attributes)?;
    if stored_token.is_empty() {
        return Ok(());
    }
    if record.execute_type == ExecuteType::Delayed {
        let (_, real_time_token) = redeem_scheduled_auth_token(&stored_token)?;
        iam_verify_auth_token(&real_time_token, record.allowable_duration)?;
    } else {
        iam_verify_auth_token(&stored_token, record.allowable_duration)?;
    }
    Ok(())
}

/// Extracts authToken from taskAttributes JSON (base64 decode → raw bytes).
/// Returns empty Vec for low-risk tasks (no authToken field).
fn extract_auth_token(task_attributes: &str) -> Result<Vec<u8>> {
    let json = JsonValue::from_text(task_attributes).map_err(|e| {
        macros_lib::log_and_into_saf_error!(ErrCode::TaskIdentityMismatch,
            "parse taskAttributes for authToken failed: {}", e)
    })?;
    match get_compact_json_value(&json, K::AUTH_TOKEN).ok() {
        Some(b64) if !b64.is_empty() => {
            let token = base64_decode(b64.trim_matches('"').as_bytes())?;
            Ok(token)
        }
        _ => Ok(Vec::new()),
    }
}

/// Retrieves authToken by uid + taskId via the shared verification spine.
/// INSTANT tasks: returns the stored normal authToken directly.
/// DELAYED tasks: the stored token is a scheduled auth token, redeemed into
/// a short-lived real-time auth token via IAM before being returned.
pub fn retrieve_auth_token_by_task_id(
    os_account_id: i32,
    caller: u32,
    task_id: &str,
) -> Result<Vec<u8>> {
    let record = fetch_record_and_verify(os_account_id, caller, task_id)?;
    let stored_token = extract_auth_token(&record.task_attributes)?;
    if stored_token.is_empty() {
        return Ok(Vec::new());
    }
    if record.execute_type == ExecuteType::Delayed {
        redeem_scheduled_auth_token(&stored_token).map(|(_, real_time_token)| real_time_token)
    } else {
        Ok(stored_token)
    }
}

/// Extracts the callerUid embedded in the taskAttributes JSON (written by the
/// generate side from the same binder uid used as the store key), so verify
/// lookups stay consistent with generation without requiring a top-level
/// callerUid field in the ticket message.
pub(crate) fn parse_caller_uid(task_attributes: &str) -> Result<u32> {
    let json = JsonValue::from_text(task_attributes).map_err(|e| {
        macros_lib::log_and_into_saf_error!(ErrCode::InvalidArgument,
            "parse taskAttributes for callerUid failed: {}", e)
    })?;
    let caller_str = get_compact_json_value(&json, JSON_KEY_CALLER_UID).map_err(|e| {
        macros_lib::log_and_into_saf_error!(ErrCode::InvalidArgument,
            "callerUid not found in taskAttributes: {}", e)
    })?;
    caller_str.trim_matches('"').parse::<u32>().map_err(|_| {
        macros_lib::log_and_into_saf_error!(ErrCode::InvalidArgument, "callerUid is not a valid u32")
    })
}

/// Verifies the task identity embedded in a ticket message (if present).
/// Consumes INSTANT tasks after successful verification.
pub(crate) fn verify_task_identity_and_consume(
    os_account_id: i32,
    message_json: &ylong_json::JsonValue
) -> Result<()> {
    let task_identity_obj = &message_json[K::TASK_IDENTITY];
    if task_identity_obj == &ylong_json::JsonValue::Null {
        return Ok(());
    }
    let task_attributes = get_compact_json_value(task_identity_obj, K::TASK_ATTRIBUTES)?;
    let identity = get_compact_json_value(task_identity_obj, K::IDENTITY)?;
    let challenge = get_compact_json_value(task_identity_obj, K::CHALLENGE).unwrap_or_default();
    // callerUid lives inside the taskAttributes JSON (written by the generate
    // side from the same binder uid used as the store key).
    let caller = parse_caller_uid(&task_attributes)?;
    if caller == 0 {
        return macros_lib::log_throw_error!(ErrCode::InvalidArgument, "callerUid is invalid");
    }

    let task_id = parse_task_id(&task_attributes)?;
    let record = fetch_record_and_verify(os_account_id, caller, &task_id)?;
    check_task_identity_match(&record, &identity, &challenge)?;
    let execute_type = record.execute_type;
    if execute_type == ExecuteType::Instant {
        logi!("VerifyTicket: INSTANT task verified, consuming task identity");
        if let Err(e) = global_task_identity_store().delete_task_entry(os_account_id, caller, &task_id) {
            loge!("VerifyTicket: consume task identity failed, err={}", e.code);
        }
    }
    Ok(())
}