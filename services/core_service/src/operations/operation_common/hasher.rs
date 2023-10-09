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

const SHA256_H: [u32; 8] = [
    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab,
    0x5be0cd19,
];

const SHA256_K: [u32; 64] = [
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4,
    0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe,
    0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f,
    0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
    0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
    0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116,
    0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7,
    0xc67178f2,
];

fn compress(input_bytes: &Vec<u8>) -> [u32; 8] {
    let mut hash = SHA256_H;
    let nblocks = input_bytes.len() / 64;
    for i in 0..nblocks {
        let mut w = [0; 64];
        for (j, item) in w.iter_mut().enumerate().take(16) {
            let offset = i * 64 + j * 4;
            *item = ((input_bytes[offset] as u32) << 24)
                | ((input_bytes[offset + 1] as u32) << 16)
                | ((input_bytes[offset + 2] as u32) << 8)
                | (input_bytes[offset + 3] as u32);
        }
        for j in 16..64 {
            let s0 = w[j - 15].rotate_right(7) ^ w[j - 15].rotate_right(18) ^ (w[j - 15] >> 3);
            let s1 = w[j - 2].rotate_right(17) ^ w[j - 2].rotate_right(19) ^ (w[j - 2] >> 10);
            w[j] = w[j - 16]
                .wrapping_add(s0)
                .wrapping_add(w[j - 7])
                .wrapping_add(s1);
        }
        let mut working: [u32; 8] = hash; // working variables
        for j in 0..64 {
            let s1 = working[4].rotate_right(6)
                ^ working[4].rotate_right(11)
                ^ working[4].rotate_right(25);
            let choose = (working[4] & working[5]) ^ ((!working[4]) & working[6]);
            let temp1 = working[7]
                .wrapping_add(s1)
                .wrapping_add(choose)
                .wrapping_add(SHA256_K[j])
                .wrapping_add(w[j]);
            let s0 = working[0].rotate_right(2)
                ^ working[0].rotate_right(13)
                ^ working[0].rotate_right(22);
            let major =
                (working[0] & working[1]) ^ (working[0] & working[2]) ^ (working[1] & working[2]);
            let temp2 = s0.wrapping_add(major);
            working[7] = working[6];
            working[6] = working[5];
            working[5] = working[4];
            working[4] = working[3].wrapping_add(temp1);
            working[3] = working[2];
            working[2] = working[1];
            working[1] = working[0];
            working[0] = temp1.wrapping_add(temp2);
        }

        for j in 0..8 {
            hash[j] = hash[j].wrapping_add(working[j]);
        }
    }
    hash
}

pub(crate) fn sha256(input: &[u8]) -> [u8; 32] {
    // padding
    let mut input_bytes = input.to_vec();
    let input_len = input_bytes.len();
    let padding_len = if input_len % 64 < 56 {
        56 - input_len % 64
    } else {
        120 - input_len % 64
    };

    input_bytes.push(0x80); // 1000 0000
    // for _ in 0..padding_len - 1 {
    //     input_bytes.push(0x00);
    // }
    input_bytes.append(&mut vec![0x00; padding_len - 1]);

    let input_bit_len = input_len * 8;
    for i in 0..8 {
        let byte = ((input_bit_len >> (56 - i * 8)) & 0xff) as u8;

        input_bytes.push(byte);
    }

    let hash = compress(&input_bytes);

    let mut ret = [0; 32];
    for i in 0..8 {
        ret[i * 4] = ((hash[i] >> 24) & 0xff) as u8;
        ret[i * 4 + 1] = ((hash[i] >> 16) & 0xff) as u8;
        ret[i * 4 + 2] = ((hash[i] >> 8) & 0xff) as u8;
        ret[i * 4 + 3] = (hash[i] & 0xff) as u8;
    }

    ret
}