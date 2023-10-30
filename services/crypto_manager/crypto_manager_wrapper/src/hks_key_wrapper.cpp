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
#include <stdio.h>

#include "hks_key_wrapper.h"
#include "hks_param.h"
#include "asset_log.h"
#include "asset_mem.h"

const int HEX_RETIO = 2;

int32_t InitParamSet(struct HksParamSet **paramSet, const struct HksParam *params, uint32_t paramcount)
{
    if (paramSet == nullptr) {
        LOGE("bad params\n");
        return HKS_FAILURE;
    }

    int32_t ret = HksInitParamSet(paramSet);
    if (ret != HKS_SUCCESS) {
        LOGE("HksInitParamSet failed");
        return ret;
    }

    if (params != nullptr && paramcount > 0) {
        ret = HksAddParams(*paramSet, params, paramcount);
        if (ret != HKS_SUCCESS) {
            LOGE("HksAddParams failed");
            HksFreeParamSet(paramSet);
            return ret;
        }
    }

    ret = HksBuildParamSet(paramSet);
    if (ret != HKS_SUCCESS) {
        LOGE("HksBuildParamSet failed!");
        HksFreeParamSet(paramSet);
        return ret;
    }
    return ret;
}

static void PrintBytes(const char *tag, const uint8_t *stream, const uint32_t length)
{
    uint32_t mallocLen = length * HEX_RETIO + 1;
    char *str = static_cast<char *>(AssetMalloc(mallocLen));
    for (uint32_t i = 0; i < length; i++)
        (void)sprintf(str + HEX_RETIO * i, "%.2x", stream[i]);
    LOGE("[YYDS] %{public}s: size=%{public}u, value=%{public}s", tag, length, str);
    AssetFree(str);
}

static int32_t CreateGenKeyParamSet(struct HksParamSet **paramSet, bool needUserAuth)
{
    if (needUserAuth) {
        LOGE("gen key for auth type, only support decrypt\n");
        struct HksParam genKeyParams[] = {
            { .tag = HKS_TAG_ALGORITHM, .uint32Param = HKS_ALG_AES },
            { .tag = HKS_TAG_PURPOSE, .uint32Param = HKS_KEY_PURPOSE_ENCRYPT | HKS_KEY_PURPOSE_DECRYPT },
            { .tag = HKS_TAG_KEY_SIZE, .uint32Param = HKS_AES_KEY_SIZE_256 },
            { .tag = HKS_TAG_PADDING, .uint32Param = HKS_PADDING_NONE },
            { .tag = HKS_TAG_BLOCK_MODE, .uint32Param = HKS_MODE_GCM },
            { .tag = HKS_TAG_KEY_AUTH_PURPOSE, .uint32Param = HKS_KEY_PURPOSE_DECRYPT },
            { .tag = HKS_TAG_KEY_AUTH_ACCESS_TYPE, .uint32Param = HKS_AUTH_ACCESS_ALWAYS_VALID },
            { .tag = HKS_TAG_CHALLENGE_TYPE, .uint32Param = HKS_CHALLENGE_TYPE_CUSTOM },
            { .tag = HKS_TAG_BATCH_PURPOSE, .uint32Param = HKS_KEY_PURPOSE_DECRYPT },
            { .tag = HKS_TAG_USER_AUTH_TYPE, .uint32Param =
                HKS_USER_AUTH_TYPE_FINGERPRINT | HKS_USER_AUTH_TYPE_FACE | HKS_USER_AUTH_TYPE_PIN }
        };

        return InitParamSet(paramSet, genKeyParams, sizeof(genKeyParams) / sizeof(HksParam));
    } else {
        LOGE("gen key for no auth type\n");
        struct HksParam genKeyParams[] = {
            { .tag = HKS_TAG_ALGORITHM, .uint32Param = HKS_ALG_AES },
            { .tag = HKS_TAG_PURPOSE, .uint32Param = HKS_KEY_PURPOSE_ENCRYPT | HKS_KEY_PURPOSE_DECRYPT },
            { .tag = HKS_TAG_KEY_SIZE, .uint32Param = HKS_AES_KEY_SIZE_256 },
            { .tag = HKS_TAG_PADDING, .uint32Param = HKS_PADDING_NONE },
            { .tag = HKS_TAG_BLOCK_MODE, .uint32Param = HKS_MODE_GCM }
        };

        return InitParamSet(paramSet, genKeyParams, sizeof(genKeyParams) / sizeof(HksParam));
    }
}

int32_t GenerateKey(const struct HksBlob *keyData, bool needUserAuth)
{
    if (keyData->size == 0 || keyData->data == nullptr) {
        LOGE("invalid key data\n");
        return HKS_ERROR_INVALID_ARGUMENT;
    }

    PrintBytes("GenerateKey", keyData->data, keyData->size);
    struct HksParamSet *paramSetIn = nullptr;
    int32_t ret = CreateGenKeyParamSet(&paramSetIn, needUserAuth);
    if (ret != HKS_SUCCESS) {
        LOGE("generate key huks init param failed\n");
        HksFreeParamSet(&paramSetIn);
        return ret;
    }

    ret = HksGenerateKey(keyData, paramSetIn, nullptr);
    if (ret != HKS_SUCCESS) {
        LOGE("generate key huks failed\n");
    }

    HksFreeParamSet(&paramSetIn);
    return ret;
}

int32_t DeleteKey(const struct HksBlob *keyData)
{
    if (keyData->size == 0 || keyData->data == nullptr) {
        LOGE("invalid key data\n");
        return HKS_ERROR_INVALID_ARGUMENT;
    }

    PrintBytes("DeleteKey", keyData->data, keyData->size);
    return HksDeleteKey(keyData, nullptr);
}

int32_t KeyExist(const struct HksBlob *keyData)
{
    if (keyData->size == 0 || keyData->data == nullptr) {
        LOGE("invalid key data\n");
        return HKS_ERROR_INVALID_ARGUMENT;
    }

    return HksKeyExist(keyData, nullptr);
}