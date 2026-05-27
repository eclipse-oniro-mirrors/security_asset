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

#include "crypto_wrapper.h"

#include <cstring>
#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <openssl/evp.h>

#include "saf_log.h"
#include "saf_result_code.h"

int32_t GenerateRandomBytes(Uint8Buff *buf)
{
    if (buf == nullptr || buf->buf == nullptr || buf->size == 0) {
        LOGE("[FATAL]GenerateRandomBytes invalid params.");
        return SAF_ERR_NULL_PTR;
    }
    int ret = RAND_bytes(buf->buf, buf->size);
    if (ret != 1) {
        LOGE("[FATAL]RAND_bytes failed, ret: %{public}d.", ret);
        return SAF_ERR_CRYPTO_OPERATION;
    }
    return SAF_SUCCESS;
}

int32_t ComputeHmacSha256(const Uint8Buff *key, const Uint8Buff *data, Uint8Buff *hmac)
{
    if (key == nullptr || key->buf == nullptr || data == nullptr || data->buf == nullptr ||
        hmac == nullptr || hmac->buf == nullptr) {
        LOGE("[FATAL]ComputeHmacSha256 invalid params.");
        return SAF_ERR_NULL_PTR;
    }
    if (hmac->size < HMAC_SHA256_SIZE) {
        LOGE("[FATAL]ComputeHmacSha256 hmac buffer too small.");
        return SAF_ERR_INVALID_HMAC_SIZE;
    }

    unsigned int len = HMAC_SHA256_SIZE;
    uint8_t *result = 
        HMAC(EVP_sha256(), key->buf, key->size, data->buf, static_cast<size_t>(data->size), hmac->buf, &len);
    if (result == nullptr) {
        LOGE("[FATAL]HMAC computation failed.");
        return SAF_ERR_CRYPTO_OPERATION;
    }
    return SAF_SUCCESS;
}

int32_t VerifyHmacSha256(const Uint8Buff *key, const Uint8Buff *data, const Uint8Buff *expectedHmac)
{
    if (expectedHmac == nullptr || expectedHmac->buf == nullptr ||
        expectedHmac->size != HMAC_SHA256_SIZE) {
        LOGE("[FATAL]VerifyHmacSha256 invalid params.");
        return SAF_ERR_NULL_PTR;
    }

    uint8_t computedHmac[HMAC_SHA256_SIZE];
    Uint8Buff computedBuf = { computedHmac, HMAC_SHA256_SIZE };
    int32_t ret = ComputeHmacSha256(key, data, &computedBuf);
    if (ret != SAF_SUCCESS) {
        return ret;
    }

    if (memcmp(computedHmac, expectedHmac->buf, HMAC_SHA256_SIZE) != 0) {
        LOGE("[FATAL]VerifyHmacSha256 hmac mismatch.");
        return SAF_EVALUATE_DENY;
    }
    return SAF_SUCCESS;
}

int32_t Base64Encode(const Uint8Buff *input, Uint8Buff *output)
{
    if (input == nullptr || input->buf == nullptr || output == nullptr || output->buf == nullptr) {
        LOGE("[FATAL]Base64Encode invalid params.");
        return SAF_ERR_NULL_PTR;
    }
    size_t expectedLen = 4 * ((input->size + 2) / 3) + 1;
    if (output->size < expectedLen) {
        LOGE("[FATAL]Base64Encode output buffer too small.");
        return SAF_ERR_BASE64_INVALID_LEN;
    }

    int len = EVP_EncodeBlock(output->buf, input->buf, input->size);
    if (len < 0) {
        LOGE("[FATAL]EVP_EncodeBlock failed.");
        return SAF_ERR_CRYPTO_OPERATION;
    }
    output->size = static_cast<uint32_t>(len);
    return SAF_SUCCESS;
}

int32_t Base64Decode(const Uint8Buff *input, Uint8Buff *output)
{
    if (input == nullptr || input->buf == nullptr || output == nullptr || output->buf == nullptr) {
        LOGE("[FATAL]Base64Decode invalid params.");
        return SAF_ERR_NULL_PTR;
    }
    if (input->size == 0) {
        LOGE("[FATAL]Base64Decode invalid input size.");
        return SAF_ERR_BASE64_INVALID_LEN;
    }
    size_t expectedLen = 3 * input->size / 4;
    if (output->size < expectedLen) {
        LOGE("[FATAL]Base64Decode output buffer too small.");
        return SAF_ERR_BASE64_INVALID_LEN;
    }

    int len = EVP_DecodeBlock(output->buf, input->buf, input->size);
    if (len < 0) {
        LOGE("[FATAL]EVP_DecodeBlock failed, len = %{public}d.", len);
        return SAF_ERR_CRYPTO_OPERATION;
    }

    size_t padding = 0;
    for (int32_t i = input->size - 1; i >= 0 && input->buf[i] == '='; --i) {
        padding++;
    }
    if (padding > len) {
        LOGE("[FATAL]padding is greater than len, len = %{public}d.", len);
        return SAF_ERR_CRYPTO_OPERATION;
    }
    len -= padding;

    output->size = static_cast<uint32_t>(len);
    return SAF_SUCCESS;
}