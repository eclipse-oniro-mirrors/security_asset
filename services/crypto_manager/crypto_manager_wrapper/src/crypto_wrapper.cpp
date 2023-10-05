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

/* AEAD return from huks with 16 bits */
static const uint32_t AEAD_SIZE = 16;
/* NONCE remain for huks, hard code now */
static const uint32_t NONCE_SIZE = 12;
static uint8_t NONCE[NONCE_SIZE] = { 0 };

/* encrypt params */
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

/* decrypt params */
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
            .data = NULL
        }
    }
};

static int32_t CheckCryptParma(const struct CryptParam *data)
{
    if ((data == NULL) || (data->keyLen == 0 || data->keyData == NULL) ||
        (data->dataInLen == 0 || data->dataIn == NULL) ||
        (data->aadLen == 0 || data->aad == NULL) ||
        (data->dataOutLen ==0 || data->dataOut == NULL)) {
        LOGE("hks invalid argument\n");
        return HKS_FAILURE;
    }

    return HKS_SUCCESS;
}

static int32_t UpdateLoopFinish(const struct HksBlob *handle, const struct HksParamSet *paramSet,
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

int32_t EncryptWrapper(const struct CryptParam *data)
{
    if (CheckCryptParma(data) != HKS_SUCCESS) {
        LOGE("hks encrypt invalid argument\n");
        return HKS_FAILURE;
    }

    if (data->dataOutLen != data->dataInLen + AEAD_SIZE + NONCE_SIZE) {
        LOGE("hks encrypt cipher len not match\n");
        return HKS_FAILURE;
    }

    int32_t ret;
    struct HksBlob keyAlias = { data->keyLen, (uint8_t *)data->keyData };
    struct HksBlob inData = { data->dataInLen, (uint8_t *)data->dataIn };
    struct HksBlob outData = { data->dataOutLen, data->dataOut };
    struct HksParamSet *encryptParamSet = NULL;
    uint8_t handleE[sizeof(uint64_t)] = { 0 };
    struct HksBlob handleEncrypt = { sizeof(uint64_t), handleE };

    /* initial AAD */
    g_encryptParams[5].blob.size = data->aadLen;
    g_encryptParams[5].blob.data = (uint8_t *)data->aad;

    /* three stage encrypt */
    ret = InitParamSet(&encryptParamSet, g_encryptParams, sizeof(g_encryptParams) / sizeof(HksParam));
    if (ret != HKS_SUCCESS) {
        LOGE("hks encrypt init paramset err = %d\n", ret);
        goto END;
    }

    /* Init */
    ret = HksInit(&keyAlias, encryptParamSet, &handleEncrypt, nullptr);
    if (ret != HKS_SUCCESS) {
        LOGE("hks encrypt init failed err = %d\n", ret);
        goto END;
    }
 
    /* Update & Finish */
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

int32_t DecryptWrapper(const struct CryptParam *data)
{
    if (CheckCryptParma(data) != HKS_SUCCESS) {
        LOGE("hks decrypt invalid argument\n");
        return HKS_FAILURE;
    }

    if ((data->dataInLen <= AEAD_SIZE + NONCE_SIZE) ||
        (data->dataOutLen != data->dataInLen - AEAD_SIZE - NONCE_SIZE)) {
        LOGE("hks decrypt cipher len or plain len not match\n");
        return HKS_FAILURE;
    }

    int32_t ret;
    struct HksBlob keyAlias = { data->keyLen, (uint8_t *)data->keyData };
    struct HksBlob inData = { data->dataOutLen, (uint8_t *)data->dataIn };
    struct HksBlob outData = { data->dataOutLen, data->dataOut };
    struct HksParamSet *decryptParamSet = nullptr;
    uint8_t handleD[sizeof(uint64_t)] = { 0 };
    struct HksBlob handleDecrypt = { sizeof(uint64_t), handleD };

    /* initial AAD */
    g_decryptParams[5].blob.size = data->aadLen;
    g_decryptParams[5].blob.data = (uint8_t *)data->aad;

    /* initial AEAD */
    g_decryptParams[7].blob.size = AEAD_SIZE;
    g_decryptParams[7].blob.data = (uint8_t *)data->dataIn + data->dataOutLen;

    /* three stage decrypt */
    ret = InitParamSet(&decryptParamSet, g_decryptParams, sizeof(g_decryptParams) / sizeof(HksParam));
    if (ret != HKS_SUCCESS) {
        LOGE("hks decrypt init paramset err = %d\n", ret);
        goto END;
    }

    /* Init */
    ret = HksInit(&keyAlias, decryptParamSet, &handleDecrypt, nullptr);
    if (ret != HKS_SUCCESS) {
        LOGE("hks decrypt init failed err = %d\n", ret);
        goto END;
    }

    /* Update & Finish */
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