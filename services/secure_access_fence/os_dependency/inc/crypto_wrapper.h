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

#ifndef CRYPTO_WRAPPER
#define CRYPTO_WRAPPER

#include <stdint.h>
#include "saf_result_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CHALLENGE_SIZE 32
#define HMAC_SHA256_SIZE 32

typedef struct {
    uint8_t *buf;
    uint32_t size;
} Uint8Buff;

int32_t GenerateRandomBytes(Uint8Buff *buf);

int32_t ComputeHmacSha256(const Uint8Buff *key, const Uint8Buff *data, Uint8Buff *hmac);

int32_t VerifyHmacSha256(const Uint8Buff *key, const Uint8Buff *data, const Uint8Buff *expectedHmac);

int32_t Base64Encode(const Uint8Buff *input, Uint8Buff *output);

int32_t Base64Decode(const Uint8Buff *input, Uint8Buff *output);

#ifdef __cplusplus
}
#endif

#endif