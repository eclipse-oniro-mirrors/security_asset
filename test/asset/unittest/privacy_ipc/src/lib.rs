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

//! Single-message IPC tests; symmetric payloads have no IPC representation.
use asset_ipc::privacy::*;
use ipc::parcel::MsgParcel;

fn protocol() -> PrivacyProtocol {
    PrivacyProtocol { data_set_size: 1, protocol_type: 1 }
}

#[test]
fn target_round_trip() {
    let value = TargetRequest { protocol: protocol(), hash_alg: 0,
        element: vec![0x5A; 1024] };
    let mut parcel = MsgParcel::new();
    serialize_target(&value, &mut parcel).unwrap();
    let result = deserialize_target(&mut parcel).unwrap();
    assert_eq!(result.element, value.element);
    assert_eq!(result.hash_alg, 0);
}

#[test]
fn decrypt_round_trip_and_field_order() {
    let value = DecryptRequest { protocol: protocol(), ciphertext: vec![0, 127, 255] };
    let mut parcel = MsgParcel::new();
    serialize_decrypt(&value, &mut parcel).unwrap();
    assert_eq!(parcel.read::<u32>().unwrap(), 1);
    assert_eq!(parcel.read::<u32>().unwrap(), 1);
    assert_eq!(read_payload(&mut parcel).unwrap(), value.ciphertext);
    let mut parcel = MsgParcel::new();
    serialize_decrypt(&value, &mut parcel).unwrap();
    assert_eq!(deserialize_decrypt(&mut parcel).unwrap().ciphertext, value.ciphertext);
}

#[test]
fn decrypt_rejects_empty_ciphertext() {
    let value = DecryptRequest { protocol: protocol(), ciphertext: Vec::new() };
    assert!(serialize_decrypt(&value, &mut MsgParcel::new()).is_err());
}

#[test]
fn aggregate_ciphertexts_exceed_ipc_limit_without_a_transfer_session() {
    let batches = vec![vec![0xA5; 100 * 1024]; 8];
    assert!(batches.iter().map(Vec::len).sum::<usize>() > 200 * 1024);
    for ciphertext in batches {
        let value = DecryptRequest { protocol: protocol(), ciphertext };
        let mut parcel = MsgParcel::new();
        serialize_decrypt(&value, &mut parcel).unwrap();
        assert_eq!(deserialize_decrypt(&mut parcel).unwrap().ciphertext, value.ciphertext);
    }
}

#[test]
fn decoded_slot_bytes_are_transported_without_reordering() {
    let mut bytes = vec![0; MAX_DECRYPT_RESULT_BYTES];
    bytes[..8].copy_from_slice(&1u64.to_le_bytes());
    bytes[MAX_DECRYPT_RESULT_BYTES - 8..].copy_from_slice(&65536u64.to_le_bytes());
    let mut parcel = MsgParcel::new();
    write_payload(&bytes, &mut parcel).unwrap();
    assert_eq!(read_payload(&mut parcel).unwrap(), bytes);
}

#[test]
fn native_view_borrows_ciphertext_storage() {
    let request = DecryptRequest { protocol: protocol(), ciphertext: vec![1, 2, 3] };
    let native = request.as_native();
    assert_eq!(native.ciphertext.data, request.ciphertext.as_ptr());
    assert_eq!(native.ciphertext.size as usize, request.ciphertext.len());
}

#[test]
fn declared_payload_longer_than_parcel_is_rejected() {
    for size in [1, u32::MAX] {
        let mut parcel = MsgParcel::new();
        parcel.write(&size).unwrap();
        assert!(read_payload(&mut parcel).is_err());
    }
    let mut parcel = MsgParcel::new();
    parcel.write(&1u32).unwrap();
    assert!(deserialize_decrypt(&mut parcel).is_err());
}

#[test]
fn native_layout_matches_c_abi() {
    let wide = std::mem::size_of::<usize>() == 8;
    assert_eq!(std::mem::size_of::<PrivacyProtocol>(), 8);
    assert_eq!(std::mem::size_of::<PrivacyBlob>(), if wide { 16 } else { 8 });
    assert_eq!(std::mem::size_of::<PrivacyDecryptRequest>(), if wide { 24 } else { 16 });
    assert_eq!(std::mem::size_of::<PrivacyTargetRequest>(), if wide { 32 } else { 20 });
    assert_eq!(std::mem::size_of::<PrivacyBuffer>(), if wide { 16 } else { 8 });
}
