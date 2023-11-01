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

#include "hks_api.h"
#include "hks_param.h"
#include "hks_type.h"

#include "asset_log.h"
#include "hks_key_wrapper.h"

/* AEAD return from huks with 16 bits */
static const uint32_t AEAD_SIZE = 16;
/* NONCE return from huks with 12 bits */
static const uint32_t NONCE_SIZE = 12;

static int32_t CheckCryptParam(const struct HksBlob *keyData, const struct HksBlob *aadData,
    const struct HksBlob *inData, struct HksBlob *outData)
{
    if ((keyData == nullptr) || (keyData->size == 0) || (keyData->data == nullptr) ||
        (aadData == nullptr) || (aadData->size == 0) || (aadData->data == nullptr) ||
        (inData == nullptr) || (inData->size == 0) || (inData->data == nullptr) ||
        (outData == nullptr) || (outData->size == 0) || (outData->data == nullptr)) {
        LOGE("hks invalid argument\n");
        return HKS_ERROR_INVALID_ARGUMENT;
    }

    return HKS_SUCCESS;
}

static int32_t InitEncryptParamSet(struct HksParamSet **paramSet, const struct HksBlob *aadData)
{
    /* encrypt param */
    struct HksParam encryptParams[] = {
        { .tag = HKS_TAG_ALGORITHM, .uint32Param = HKS_ALG_AES },
        { .tag = HKS_TAG_PURPOSE, .uint32Param = HKS_KEY_PURPOSE_ENCRYPT },
        { .tag = HKS_TAG_KEY_SIZE, .uint32Param = HKS_AES_KEY_SIZE_256 },
        { .tag = HKS_TAG_PADDING, .uint32Param = HKS_PADDING_NONE },
        { .tag = HKS_TAG_BLOCK_MODE, .uint32Param = HKS_MODE_GCM },
        { .tag = HKS_TAG_ASSOCIATED_DATA, .blob = { .size = aadData->size, .data = (uint8_t *)aadData->data }} /* initial AAD */
    };

    return InitParamSet(paramSet, encryptParams, sizeof(encryptParams) / sizeof(HksParam));
}

int32_t EncryptWrapper(const struct HksBlob *keyData, const struct HksBlob *aadData,
    const struct HksBlob *inData, struct HksBlob *outData)
{
    if (CheckCryptParam(keyData, aadData, inData, outData) != HKS_SUCCESS) {
        LOGE("hks encrypt invalid argument\n");
        return HKS_ERROR_INVALID_ARGUMENT;
    }

    if (outData->size != inData->size + AEAD_SIZE + NONCE_SIZE) {
        LOGE("hks encrypt cipher len not match\n");
        return HKS_ERROR_INVALID_ARGUMENT;
    }

    int32_t ret;
    struct HksParamSet *encryptParamSet = nullptr;
    uint8_t handleE[sizeof(uint64_t)] = { 0 };
    struct HksBlob handleEncrypt = { sizeof(uint64_t), handleE };

    /* init paramset */
    ret = InitEncryptParamSet(&encryptParamSet, aadData);
    if (ret != HKS_SUCCESS) {
        LOGE("hks encrypt init paramset err = %{public}d\n", ret);
        return ret;
    }

    /* three stage encrypt Init */
    ret = HksInit(keyData, encryptParamSet, &handleEncrypt, nullptr);
    if (ret != HKS_SUCCESS) {
        LOGE("hks encrypt init failed err = %{public}d\n", ret);
        HksFreeParamSet(&encryptParamSet);
        return ret;
    }

    /* Do finish */
    ret = HksFinish(&handleEncrypt, encryptParamSet, inData, outData);
    if (ret != HKS_SUCCESS) {
        LOGE("hks encrypt updata and finish failed err = %{public}d\n", ret);
    }

    HksFreeParamSet(&encryptParamSet);
    return ret;
}

static int32_t InitDecryptParamSet(struct HksParamSet **paramSet, const struct HksBlob *aadData,
    const struct HksBlob *inData, struct HksBlob *outData)
{
    /* decrypt params */
    struct HksParam decryptParams[] = {
        { .tag = HKS_TAG_ALGORITHM, .uint32Param = HKS_ALG_AES },
        { .tag = HKS_TAG_PURPOSE, .uint32Param = HKS_KEY_PURPOSE_DECRYPT },
        { .tag = HKS_TAG_KEY_SIZE, .uint32Param = HKS_AES_KEY_SIZE_256 },
        { .tag = HKS_TAG_PADDING, .uint32Param = HKS_PADDING_NONE },
        { .tag = HKS_TAG_BLOCK_MODE, .uint32Param = HKS_MODE_GCM },
        { .tag = HKS_TAG_ASSOCIATED_DATA, .blob = { .size = aadData->size, .data = (uint8_t *)aadData->data }},
        { .tag = HKS_TAG_NONCE, .blob = { .size = NONCE_SIZE, .data = (uint8_t *)inData->data + outData->size + AEAD_SIZE }},
        { .tag = HKS_TAG_AE_TAG, .blob = { .size = AEAD_SIZE, .data = (uint8_t *)inData->data + outData->size }}
    };

    return InitParamSet(paramSet, decryptParams, sizeof(decryptParams) / sizeof(HksParam));
}

int32_t DecryptWrapper(const struct HksBlob *keyData, const struct HksBlob *aadData,
    const struct HksBlob *inData, struct HksBlob *outData)
{
    if (CheckCryptParam(keyData, aadData, inData, outData) != HKS_SUCCESS) {
        LOGE("hks decrypt invalid argument\n");
        return HKS_ERROR_INVALID_ARGUMENT;
    }

    if ((inData->size <= AEAD_SIZE + NONCE_SIZE) ||
        (outData->size != inData->size - AEAD_SIZE - NONCE_SIZE)) {
        LOGE("hks decrypt cipher len or plain len not match\n");
        return HKS_ERROR_INVALID_ARGUMENT;
    }

    /* data in len is out len, except aead size */
    struct HksBlob tmpInData = { outData->size, inData->data };

    int32_t ret;
    struct HksParamSet *decryptParamSet = nullptr;
    uint8_t handleD[sizeof(uint64_t)] = { 0 };
    struct HksBlob handleDecrypt = { sizeof(uint64_t), handleD };

    /* init paramset */
    ret = InitDecryptParamSet(&decryptParamSet, aadData, inData, outData);
    if (ret != HKS_SUCCESS) {
        LOGE("hks decrypt init paramset err = %{public}d\n", ret);
        return ret;
    }

    /* three stage decrypt Init */
    ret = HksInit(keyData, decryptParamSet, &handleDecrypt, nullptr);
    if (ret != HKS_SUCCESS) {
        LOGE("hks decrypt init failed err = %{public}d\n", ret);
        HksFreeParamSet(&decryptParamSet);
        return ret;
    }

    /* Do finish */
    ret = HksFinish(&handleDecrypt, decryptParamSet, &tmpInData, outData);
    if (ret != HKS_SUCCESS) {
        LOGE("hks decrypt updata and finish failed err = %{public}d\n", ret);
    }

    HksFreeParamSet(&decryptParamSet);
    return ret;
}

#define CHALLENGEPOS_MAX 3
static int32_t CheckInitParma(const struct CryptParam *param, const struct HksBlob *keyData,
    struct HksBlob *challengeData, struct HksBlob *handleData)
{
    if (param == nullptr || param->challengePos > CHALLENGEPOS_MAX ||
        ((param->cryptoMode != HKS_KEY_PURPOSE_ENCRYPT) && (param->cryptoMode != HKS_KEY_PURPOSE_DECRYPT ))) {
        LOGE("hks invalid param\n");
        return HKS_ERROR_INVALID_ARGUMENT;
    }

    if (keyData == nullptr || keyData->size == 0 || keyData->data == nullptr) {
        LOGE("hks invalid key data\n");
        return HKS_ERROR_INVALID_ARGUMENT;
    }

    if (challengeData == nullptr || challengeData->size == 0 || challengeData->data == nullptr) {
        LOGE("hks invalid handle data\n");
        return HKS_ERROR_INVALID_ARGUMENT;
    }

    if (handleData == nullptr || handleData->size == 0 || handleData->data == nullptr) {
        LOGE("hks invalid handle data\n");
        return HKS_ERROR_INVALID_ARGUMENT;
    }

    return HKS_SUCCESS;
}

static int32_t CreateInitParamSet(struct HksParamSet **paramSet, const struct CryptParam *param)
{
    /* init params */
    struct HksParam initParams[] = {
        { .tag = HKS_TAG_ALGORITHM, .uint32Param = HKS_ALG_AES},
        { .tag = HKS_TAG_PURPOSE, .uint32Param = param->cryptoMode },
        { .tag = HKS_TAG_KEY_SIZE, .uint32Param = HKS_AES_KEY_SIZE_256 },
        { .tag = HKS_TAG_PADDING, .uint32Param = HKS_PADDING_NONE },
        { .tag = HKS_TAG_BLOCK_MODE, .uint32Param = HKS_MODE_GCM },
        { .tag = HKS_TAG_DIGEST, .uint32Param = HKS_DIGEST_NONE },
        { .tag = HKS_TAG_CHALLENGE_POS, .uint32Param = param->challengePos },
        { .tag = HKS_TAG_IS_BATCH_OPERATION, .boolParam = true },
        { .tag = HKS_TAG_BATCH_OPERATION_TIMEOUT, .uint32Param = param->expTime },
        { .tag = HKS_TAG_BATCH_PURPOSE, .uint32Param = param->cryptoMode },
    };

    return InitParamSet(paramSet, initParams, sizeof(initParams) / sizeof(HksParam));
}

int32_t InitCryptoWrapper(const struct CryptParam *param, const struct HksBlob *keyData,
    struct HksBlob *challengeData, struct HksBlob *handleData)
{
    if (CheckInitParma(param, keyData, challengeData, handleData) != HKS_SUCCESS) {
        LOGE("hks init crypto invalid argument\n");
        return HKS_ERROR_INVALID_ARGUMENT;
    }

    int32_t ret;
    struct HksParamSet *paramSet = nullptr;

    /* init paramset */
    ret = CreateInitParamSet(&paramSet, param);
    if (ret != HKS_SUCCESS) {
        LOGE("hks crypto init paramset err = %{public}d\n", ret);
        return ret;
    }

    /* Init */
    ret = HksInit(keyData, paramSet, handleData, challengeData);
    if (ret != HKS_SUCCESS) {
        LOGE("hks crypto init failed err = %{public}d\n", ret);
    }

    HksFreeParamSet(&paramSet);
    LOGE("hks crypto init done ret = %{public}d\n", ret);
    return ret;
}

static int32_t CheckCryptoParam(const CryptParam *param, const struct HksBlob *aadData,
    const struct HksBlob *authToken, const struct HksBlob *handleData,
    const struct HksBlob *inData, struct HksBlob *outData)
{
    if (param == nullptr || param->challengePos > CHALLENGEPOS_MAX || param->cryptoMode != HKS_KEY_PURPOSE_DECRYPT) {
        LOGE("hks invalid param\n");
        return HKS_ERROR_INVALID_ARGUMENT;
    }

    if (authToken == nullptr || authToken->size == 0 || authToken->data == nullptr) {
        LOGE("hks invalid auth token\n");
        return HKS_ERROR_INVALID_ARGUMENT;
    }

    if (aadData == nullptr || aadData->size == 0 || aadData->data == nullptr) {
        LOGE("hks invalid aad data\n");
        return HKS_ERROR_INVALID_ARGUMENT;
    }

    if (handleData == nullptr || handleData->size == 0 || handleData->data == nullptr) {
        LOGE("hks invalid handle data\n");
        return HKS_ERROR_INVALID_ARGUMENT;
    }

    if (inData == nullptr || inData->size == 0 || inData->data == nullptr) {
        LOGE("hks invalid in data\n");
        return HKS_ERROR_INVALID_ARGUMENT;
    }

    if (outData == nullptr || outData->size == 0 || outData->data == nullptr) {
        LOGE("hks invalid out data\n");
        return HKS_ERROR_INVALID_ARGUMENT;
    }

    return HKS_SUCCESS;
}

static int32_t CreateCryptoParamSet(struct HksParamSet **paramSet, const CryptParam *param,
    const struct HksBlob *aadData, const struct HksBlob *authToken,
    const struct HksBlob *inData, struct HksBlob *outData)
{
    /* init decrypto params */
    struct HksParam initParams[] = {
        { .tag = HKS_TAG_ALGORITHM, .uint32Param = HKS_ALG_AES},
        { .tag = HKS_TAG_PURPOSE, .uint32Param = param->cryptoMode },
        { .tag = HKS_TAG_KEY_SIZE, .uint32Param = HKS_AES_KEY_SIZE_256 },
        { .tag = HKS_TAG_DIGEST, .uint32Param = HKS_DIGEST_NONE },
        { .tag = HKS_TAG_PADDING, .uint32Param = HKS_PADDING_NONE },
        { .tag = HKS_TAG_BLOCK_MODE, .uint32Param = HKS_MODE_GCM },
        { .tag = HKS_TAG_IS_BATCH_OPERATION, .boolParam = true },
        { .tag = HKS_TAG_BATCH_PURPOSE, .uint32Param = param->cryptoMode },
        { .tag = HKS_TAG_ASSOCIATED_DATA, .blob = { .size = aadData->size, .data = (uint8_t *)aadData->data }}, /* initial AAD */
        { .tag = HKS_TAG_NONCE, .blob = { .size = NONCE_SIZE, .data = (uint8_t *)inData->data + outData->size + AEAD_SIZE }},
        { .tag = HKS_TAG_AE_TAG, .blob = { .size = AEAD_SIZE, .data = (uint8_t *)inData->data + outData->size }},
        { .tag = HKS_TAG_AUTH_TOKEN, .blob = { .size = authToken->size, .data = (uint8_t *)authToken->data }}, /* initial auth token */
    };

    return InitParamSet(paramSet, initParams, sizeof(initParams) / sizeof(HksParam));
}

int32_t ExecCryptoWrapper(const CryptParam *param, const struct HksBlob *aadData, const struct HksBlob *authToken,
    const struct HksBlob *handleData, const struct HksBlob *inData, struct HksBlob *outData)
{
    if (CheckCryptoParam(param, aadData, authToken, handleData, inData, outData) != HKS_SUCCESS) {
        LOGE("hks update crypto invalid argument\n");
        return HKS_ERROR_INVALID_ARGUMENT;
    }

    int32_t ret;
    struct HksParamSet *paramSet = nullptr;
    struct HksBlob tmpInData = { 0, nullptr };

    tmpInData = { outData->size, (uint8_t *)inData->data };

    /* init paramset */
    ret = CreateCryptoParamSet(&paramSet, param, aadData, authToken, inData, outData);
    if (ret != HKS_SUCCESS) {
        LOGE("hks crypto update paramset err = %{public}d\n", ret);
        return ret;
    }

    /* update */
    ret = HksUpdate(handleData, paramSet, &tmpInData, outData);
    if (ret != HKS_SUCCESS) {
        LOGE("hks crypto update failed err = %{public}d\n", ret);
    }

    HksFreeParamSet(&paramSet);
    LOGE("hks crypto update done ret = %{public}d\n", ret);
    return ret;
}

static bool CheckDropParam(const CryptParam *param, struct HksBlob *handleData)
{
    if (param == nullptr || param->challengePos > CHALLENGEPOS_MAX || param->cryptoMode != HKS_KEY_PURPOSE_DECRYPT) {
        LOGE("hks invalid param\n");
        return HKS_ERROR_INVALID_ARGUMENT;
    }

    if (handleData == nullptr || handleData->size == 0 || handleData->data == nullptr) {
        LOGE("hks invalid handle data\n");
        return HKS_ERROR_INVALID_ARGUMENT;
    }

    return HKS_SUCCESS;
}

int32_t DropCrypto(const CryptParam *param, struct HksBlob *handleData)
{
    if (CheckDropParam(param, handleData) != HKS_SUCCESS) {
        LOGE("hks finish crypto invalid argument\n");
        return HKS_ERROR_INVALID_ARGUMENT;
    }

    int32_t ret;
    struct HksParamSet *paramSet = nullptr;
    struct HksBlob inData = { 0, nullptr }; // HksFinish in drop, donnot need input
    struct HksBlob outData = { 0, nullptr };

    /* init paramset */
    ret = InitParamSet(&paramSet, nullptr, 0);
    if (ret != HKS_SUCCESS) {
        LOGE("hks crypto finish paramset err = %{public}d\n", ret);
        return ret;
    }

    /* finish */
    ret = HksFinish(handleData, paramSet, &inData, &outData);
    if (ret != HKS_SUCCESS) {
        LOGE("hks crypto finish failed err = %{public}d\n", ret);
    }

    HksFreeParamSet(&paramSet);
    LOGE("hks crypto finish done ret = %{public}d\n", ret);
    return ret;
}