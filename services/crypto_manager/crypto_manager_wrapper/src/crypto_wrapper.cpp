/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include <securec.h>
#include "hks_api.h"
#include "hks_param.h"
#include "hks_type.h"
#include "asset_log.h"
#include "hks_key_wrapper.h"

static const uint32_t AEAD_SIZE = 16;
static const uint32_t NONCE_SIZE = 12;
static uint8_t NONCE[NONCE_SIZE] = { 0 }; // hard code

// encrypt params
static struct HksParam g_encryptParams[] = {
    {
        .tag = HKS_TAG_ALGORITHM,
        .uint32Param = HKS_ALG_AES
    }, {
        .tag = HKS_TAG_PURPOSE,
        .uint32Param = HKS_KEY_PURPOSE_ENCRYPT
    }, {
        .tag = HKS_TAG_KEY_SIZE,
        .uint32Param = HKS_AES_KEY_SIZE_256
    }, {
        .tag = HKS_TAG_PADDING,
        .uint32Param = HKS_PADDING_NONE
    }, {
        .tag = HKS_TAG_BLOCK_MODE,
        .uint32Param = HKS_MODE_GCM
    }, {
        .tag = HKS_TAG_ASSOCIATED_DATA,
        .blob = {
            .size = 0,
            .data = NULL
        }
    }, {
        .tag = HKS_TAG_NONCE,
        .blob = {
            .size = NONCE_SIZE,
            .data = (uint8_t *)NONCE //todo
        }
    }
};

// decrypt params
static struct HksParam g_decryptParams[] = {
    {
        .tag = HKS_TAG_ALGORITHM,
        .uint32Param = HKS_ALG_AES
    }, {
        .tag = HKS_TAG_PURPOSE,
        .uint32Param = HKS_KEY_PURPOSE_DECRYPT
    }, {
        .tag = HKS_TAG_KEY_SIZE,
        .uint32Param = HKS_AES_KEY_SIZE_256
    }, {
        .tag = HKS_TAG_PADDING,
        .uint32Param = HKS_PADDING_NONE
    }, {
        .tag = HKS_TAG_BLOCK_MODE,
        .uint32Param = HKS_MODE_GCM
    }, {
        .tag = HKS_TAG_ASSOCIATED_DATA,
        .blob = {
            .size = 0,
            .data = NULL
        }
    }, {
        .tag = HKS_TAG_NONCE,
        .blob = {
            .size = NONCE_SIZE,
            .data = (uint8_t *)NONCE
        }
    }, {
        .tag = HKS_TAG_AE_TAG,
        .blob = {
            .size = 0,
            .data = NULL // todo: need fix by code
        }
    }
};

int32_t UpdateLoopFinish(const struct HksBlob *handle, const struct HksParamSet *paramSet,
    const struct HksBlob *inData, struct HksBlob *outData)
{
    /* donnot need update, just finish */
    uint8_t *tempOutData = (uint8_t *)malloc(outData->size);
    if (tempOutData == NULL) {
        LOGE("malloc failed\n");
        return HKS_FAILURE;
    }

    struct HksBlob outDataFinish = { outData->size, tempOutData };

    if (HksFinish(handle, paramSet, inData, &outDataFinish) != HKS_SUCCESS) {
        LOGE("HksFinish Failed.");
        free(outDataFinish.data);
        return HKS_FAILURE;
    }

    (void)memcpy_s(outData->data, outData->size, outDataFinish.data, outDataFinish.size);
    free(outDataFinish.data);

    return HKS_SUCCESS;
}

int EncryptWrapper(uint32_t keyLen, const uint8_t *keyData, uint32_t aadLen, const uint8_t *aad,
    uint32_t msgLen, const uint8_t *msg, uint32_t cipherLen, uint8_t *cipher)
{
    if ((keyLen == 0 || keyData == NULL) || (msgLen == 0 || msg == NULL) || (aadLen == 0 || aad == NULL) ||
        (cipherLen != (msgLen + AEAD_SIZE + NONCE_SIZE) || cipher == NULL)) {
        LOGE("hks encrypt invalid argument\n");
        return HKS_FAILURE;
    }

    int32_t ret;
    struct HksBlob keyAlias = { keyLen, (uint8_t *)keyData };
    struct HksBlob inData = { msgLen, (uint8_t *)msg };
    struct HksBlob outData = { cipherLen, cipher };
    struct HksParamSet *encryptParamSet = NULL;
    uint8_t handleE[sizeof(uint64_t)] = { 0 };
    struct HksBlob handleEncrypt = { sizeof(uint64_t), handleE };

    /* initial AAD */
    g_encryptParams[5].blob.size = aadLen;
    g_encryptParams[5].blob.data = (uint8_t *)aad;

    // three stage encrypt
    ret = InitParamSet(&encryptParamSet, g_encryptParams, sizeof(g_encryptParams) / sizeof(HksParam));
    if (ret != HKS_SUCCESS) {
        LOGE("hks encrypt init paramset err = %d\n", ret);
        goto END;
    }

    // Init
    ret = HksInit(&keyAlias, encryptParamSet, &handleEncrypt, nullptr);
    if (ret != HKS_SUCCESS) {
        LOGE("hks encrypt init failed err = %d\n", ret);
        goto END;
    }
 
    // Update & Finish
    ret = UpdateLoopFinish(&handleEncrypt, encryptParamSet, &inData, &outData);
    if (ret != HKS_SUCCESS) {
        LOGE("hks encrypt updata and finish failed err = %d\n", ret);
    }

END:
    /* reset AAD */
    g_encryptParams[5].blob.size = 0;
    g_encryptParams[5].blob.data = NULL;
    return ret;
}

int DecryptWrapper(uint32_t keyLen, const uint8_t *keyData, uint32_t aadLen, const uint8_t *aad,
    uint32_t cipherLen, const uint8_t *cipher, uint32_t plainLen, uint8_t *plain)
{
    if ((keyLen == 0 || keyData == NULL) || (cipherLen <= AEAD_SIZE + NONCE_SIZE || cipher == NULL) ||
        (aadLen == 0 || aad == NULL) || (plainLen != cipherLen - AEAD_SIZE - NONCE_SIZE || plain == NULL)) {
        LOGE("hks decrypt invalid argument\n");
        return HKS_FAILURE;
    }

    int32_t ret;
    struct HksBlob keyAlias = { keyLen, (uint8_t *)keyData };
    struct HksBlob inData = { plainLen, (uint8_t *)cipher };
    struct HksBlob outData = { plainLen, plain };
    struct HksParamSet *decryptParamSet = nullptr;
    uint8_t handleD[sizeof(uint64_t)] = { 0 };
    struct HksBlob handleDecrypt = { sizeof(uint64_t), handleD };

    /* initial AAD */
    g_decryptParams[5].blob.size = aadLen;
    g_decryptParams[5].blob.data = (uint8_t *)aad;

    /* initial AEAD */
    g_decryptParams[7].blob.size = AEAD_SIZE;
    g_decryptParams[7].blob.data = (uint8_t *)cipher + plainLen;

    // three stage decrypt
    ret = InitParamSet(&decryptParamSet, g_decryptParams, sizeof(g_decryptParams) / sizeof(HksParam));
    if (ret != HKS_SUCCESS) {
        LOGE("hks decrypt init paramset err = %d\n", ret);
        goto END;
    }

    // Init
    ret = HksInit(&keyAlias, decryptParamSet, &handleDecrypt, nullptr);
    if (ret != HKS_SUCCESS) {
        LOGE("hks decrypt init failed err = %d\n", ret);
        goto END;
    }

    // Update & Finish
    ret = UpdateLoopFinish(&handleDecrypt, decryptParamSet, &inData, &outData);
    if (ret != HKS_SUCCESS) {
        LOGE("hks decrypt updata and finish failed err = %d\n", ret);
    }

END:
    /* reset AAD */
    g_encryptParams[5].blob.size = 0;
    g_encryptParams[5].blob.data = NULL;
    /* reset AEAD */
    g_decryptParams[7].blob.size = 0;
    g_decryptParams[7].blob.data = NULL;
    return ret;
}