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

#include <stdint.h>

#include "hks_api.h"
#include "hks_param.h"

#include "asset_log.h"
#include "asset_type.h"
#include "huks_wrapper.h"

static enum HksAuthStorageLevel AccessibilityToHksAuthStorageLevel(enum Accessibility accessibility)
{
    switch (accessibility) {
        case DEVICE_POWERED_ON:
            return HKS_AUTH_STORAGE_LEVEL_DE;
        case DEVICE_FIRST_UNLOCKED:
            return HKS_AUTH_STORAGE_LEVEL_CE;
        default:
            return HKS_AUTH_STORAGE_LEVEL_ECE;
    }
}

static int32_t HuksErrorTransfer(int32_t ret)
{
    switch (ret) {
        case HKS_SUCCESS:
            return ASSET_SUCCESS;
        case HKS_ERROR_NO_PERMISSION:
        case HKS_ERROR_DEVICE_PASSWORD_UNSET:
            return ASSET_STATUS_MISMATCH;
        case HKS_ERROR_NOT_EXIST:
            return ASSET_NOT_FOUND;
        case HKS_ERROR_KEY_AUTH_FAILED:
        case HKS_ERROR_KEY_AUTH_VERIFY_FAILED:
            return ASSET_ACCESS_DENIED;
        case HKS_ERROR_CRYPTO_ENGINE_ERROR:
            return ASSET_DATA_CORRUPTED;
        default:
            return ASSET_CRYPTO_ERROR;
    }
}

static int32_t AddSpecificUserIdParams(struct HksParamSet *paramSet, int32_t userId)
{
    struct HksParam specificUserIdParams[] = {
        { .tag = HKS_TAG_SPECIFIC_USER_ID, .int32Param = userId },
    };
    return HksAddParams(paramSet, specificUserIdParams, ARRAY_SIZE(specificUserIdParams));
}

static int32_t BuildParamSet(struct HksParamSet **paramSet, const struct HksParam *params, uint32_t paramCount,
    int32_t userId)
{
    int32_t ret = HksInitParamSet(paramSet);
    if (ret != HKS_SUCCESS) {
        LOGE("[FATAL]HUKS init param set failed. error=%{public}d", ret);
        return ret;
    }

    if (paramCount != 0) {
        ret = HksAddParams(*paramSet, params, paramCount);
        if (ret != HKS_SUCCESS) {
            LOGE("[FATAL]HUKS add params failed. error=%{public}d", ret);
            HksFreeParamSet(paramSet);
            return ret;
        }

        if (userId > ASSET_ROOT_USER_UPPERBOUND) {
            ret = AddSpecificUserIdParams(*paramSet, userId);
            if (ret != HKS_SUCCESS) {
                LOGE("[FATAL]HUKS add specific userId failed. error=%{public}d", ret);
                HksFreeParamSet(paramSet);
                return ret;
            }
        }
    }

    ret = HksBuildParamSet(paramSet);
    if (ret != HKS_SUCCESS) {
        LOGE("[FATAL]HUKS build param set failed. error=%{public}d", ret);
        HksFreeParamSet(paramSet);
    }
    return ret;
}

static int32_t AddCommonGenParams(struct HksParamSet *paramSet, const struct KeyId *keyId)
{
    struct HksParam commonParams[] = {
        { .tag = HKS_TAG_ALGORITHM, .uint32Param = HKS_ALG_AES },
        { .tag = HKS_TAG_PURPOSE, .uint32Param = HKS_KEY_PURPOSE_ENCRYPT | HKS_KEY_PURPOSE_DECRYPT },
        { .tag = HKS_TAG_KEY_SIZE, .uint32Param = HKS_AES_KEY_SIZE_256 },
        { .tag = HKS_TAG_PADDING, .uint32Param = HKS_PADDING_NONE },
        { .tag = HKS_TAG_BLOCK_MODE, .uint32Param = HKS_MODE_GCM },
        { .tag = HKS_TAG_AUTH_STORAGE_LEVEL, .uint32Param = AccessibilityToHksAuthStorageLevel(keyId->accessibility) },
        { .tag = HKS_TAG_IS_ALLOWED_DATA_WRAP, .boolParam = true },
    };
    return HksAddParams(paramSet, commonParams, ARRAY_SIZE(commonParams));
}

static int32_t AddAuthGenParams(struct HksParamSet *paramSet)
{
    struct HksParam authParams[] = {
        { .tag = HKS_TAG_KEY_AUTH_PURPOSE, .uint32Param = HKS_KEY_PURPOSE_DECRYPT },
        { .tag = HKS_TAG_KEY_AUTH_ACCESS_TYPE, .uint32Param = HKS_AUTH_ACCESS_ALWAYS_VALID },
        { .tag = HKS_TAG_BATCH_PURPOSE, .uint32Param = HKS_KEY_PURPOSE_DECRYPT },
        { .tag = HKS_TAG_CHALLENGE_TYPE, .uint32Param = HKS_CHALLENGE_TYPE_NORMAL },
        { .tag = HKS_TAG_USER_AUTH_TYPE, .uint32Param =
            HKS_USER_AUTH_TYPE_FINGERPRINT | HKS_USER_AUTH_TYPE_FACE | HKS_USER_AUTH_TYPE_PIN }
    };
    return HksAddParams(paramSet, authParams, ARRAY_SIZE(authParams));
}

int32_t GenerateKey(const struct KeyId *keyId, bool needAuth, bool requirePasswordSet)
{
    struct HksParamSet *paramSet = NULL;
    int32_t ret = HKS_SUCCESS;
    do {
        ret = HksInitParamSet(&paramSet);
        if (ret != HKS_SUCCESS) {
            LOGE("[FATAL]HUKS init param set failed. error=%{public}d", ret);
            break;
        }

        ret = AddCommonGenParams(paramSet, keyId);
        if (ret != HKS_SUCCESS) {
            LOGE("[FATAL]HUKS add common params failed. error=%{public}d", ret);
            break;
        }

        if (keyId->userId > ASSET_ROOT_USER_UPPERBOUND) {
            ret = AddSpecificUserIdParams(paramSet, keyId->userId);
            if (ret != HKS_SUCCESS) {
                LOGE("[FATAL]HUKS add specific userId failed. error=%{public}d", ret);
                break;
            }
        }

        if (needAuth) {
            ret = AddAuthGenParams(paramSet);
            if (ret != HKS_SUCCESS) {
                LOGE("[FATAL]HUKS add auth params failed. error=%{public}d", ret);
                break;
            }
        }

        if (requirePasswordSet) {
            struct HksParam tempParam = { .tag = HKS_TAG_IS_DEVICE_PASSWORD_SET, .boolParam = true };
            ret = HksAddParams(paramSet, &tempParam, 1); // 1: add one param to paramSet
            if (ret != HKS_SUCCESS) {
                LOGE("[FATAL]HUKS add requirePasswordSet param failed. error=%{public}d", ret);
                break;
            }
        }

        ret = HksBuildParamSet(&paramSet);
        if (ret != HKS_SUCCESS) {
            LOGE("[FATAL]HUKS build param set failed. error=%{public}d", ret);
            break;
        }

        ret = HksGenerateKey(&keyId->alias, paramSet, NULL);
        if (ret != HKS_SUCCESS) {
            LOGE("[FATAL]HUKS generate key failed. error=%{public}d", ret);
        }
    } while (0);

    HksFreeParamSet(&paramSet);
    return HuksErrorTransfer(ret);
}

int32_t DeleteKey(const struct KeyId *keyId)
{
    struct HksParam params[] = {
        { .tag = HKS_TAG_AUTH_STORAGE_LEVEL, .uint32Param = AccessibilityToHksAuthStorageLevel(keyId->accessibility) },
    };
    struct HksParamSet *paramSet = NULL;
    int32_t ret = BuildParamSet(&paramSet, params, ARRAY_SIZE(params), keyId->userId);
    if (ret != HKS_SUCCESS) {
        return HuksErrorTransfer(ret);
    }

    ret = HksDeleteKey(&keyId->alias, paramSet);
    HksFreeParamSet(&paramSet);
    return HuksErrorTransfer(ret);
}

int32_t IsKeyExist(const struct KeyId *keyId)
{
    struct HksParam params[] = {
        { .tag = HKS_TAG_AUTH_STORAGE_LEVEL, .uint32Param = AccessibilityToHksAuthStorageLevel(keyId->accessibility) },
    };
    struct HksParamSet *paramSet = NULL;
    int32_t ret = BuildParamSet(&paramSet, params, ARRAY_SIZE(params), keyId->userId);
    if (ret != HKS_SUCCESS) {
        return HuksErrorTransfer(ret);
    }

    ret = HksKeyExist(&keyId->alias, paramSet);
    HksFreeParamSet(&paramSet);
    return HuksErrorTransfer(ret);
}

int32_t EncryptData(const struct KeyId *keyId, const struct HksBlob *aad, const struct HksBlob *inData,
    struct HksBlob *outData)
{
    struct HksParam encryptParams[] = {
        { .tag = HKS_TAG_ALGORITHM, .uint32Param = HKS_ALG_AES },
        { .tag = HKS_TAG_PURPOSE, .uint32Param = HKS_KEY_PURPOSE_ENCRYPT },
        { .tag = HKS_TAG_KEY_SIZE, .uint32Param = HKS_AES_KEY_SIZE_256 },
        { .tag = HKS_TAG_PADDING, .uint32Param = HKS_PADDING_NONE },
        { .tag = HKS_TAG_BLOCK_MODE, .uint32Param = HKS_MODE_GCM },
        { .tag = HKS_TAG_ASSOCIATED_DATA, .blob = *aad },
        { .tag = HKS_TAG_AUTH_STORAGE_LEVEL, .uint32Param = AccessibilityToHksAuthStorageLevel(keyId->accessibility) },
    };
    struct HksParamSet *encryptParamSet = NULL;
    int32_t ret = BuildParamSet(&encryptParamSet, encryptParams, ARRAY_SIZE(encryptParams), keyId->userId);
    if (ret != HKS_SUCCESS) {
        return HuksErrorTransfer(ret);
    }

    uint8_t handle[sizeof(uint64_t)] = { 0 };
    struct HksBlob handleBlob = { sizeof(uint64_t), handle };
    ret = HksInit(&keyId->alias, encryptParamSet, &handleBlob, NULL);
    if (ret != HKS_SUCCESS) {
        LOGE("[FATAL]HUKS encrypt init failed. error=%{public}d", ret);
        HksFreeParamSet(&encryptParamSet);
        return HuksErrorTransfer(ret);
    }

    ret = HksFinish(&handleBlob, encryptParamSet, inData, outData);
    HksFreeParamSet(&encryptParamSet);
    if (ret != HKS_SUCCESS) {
        LOGE("[FATAL]HUKS encrypt finish failed. error=%{public}d", ret);
    }
    return HuksErrorTransfer(ret);
}

int32_t DecryptData(const struct KeyId *keyId, const struct HksBlob *aad, const struct HksBlob *inData,
    struct HksBlob *outData)
{
    struct HksBlob cipher = { inData->size - NONCE_SIZE - TAG_SIZE, inData->data };
    struct HksBlob tag = { TAG_SIZE, inData->data + (inData->size - NONCE_SIZE - TAG_SIZE) };
    struct HksBlob nonce = { NONCE_SIZE, inData->data + (inData->size - NONCE_SIZE) };

    struct HksParamSet *decryptParamSet = NULL;
    struct HksParam decryptParams[] = {
        { .tag = HKS_TAG_ALGORITHM, .uint32Param = HKS_ALG_AES },
        { .tag = HKS_TAG_PURPOSE, .uint32Param = HKS_KEY_PURPOSE_DECRYPT },
        { .tag = HKS_TAG_KEY_SIZE, .uint32Param = HKS_AES_KEY_SIZE_256 },
        { .tag = HKS_TAG_PADDING, .uint32Param = HKS_PADDING_NONE },
        { .tag = HKS_TAG_BLOCK_MODE, .uint32Param = HKS_MODE_GCM },
        { .tag = HKS_TAG_ASSOCIATED_DATA, .blob = *aad },
        { .tag = HKS_TAG_NONCE, .blob = nonce },
        { .tag = HKS_TAG_AE_TAG, .blob = tag },
        { .tag = HKS_TAG_AUTH_STORAGE_LEVEL, .uint32Param = AccessibilityToHksAuthStorageLevel(keyId->accessibility) },
    };

    int32_t ret = BuildParamSet(&decryptParamSet, decryptParams, ARRAY_SIZE(decryptParams), keyId->userId);
    if (ret != HKS_SUCCESS) {
        return HuksErrorTransfer(ret);
    }

    uint8_t handle[sizeof(uint64_t)] = { 0 };
    struct HksBlob handleBlob = { sizeof(uint64_t), handle };
    ret = HksInit(&keyId->alias, decryptParamSet, &handleBlob, NULL);
    if (ret != HKS_SUCCESS) {
        LOGE("[FATAL]HUKS decrypt init failed. error=%{public}d", ret);
        HksFreeParamSet(&decryptParamSet);
        return HuksErrorTransfer(ret);
    }

    ret = HksFinish(&handleBlob, decryptParamSet, &cipher, outData);
    HksFreeParamSet(&decryptParamSet);
    if (ret != HKS_SUCCESS) {
        LOGE("[FATAL]HUKS decrypt finish failed. error=%{public}d", ret);
    }
    return HuksErrorTransfer(ret);
}

int32_t InitKey(const struct KeyId *keyId, uint32_t validTime, struct HksBlob *challenge, struct HksBlob *handle)
{
    struct HksParam initParams[] = {
        { .tag = HKS_TAG_ALGORITHM, .uint32Param = HKS_ALG_AES},
        { .tag = HKS_TAG_PURPOSE, .uint32Param = HKS_KEY_PURPOSE_DECRYPT },
        { .tag = HKS_TAG_KEY_SIZE, .uint32Param = HKS_AES_KEY_SIZE_256 },
        { .tag = HKS_TAG_IS_BATCH_OPERATION, .boolParam = true },
        { .tag = HKS_TAG_BATCH_OPERATION_TIMEOUT, .uint32Param = validTime },
        { .tag = HKS_TAG_AUTH_STORAGE_LEVEL, .uint32Param = AccessibilityToHksAuthStorageLevel(keyId->accessibility) },
    };
    struct HksParamSet *paramSet = NULL;
    int32_t ret = BuildParamSet(&paramSet, initParams, ARRAY_SIZE(initParams), keyId->userId);
    if (ret != HKS_SUCCESS) {
        return HuksErrorTransfer(ret);
    }

    ret = HksInit(&keyId->alias, paramSet, handle, challenge);
    HksFreeParamSet(&paramSet);
    if (ret != HKS_SUCCESS) {
        LOGE("[FATAL]HUKS batch decrypt init failed. error=%{public}d", ret);
    }
    return HuksErrorTransfer(ret);
}

int32_t ExecCrypt(const struct HksBlob *handle, const struct HksBlob *aad, const struct HksBlob *authToken,
    const struct HksBlob *inData, struct HksBlob *outData)
{
    struct HksBlob tag = { TAG_SIZE, inData->data + (inData->size - NONCE_SIZE - TAG_SIZE) };
    struct HksBlob nonce = { NONCE_SIZE, inData->data + (inData->size - NONCE_SIZE) };

    struct HksParam updateParams[] = {
        { .tag = HKS_TAG_ALGORITHM, .uint32Param = HKS_ALG_AES },
        { .tag = HKS_TAG_PURPOSE, .uint32Param = HKS_KEY_PURPOSE_DECRYPT },
        { .tag = HKS_TAG_KEY_SIZE, .uint32Param = HKS_AES_KEY_SIZE_256 },
        { .tag = HKS_TAG_PADDING, .uint32Param = HKS_PADDING_NONE },
        { .tag = HKS_TAG_BLOCK_MODE, .uint32Param = HKS_MODE_GCM },
        { .tag = HKS_TAG_ASSOCIATED_DATA, .blob = { .size = aad->size, .data = aad->data } },
        { .tag = HKS_TAG_NONCE, .blob = nonce },
        { .tag = HKS_TAG_AE_TAG, .blob = tag },
        { .tag = HKS_TAG_AUTH_TOKEN, .blob = *authToken },
    };

    struct HksParamSet *paramSet = NULL;
    int32_t ret = BuildParamSet(&paramSet, updateParams, ARRAY_SIZE(updateParams), 0);
    if (ret != HKS_SUCCESS) {
        return HuksErrorTransfer(ret);
    }

    struct HksBlob cipher = { inData->size - NONCE_SIZE - TAG_SIZE, inData->data };
    ret = HksUpdate(handle, paramSet, &cipher, outData);
    HksFreeParamSet(&paramSet);
    if (ret != HKS_SUCCESS) {
        LOGE("[FATAL]HUKS batch decrypt update failed. error=%{public}d", ret);
    }
    return HuksErrorTransfer(ret);
}

int32_t Drop(const struct HksBlob *handle)
{
    struct HksBlob inData = { 0, NULL };
    struct HksBlob outData = { 0, NULL };

    struct HksParamSet *paramSet = NULL;
    int32_t ret = BuildParamSet(&paramSet, NULL, 0, 0);
    if (ret != HKS_SUCCESS) {
        return HuksErrorTransfer(ret);
    }

    ret = HksFinish(handle, paramSet, &inData, &outData);
    HksFreeParamSet(&paramSet);
    if (ret != HKS_SUCCESS) {
        LOGE("[FATAL]HUKS batch decrypt finish failed. error=%{public}d", ret);
    }
    return HuksErrorTransfer(ret);
}

int32_t RenameKeyAlias(const struct KeyId *keyId, const struct HksBlob *newKeyAlias)
{
    struct HksParam params[] = {
        { .tag = HKS_TAG_AUTH_STORAGE_LEVEL, .uint32Param = AccessibilityToHksAuthStorageLevel(keyId->accessibility) },
        { .tag = HKS_TAG_IS_COPY_NEW_KEY, .boolParam = true },
    };
    struct HksParamSet *paramSet = NULL;
    int32_t ret = BuildParamSet(&paramSet, params, ARRAY_SIZE(params), keyId->userId);
    if (ret != HKS_SUCCESS) {
        return HuksErrorTransfer(ret);
    }

    ret = HksRenameKeyAlias(&keyId->alias, paramSet, newKeyAlias);
    HksFreeParamSet(&paramSet);
    if (ret != HKS_SUCCESS) {
        LOGE("[FATAL]HUKS rename key alias failed. error=%{public}d", ret);
    }
    return HuksErrorTransfer(ret);
}