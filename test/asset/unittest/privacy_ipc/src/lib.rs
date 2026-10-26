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

//! Regression tests for field order, optional values and native input boundaries.
use asset_ipc::privacy::*;
use ipc::parcel::MsgParcel;
use std::ptr::null;

#[test]
fn target_fields_round_trip() {
    let input = TargetRequest {
        protocol: PrivacyProtocol { data_set_size: 2, protocol_type: 1 },
        hash_alg: 0,
        element: vec![0, 127, 255],
    };
    let mut parcel = MsgParcel::new();
    serialize_target(&input, &mut parcel).unwrap();
    let output = deserialize_target(&mut parcel).unwrap();
    assert_eq!(output.protocol.data_set_size, 2);
    assert_eq!(output.protocol.protocol_type, 1);
    assert_eq!(output.hash_alg, 0);
    assert_eq!(output.element, input.element);
}

#[test]
fn search_preserves_absent_and_present_empty_lists() {
    for symmetric in [None, Some(Vec::new()), Some(vec![vec![], vec![0, 255]])] {
        let input = SearchRequest {
            protocol: PrivacyProtocol { data_set_size: 1, protocol_type: 2 },
            ciphertexts: vec![vec![1, 2], vec![3]],
            symmetric,
        };
        let mut parcel = MsgParcel::new();
        serialize_search(&input, &mut parcel).unwrap();
        let output = deserialize_search(&mut parcel).unwrap();
        assert_eq!(output.ciphertexts, input.ciphertexts);
        assert_eq!(output.symmetric, input.symmetric);
    }
}

#[test]
fn result_preserves_match_and_optional_values() {
    for values in [None, Some(Vec::new()), Some(vec![vec![], vec![0, 255]])] {
        let input = SearchResponse { matched: true, values };
        let mut parcel = MsgParcel::new();
        serialize_result(&input, &mut parcel).unwrap();
        let output = deserialize_result(&mut parcel).unwrap();
        assert!(output.matched);
        assert_eq!(output.values, input.values);
    }
}

#[test]
fn oversized_list_count_is_rejected_before_items() {
    let mut parcel = MsgParcel::new();
    parcel.write(&1u32).unwrap();
    parcel.write(&2u32).unwrap();
    parcel.write(&((MAX_ITEMS + 1) as u32)).unwrap();
    assert!(deserialize_search(&mut parcel).is_err());
}

#[test]
fn truncated_target_is_rejected() {
    let mut parcel = MsgParcel::new();
    parcel.write(&1u32).unwrap();
    assert!(deserialize_target(&mut parcel).is_err());
}

#[test]
fn native_target_checks_length_and_pointer() {
    let mut input = PrivacyTargetRequest {
        protocol: PrivacyProtocol { data_set_size: 1, protocol_type: 2 },
        hash_alg: 0,
        element: PrivacyBlob { data: null(), size: 0 },
    };
    assert!(unsafe { input.to_value() }.is_ok());
    input.element.size = 1;
    assert!(unsafe { input.to_value() }.is_err());
    input.element.size = MAX_BYTES as u32;
    assert!(unsafe { input.to_value() }.is_err());
    input.element.size = 0;
    assert!(unsafe { input.to_value() }.is_ok());
}

#[test]
fn combined_lists_share_one_size_budget() {
    let input = SearchRequest {
        protocol: PrivacyProtocol { data_set_size: 1, protocol_type: 2 },
        ciphertexts: vec![vec![0; MAX_BYTES / 2]],
        symmetric: Some(vec![vec![0; MAX_BYTES / 2]]),
    };
    assert!(serialize_search(&input, &mut MsgParcel::new()).is_err());
}

#[test]
fn target_boundary_includes_all_length_fields() {
    let mut input = TargetRequest {
        protocol: PrivacyProtocol { data_set_size: 1, protocol_type: 2 },
        hash_alg: 0,
        element: vec![0; MAX_BYTES - TARGET_OVERHEAD_BYTES],
    };
    assert!(unsafe { input.as_native().to_value() }.is_ok());
    input.element.push(0);
    assert!(unsafe { input.as_native().to_value() }.is_err());
}

#[test]
fn native_layout_matches_c_abi_sizes() {
    let wide = std::mem::size_of::<usize>() == 8;
    assert_eq!(std::mem::size_of::<PrivacyProtocol>(), 8);
    assert_eq!(std::mem::size_of::<PrivacyBlob>(), if wide { 16 } else { 8 });
    assert_eq!(std::mem::size_of::<PrivacyBlobList>(), if wide { 16 } else { 8 });
    assert_eq!(std::mem::size_of::<PrivacyTargetRequest>(), if wide { 32 } else { 20 });
    assert_eq!(std::mem::size_of::<PrivacySearchRequest>(), if wide { 48 } else { 28 });
    assert_eq!(std::mem::size_of::<PrivacyBuffer>(), if wide { 16 } else { 8 });
    assert_eq!(std::mem::size_of::<PrivacyResult>(), if wide { 32 } else { 20 });
}
