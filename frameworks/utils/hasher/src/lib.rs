/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

//! This crate implements the sha256
#![allow(dead_code)]

const LOWER_BYTES_MASK: u32 = 0xff;
const BITS_PER_U8: usize = 8;
const U8_PER_U32: usize = 4;
const SHA256_LEN: usize = 32;
const BYTES_PER_CHUNK: usize = 64;

const SHA256_H: [u32; 8] =
    [0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19];

const SHA256_K: [u32; 64] = [
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5, 0xd807aa98,
    0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
    0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8,
    0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819,
    0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
    0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7,
    0xc67178f2,
];

fn expand_chunk(plain_chunk: [u8; BYTES_PER_CHUNK]) -> [u32; BYTES_PER_CHUNK] {
    let mut expanded_chunk = [0; BYTES_PER_CHUNK];
    for (i, item) in expanded_chunk.iter_mut().enumerate().take(16) {
        let offset = i * U8_PER_U32;
        *item = ((plain_chunk[offset] as u32) << 24)
            | ((plain_chunk[offset + 1] as u32) << 16)
            | ((plain_chunk[offset + 2] as u32) << 8)
            | (plain_chunk[offset + 3] as u32);
    }

    for i in 16..64 {
        let s0 = expanded_chunk[i - 15].rotate_right(7)
            ^ expanded_chunk[i - 15].rotate_right(18)
            ^ (expanded_chunk[i - 15] >> 3);
        let s1 = expanded_chunk[i - 2].rotate_right(17)
            ^ expanded_chunk[i - 2].rotate_right(19)
            ^ (expanded_chunk[i - 2] >> 10);
        expanded_chunk[i] =
            expanded_chunk[i - 16].wrapping_add(s0).wrapping_add(expanded_chunk[i - 7]).wrapping_add(s1);
    }
    expanded_chunk
}

fn compress_chunk(expanded_chunk: [u32; 64]) -> [u32; 8] {
    let mut compressed_chunk: [u32; 8] = SHA256_H;
    for i in 0..64 {
        let s1 = compressed_chunk[4].rotate_right(6)
            ^ compressed_chunk[4].rotate_right(11)
            ^ compressed_chunk[4].rotate_right(25);
        let choose = (compressed_chunk[4] & compressed_chunk[5]) ^ ((!compressed_chunk[4]) & compressed_chunk[6]);
        let temp1 = compressed_chunk[7]
            .wrapping_add(s1)
            .wrapping_add(choose)
            .wrapping_add(SHA256_K[i])
            .wrapping_add(expanded_chunk[i]);
        let s0 = compressed_chunk[0].rotate_right(2)
            ^ compressed_chunk[0].rotate_right(13)
            ^ compressed_chunk[0].rotate_right(22);
        let major = (compressed_chunk[0] & compressed_chunk[1])
            ^ (compressed_chunk[0] & compressed_chunk[2])
            ^ (compressed_chunk[1] & compressed_chunk[2]);
        let temp2 = s0.wrapping_add(major);
        compressed_chunk[7] = compressed_chunk[6];
        compressed_chunk[6] = compressed_chunk[5];
        compressed_chunk[5] = compressed_chunk[4];
        compressed_chunk[4] = compressed_chunk[3].wrapping_add(temp1);
        compressed_chunk[3] = compressed_chunk[2];
        compressed_chunk[2] = compressed_chunk[1];
        compressed_chunk[1] = compressed_chunk[0];
        compressed_chunk[0] = temp1.wrapping_add(temp2);
    }
    compressed_chunk
}

fn compress(input_bytes: &[u8]) -> [u32; 8] {
    let mut compress = SHA256_H;
    let chunk_num = input_bytes.len() / BYTES_PER_CHUNK;
    for i in 0..chunk_num {
        // the try_into of array cannot be failed, for the length of plain_chunk is sure to be 64 as expected
        let expanded_chunk =
            expand_chunk(input_bytes[i * BYTES_PER_CHUNK..(i + 1) * BYTES_PER_CHUNK].try_into().unwrap());
        let compressed_chunk: [u32; 8] = compress_chunk(expanded_chunk);
        for j in 0..8 {
            compress[j] = compress[j].wrapping_add(compressed_chunk[j]);
        }
    }
    compress
}

fn pre_process_plain(plain: &Vec<u8>) -> Vec<u8> {
    // padding
    let mut process_plain = plain.clone();
    let plain_len = plain.len();
    let padding_len = if plain_len % BYTES_PER_CHUNK < 56 {
        56 - plain_len % BYTES_PER_CHUNK
    } else {
        120 - plain_len % BYTES_PER_CHUNK
    };

    process_plain.push(0x80); // 1000 0000

    process_plain.append(&mut vec![0x00; padding_len - 1]);

    let plain_bit_len = plain_len * BITS_PER_U8;
    for i in 0..8 {
        let split_byte = ((plain_bit_len >> (56 - i * BITS_PER_U8)) & LOWER_BYTES_MASK as usize) as u8;
        process_plain.push(split_byte);
    }
    process_plain
}

fn into_vec_u8(hash: &[u32; 8]) -> Vec<u8> {
    let mut ret = [0; SHA256_LEN];
    for i in 0..hash.len() {
        ret[i * U8_PER_U32] = ((hash[i] >> 24) & LOWER_BYTES_MASK) as u8;
        ret[i * U8_PER_U32 + 1] = ((hash[i] >> 16) & LOWER_BYTES_MASK) as u8;
        ret[i * U8_PER_U32 + 2] = ((hash[i] >> 8) & LOWER_BYTES_MASK) as u8;
        ret[i * U8_PER_U32 + 3] = (hash[i] & LOWER_BYTES_MASK) as u8;
    }

    ret.to_vec()
}

/// the function to execute sha256
pub fn sha256(plain: &Vec<u8>) -> Vec<u8> {
    let processed_plain = pre_process_plain(plain);

    into_vec_u8(&compress(&processed_plain))
}
