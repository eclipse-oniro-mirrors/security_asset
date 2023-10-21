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

static int32_t CheckCryptParam(const struct CryptParam *data)
{
    if ((data == nullptr) || (data->keyLen == 0 || data->keyData == nullptr) ||
        (data->dataInLen == 0 || data->dataIn == nullptr) ||
        (data->aadLen == 0 || data->aad == nullptr) ||
        (data->dataOutLen ==0 || data->dataOut == nullptr)) {
        LOGE("hks invalid argument\n");
        return HKS_ERROR_INVALID_ARGUMENT;
    }

    return HKS_SUCCESS;
}

static int32_t InitEncryptParamSet(struct HksParamSet **paramSet, const struct CryptParam *data)
{
    /* encrypt param */
    struct HksParam encryptParams[] = {
        { .tag = HKS_TAG_ALGORITHM, .uint32Param = HKS_ALG_AES },
        { .tag = HKS_TAG_PURPOSE, .uint32Param = HKS_KEY_PURPOSE_ENCRYPT },
        { .tag = HKS_TAG_KEY_SIZE, .uint32Param = HKS_AES_KEY_SIZE_256 },
        { .tag = HKS_TAG_PADDING, .uint32Param = HKS_PADDING_NONE },
        { .tag = HKS_TAG_BLOCK_MODE, .uint32Param = HKS_MODE_GCM },
        { .tag = HKS_TAG_ASSOCIATED_DATA, .blob = { .size = data->aadLen, .data = (uint8_t *)data->aad }}, /* initial AAD */
        { .tag = HKS_TAG_NONCE, .blob = { .size = NONCE_SIZE, .data = (uint8_t *)NONCE }} /* todo */
    };

    return InitParamSet(paramSet, encryptParams, sizeof(encryptParams) / sizeof(HksParam));
}

int32_t EncryptWrapper(const struct CryptParam *data)
{
    if (CheckCryptParam(data) != HKS_SUCCESS) {
        LOGE("hks encrypt invalid argument\n");
        return HKS_ERROR_INVALID_ARGUMENT;
    }

    if (data->dataOutLen != data->dataInLen + AEAD_SIZE) { // todo : zdy 加上nonce的长度
        LOGE("hks encrypt cipher len not match\n");
        return HKS_ERROR_INVALID_ARGUMENT;
    }

    int32_t ret;
    struct HksBlob keyAlias = { data->keyLen, (uint8_t *)data->keyData };
    struct HksBlob inData = { data->dataInLen, (uint8_t *)data->dataIn };
    struct HksBlob outData = { data->dataOutLen, data->dataOut };
    struct HksParamSet *encryptParamSet = nullptr;
    uint8_t handleE[sizeof(uint64_t)] = { 0 };
    struct HksBlob handleEncrypt = { sizeof(uint64_t), handleE };

    /* init paramset */
    ret = InitEncryptParamSet(&encryptParamSet, data);
    if (ret != HKS_SUCCESS) {
        LOGE("hks encrypt init paramset err = %d\n", ret);
        return ret;
    }

    /* three stage encrypt Init */
    ret = HksInit(&keyAlias, encryptParamSet, &handleEncrypt, nullptr);
    if (ret != HKS_SUCCESS) {
        LOGE("hks encrypt init failed err = %d\n", ret);
        HksFreeParamSet(&encryptParamSet);
        return ret;
    }

    /* Do finish */
    ret = HksFinish(&handleEncrypt, encryptParamSet, &inData, &outData);
    if (ret != HKS_SUCCESS) {
        LOGE("hks encrypt updata and finish failed err = %d\n", ret);
    }

    HksFreeParamSet(&encryptParamSet);
    return ret;
}

static int32_t InitDecryptParamSet(struct HksParamSet **paramSet, const struct CryptParam *data)
{
    /* decrypt params */
    static struct HksParam decryptParams[] = {
        { .tag = HKS_TAG_ALGORITHM, .uint32Param = HKS_ALG_AES },
        { .tag = HKS_TAG_PURPOSE, .uint32Param = HKS_KEY_PURPOSE_DECRYPT },
        { .tag = HKS_TAG_KEY_SIZE, .uint32Param = HKS_AES_KEY_SIZE_256 },
        { .tag = HKS_TAG_PADDING, .uint32Param = HKS_PADDING_NONE },
        { .tag = HKS_TAG_BLOCK_MODE, .uint32Param = HKS_MODE_GCM },
        { .tag = HKS_TAG_ASSOCIATED_DATA, .blob = { .size = data->aadLen, .data = (uint8_t *)data->aad }},
        { .tag = HKS_TAG_NONCE, .blob = { .size = NONCE_SIZE, .data = (uint8_t *)NONCE }},
        { .tag = HKS_TAG_AE_TAG, .blob = { .size = AEAD_SIZE, .data = (uint8_t *)data->dataIn + data->dataOutLen }}
    };

    return InitParamSet(paramSet, decryptParams, sizeof(decryptParams) / sizeof(HksParam));
}

// todo: zdy : 封装rust的blob
int32_t DecryptWrapper(const struct CryptParam *data)
{
    if (CheckCryptParam(data) != HKS_SUCCESS) {
        LOGE("hks decrypt invalid argument\n");
        return HKS_ERROR_INVALID_ARGUMENT;
    }

    if ((data->dataInLen <= AEAD_SIZE) || // todo : zdy 加上nonce的长度
        (data->dataOutLen != data->dataInLen - AEAD_SIZE)) { // todo : zdy 加上nonce的长度
        LOGE("hks decrypt cipher len or plain len not match\n");
        return HKS_ERROR_INVALID_ARGUMENT;
    }

    int32_t ret;
    struct HksBlob keyAlias = { data->keyLen, (uint8_t *)data->keyData };
    struct HksBlob inData = { data->dataOutLen, (uint8_t *)data->dataIn };
    struct HksBlob outData = { data->dataOutLen, data->dataOut };
    struct HksParamSet *decryptParamSet = nullptr;
    uint8_t handleD[sizeof(uint64_t)] = { 0 };
    struct HksBlob handleDecrypt = { sizeof(uint64_t), handleD };

    /* init paramset */
    ret = InitDecryptParamSet(&decryptParamSet, data);
    if (ret != HKS_SUCCESS) {
        LOGE("hks decrypt init paramset err = %d\n", ret);
        return ret;
    }

    /* three stage decrypt Init */
    ret = HksInit(&keyAlias, decryptParamSet, &handleDecrypt, nullptr);
    if (ret != HKS_SUCCESS) {
        LOGE("hks decrypt init failed err = %d\n", ret);
        HksFreeParamSet(&decryptParamSet);
        return ret;
    }

    /* Do finish */
    ret = HksFinish(&handleDecrypt, decryptParamSet, &inData, &outData);
    if (ret != HKS_SUCCESS) {
        LOGE("hks decrypt updata and finish failed err = %d\n", ret);
    }

    HksFreeParamSet(&decryptParamSet);
    return ret;
}

#define CHALLENGEPOS_MAX 3
static int32_t CheckInitParma(const struct CryptParam *data)
{
    if ((data->keyLen == 0 || data->keyData == nullptr) || (data->challengeLen == 0 || data->challengeData == nullptr) ||
        (data->handleLen == 0 || data->handleData == nullptr)) {
        LOGE("invalid params\n");
        return HKS_ERROR_INVALID_ARGUMENT;
    }

    if ((data->cryptoMode != HKS_KEY_PURPOSE_ENCRYPT) && (data->cryptoMode != HKS_KEY_PURPOSE_DECRYPT)) {
        LOGE("invalid crypto mode = %d\n", data->cryptoMode);
        return HKS_ERROR_INVALID_ARGUMENT;
    }

    if (data->challengePos > CHALLENGEPOS_MAX) {
        LOGE("invalid challenge position = %d\n", data->challengePos);
        return HKS_ERROR_INVALID_ARGUMENT;
    }

    return HKS_SUCCESS;
}

static int32_t CreateInitParamSet(struct HksParamSet **paramSet, enum HksKeyPurpose cryptoMode, uint32_t challengePos)
{
    /* init params */
    static struct HksParam initParams[] = {
        { .tag = HKS_TAG_ALGORITHM, .uint32Param = HKS_ALG_AES},
        { .tag = HKS_TAG_PURPOSE, .uint32Param = cryptoMode },
        { .tag = HKS_TAG_KEY_SIZE, .uint32Param = HKS_AES_KEY_SIZE_256 },
        { .tag = HKS_TAG_PADDING, .uint32Param = HKS_PADDING_NONE },
        { .tag = HKS_TAG_BLOCK_MODE, .uint32Param = HKS_MODE_GCM },
        { .tag = HKS_TAG_CHALLENGE_POS, .uint32Param = challengePos }
    };

    return InitParamSet(paramSet, initParams, sizeof(initParams) / sizeof(HksParam));
}

int32_t InitCryptoWrapper(const struct CryptParam *data)
{
    if (CheckInitParma(data) != HKS_SUCCESS) {
        LOGE("hks init crypto invalid argument\n");
        return HKS_ERROR_INVALID_ARGUMENT;
    }

    int32_t ret;
    struct HksBlob keyAlias = { data->keyLen, (uint8_t *)(data->keyData) };
    struct HksParamSet *paramSet = nullptr;
    struct HksBlob handleInit = { data->handleLen, data->handleData };
    struct HksBlob challengeInit = { data->challengeLen, data->challengeData };

    /* init paramset */
    ret = CreateInitParamSet(&paramSet, data->cryptoMode, data->challengePos);
    if (ret != HKS_SUCCESS) {
        LOGE("hks crypto init paramset err = %d\n", ret);
        return ret;
    }

    /* Init */
    ret = HksInit(&keyAlias, paramSet, &handleInit, &challengeInit);
    if (ret != HKS_SUCCESS) {
        LOGE("hks crypto init failed err = %d\n", ret);
    }

    HksFreeParamSet(&paramSet);
    return ret;
}

static int32_t CheckCryptoParam(const struct CryptParam *data, bool needData)
{
    if (data == nullptr || (data->handleLen == 0 || data->handleData == nullptr)) {
        LOGE("hks invalid argument\n");
        return HKS_ERROR_INVALID_ARGUMENT;
    }

    if (needData && ((data->dataInLen == 0 || data->dataIn == nullptr) ||
        (data->aadLen == 0 || data->aad == nullptr) ||
        (data->dataOutLen ==0 || data->dataOut == nullptr))) {
        LOGE("hks invalid input data\n");
        return HKS_ERROR_INVALID_ARGUMENT;
    }

    if ((data->cryptoMode != HKS_KEY_PURPOSE_ENCRYPT) && (data->cryptoMode != HKS_KEY_PURPOSE_DECRYPT)) {
        LOGE("invalid crypto mode = %d\n", data->cryptoMode);
        return HKS_ERROR_INVALID_ARGUMENT;
    }

    if (data->challengePos > CHALLENGEPOS_MAX) {
        LOGE("invalid challenge position = %d\n", data->challengePos);
        return HKS_ERROR_INVALID_ARGUMENT;
    }

    return HKS_SUCCESS;
}

static int32_t CreateCryptoParamSet(struct HksParamSet **paramSet, const struct CryptParam *data)
{
    if (data->cryptoMode == HKS_KEY_PURPOSE_ENCRYPT) {
        /* init encrypto params */
        struct HksParam initParams[] = {
            { .tag = HKS_TAG_ALGORITHM, .uint32Param = HKS_ALG_AES},
            { .tag = HKS_TAG_PURPOSE, .uint32Param = data->cryptoMode },
            { .tag = HKS_TAG_KEY_SIZE, .uint32Param = HKS_AES_KEY_SIZE_256 },
            { .tag = HKS_TAG_PADDING, .uint32Param = HKS_PADDING_NONE },
            { .tag = HKS_TAG_BLOCK_MODE, .uint32Param = HKS_MODE_GCM },
            { .tag = HKS_TAG_CHALLENGE_POS, .uint32Param = data->challengePos },
            { .tag = HKS_TAG_ASSOCIATED_DATA, .blob = { .size = data->aadLen, .data = (uint8_t *)data->aad }}, /* initial AAD */
            { .tag = HKS_TAG_NONCE, .blob = { .size = NONCE_SIZE, .data = (uint8_t *)NONCE }}, /* todo */
        };

        return InitParamSet(paramSet, initParams, sizeof(initParams) / sizeof(HksParam));
    } else if (data->cryptoMode == HKS_KEY_PURPOSE_DECRYPT) {
        /* init decrypto params */
        struct HksParam initParams[] = {
            { .tag = HKS_TAG_ALGORITHM, .uint32Param = HKS_ALG_AES},
            { .tag = HKS_TAG_PURPOSE, .uint32Param = data->cryptoMode },
            { .tag = HKS_TAG_KEY_SIZE, .uint32Param = HKS_AES_KEY_SIZE_256 },
            { .tag = HKS_TAG_PADDING, .uint32Param = HKS_PADDING_NONE },
            { .tag = HKS_TAG_BLOCK_MODE, .uint32Param = HKS_MODE_GCM },
            { .tag = HKS_TAG_CHALLENGE_POS, .uint32Param = data->challengePos },
            { .tag = HKS_TAG_ASSOCIATED_DATA, .blob = { .size = data->aadLen, .data = (uint8_t *)data->aad }}, /* initial AAD */
            { .tag = HKS_TAG_NONCE, .blob = { .size = NONCE_SIZE, .data = (uint8_t *)NONCE }}, /* todo */
            { .tag = HKS_TAG_AE_TAG, .blob = { .size = AEAD_SIZE, .data = (uint8_t *)data->dataIn + data->dataOutLen }}
        };

        return InitParamSet(paramSet, initParams, sizeof(initParams) / sizeof(HksParam));
    }

    LOGE("invalid crypto mode\n");
    return HKS_ERROR_INVALID_ARGUMENT;
}

static int32_t CreateFinishParamSet(struct HksParamSet **paramSet, const struct CryptParam *data)
{
    /* init encrypto params */
    struct HksParam initParams[] = {
        { .tag = HKS_TAG_ALGORITHM, .uint32Param = HKS_ALG_AES},
        { .tag = HKS_TAG_PURPOSE, .uint32Param = data->cryptoMode },
        { .tag = HKS_TAG_KEY_SIZE, .uint32Param = HKS_AES_KEY_SIZE_256 },
        { .tag = HKS_TAG_PADDING, .uint32Param = HKS_PADDING_NONE },
        { .tag = HKS_TAG_BLOCK_MODE, .uint32Param = HKS_MODE_GCM },
        { .tag = HKS_TAG_CHALLENGE_POS, .uint32Param = data->challengePos },
    };

    return InitParamSet(paramSet, initParams, sizeof(initParams) / sizeof(HksParam));
}

int32_t ExecCryptoWrapper(const struct CryptParam *data)
{
    if (CheckCryptoParam(data, true) != HKS_SUCCESS) {
        LOGE("hks update crypto invalid argument\n");
        return HKS_ERROR_INVALID_ARGUMENT;
    }

    int32_t ret;
    struct HksParamSet *paramSet = nullptr;
    struct HksBlob handle = { data->handleLen, data->handleData };
    struct HksBlob inData = { data->dataInLen, (uint8_t *)data->dataIn };
    struct HksBlob outData = { data->dataOutLen, data->dataOut };

    /* init paramset */
    ret = CreateFinishParamSet(&paramSet, data);
    if (ret != HKS_SUCCESS) {
        LOGE("hks crypto update paramset err = %d\n", ret);
        return ret;
    }

    /* update */
    ret = HksUpdate(&handle, paramSet, &inData, &outData);
    if (ret != HKS_SUCCESS) {
        LOGE("hks crypto update failed err = %d\n", ret);
    }

    HksFreeParamSet(&paramSet);
    return ret;
}

int32_t DropCrypto(const struct CryptParam *data)
{
    if (CheckCryptoParam(data, false) != HKS_SUCCESS) {
        LOGE("hks finish crypto invalid argument\n");
        return HKS_ERROR_INVALID_ARGUMENT;
    }

    int32_t ret;
    struct HksParamSet *paramSet = nullptr;
    struct HksBlob handle = { data->handleLen, (uint8_t *)(data->handleData) };
    struct HksBlob inData = { 0, nullptr }; // HksFinish in drop, donnot need input
    struct HksBlob outData = { 0, nullptr };

    /* init paramset */
    ret = CreateCryptoParamSet(&paramSet, data);
    if (ret != HKS_SUCCESS) {
        LOGE("hks crypto finish paramset err = %d\n", ret);
        return ret;
    }

    /* update */
    ret = HksFinish(&handle, paramSet, &inData, &outData);
    if (ret != HKS_SUCCESS) {
        LOGE("hks crypto finish failed err = %d\n", ret);
    }

    HksFreeParamSet(&paramSet);
    return ret;
}