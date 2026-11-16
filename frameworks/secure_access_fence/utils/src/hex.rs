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

//! This module implements lowercase hexadecimal encoding.

const HEX: &[u8; 16] = b"0123456789abcdef";
const BITS_PER_HEX_DIGIT: u32 = 4;
const LOW_NIBBLE_MASK: u8 = (1 << BITS_PER_HEX_DIGIT) - 1;
const HEX_DIGITS_PER_BYTE: usize = 2;

/// Encodes `bytes` as lowercase hexadecimal bytes.
pub fn to_hex(bytes: &[u8]) -> Vec<u8> {
    let mut out = Vec::with_capacity(bytes.len() * HEX_DIGITS_PER_BYTE);
    for &byte in bytes {
        out.push(HEX[(byte >> BITS_PER_HEX_DIGIT) as usize]);
        out.push(HEX[(byte & LOW_NIBBLE_MASK) as usize]);
    }
    out
}