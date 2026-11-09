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

#include "huks_seed_deriver.h"

#include "hks_api.h"
#include "hks_param.h"
#include "privacy_error_code.h"
#include "privacy_log.h"
#include <iterator>
#include <limits>
#include <mutex>
#include <string>

namespace OHOS::Security::PrivacyComputation {
namespace {
using namespace OHOS::Security::PrivacyComputation::internal;
constexpr uint32_t ROOT_KEY_BITS = 256;
constexpr char ROOT_KEY_ALIAS[] = "asset.privacy.derive.root.v1";
std::mutex g_rootMutex;

int32_t BuildParamSet(HksParamSet **paramSet, const HksParam *params, uint32_t paramCount)
{
    auto ret = HksInitParamSet(paramSet);
    if (ret != HKS_SUCCESS) {
        LOGE("Initialize privacy HUKS parameters failed, ret=%{public}d", ret);
        return ret;
    }
    ret = HksAddParams(*paramSet, params, paramCount);
    if (ret != HKS_SUCCESS) {
        LOGE("Add privacy HUKS parameters failed, ret=%{public}d", ret);
        HksFreeParamSet(paramSet);
        return ret;
    }
    ret = HksBuildParamSet(paramSet);
    if (ret != HKS_SUCCESS) {
        LOGE("Build privacy HUKS parameters failed, ret=%{public}d", ret);
        HksFreeParamSet(paramSet);
    }
    return ret;
}

int32_t DeriveSeed(const HksBlob &alias, const HksBlob &salt, const HksParamSet *params, HeSeed &seed)
{
    std::array<uint8_t, sizeof(uint64_t)> handleData{};
    HksBlob handle{static_cast<uint32_t>(handleData.size()), handleData.data()};
    auto ret = HksInit(&alias, params, &handle, nullptr);
    if (ret != HKS_SUCCESS) {
        LOGE("Initialize privacy HUKS derivation failed, ret=%{public}d", ret);
        return PRIVACY_PROTOCOL_CRYPTO_ERROR;
    }
    LOGI("HUKS derive Init success");
    HeSeed updateStorage;
    HksBlob updateOutput{static_cast<uint32_t>(updateStorage.bytes.size()), updateStorage.bytes.data()};
    // HKDF consumes SALT/INFO tags; the API still requires an input blob for each stage.
    ret = HksUpdate(&handle, params, &salt, &updateOutput);
    if (ret != HKS_SUCCESS) {
        LOGE("Update privacy HUKS derivation failed, ret=%{public}d", ret);
        const auto abortRet = HksAbort(&handle, params);
        if (abortRet != HKS_SUCCESS) {
            LOGE("Abort privacy HUKS derivation failed, ret=%{public}d", abortRet);
        }
        return PRIVACY_PROTOCOL_CRYPTO_ERROR;
    }
    LOGI("HUKS derive Update success");
    HeSeed derived;
    HksBlob output{static_cast<uint32_t>(derived.bytes.size()), derived.bytes.data()};
    ret = HksFinish(&handle, params, &salt, &output);
    if (ret != HKS_SUCCESS) {
        LOGE("Finish privacy HUKS derivation failed, ret=%{public}d", ret);
        const auto abortRet = HksAbort(&handle, params);
        if (abortRet != HKS_SUCCESS) {
            LOGE("Abort privacy HUKS derivation failed, ret=%{public}d", abortRet);
        }
        return PRIVACY_PROTOCOL_CRYPTO_ERROR;
    }
    if (output.size != derived.bytes.size()) {
        LOGE("Privacy HUKS seed has invalid length=%{public}u", output.size);
        return PRIVACY_PROTOCOL_CRYPTO_ERROR;
    }
    seed.bytes = derived.bytes;
    LOGI("HUKS derive Finish success, seedBytes=%{public}u", output.size);
    return PRIVACY_PROTOCOL_SUCCESS;
}

int32_t GenerateRoot(const HksBlob &alias, uint32_t userId)
{
    const HksParam params[] = {
        {.tag = HKS_TAG_ALGORITHM, .uint32Param = HKS_ALG_AES},
        {.tag = HKS_TAG_PURPOSE, .uint32Param = HKS_KEY_PURPOSE_DERIVE},
        {.tag = HKS_TAG_KEY_SIZE, .uint32Param = ROOT_KEY_BITS},
        {.tag = HKS_TAG_DIGEST, .uint32Param = HKS_DIGEST_SHA256},
        {.tag = HKS_TAG_AUTH_STORAGE_LEVEL, .uint32Param = HKS_AUTH_STORAGE_LEVEL_CE},
        {.tag = HKS_TAG_SPECIFIC_USER_ID, .int32Param = static_cast<int32_t>(userId)},
    };
    HksParamSet *paramSet = nullptr;
    const auto ret = BuildParamSet(&paramSet, params, static_cast<uint32_t>(std::size(params)));
    if (ret != HKS_SUCCESS) {
        return PRIVACY_PROTOCOL_CRYPTO_ERROR;
    }
    LOGI("HUKS root generate begin, userId=%{private}u", userId);
    const auto generateRet = HksGenerateKey(&alias, paramSet, nullptr);
    HksFreeParamSet(&paramSet);
    if (generateRet != HKS_SUCCESS) {
        LOGE("Generate privacy HUKS derivation root failed, ret=%{public}d", generateRet);
        return PRIVACY_PROTOCOL_CRYPTO_ERROR;
    }
    LOGI("HUKS root generate success, userId=%{private}u", userId);
    return PRIVACY_PROTOCOL_SUCCESS;
}
} // namespace

int32_t HuksSeedDeriver::PrepareRootKey(uint32_t userId, RootKeyPolicy policy) const
{
    if (userId > static_cast<uint32_t>(std::numeric_limits<int32_t>::max())) {
        LOGE("Invalid HUKS CE userId");
        return PRIVACY_PROTOCOL_INVALID_ARGUMENT;
    }
    LOGI("HUKS root prepare begin, userId=%{private}u, policy=%{public}u", userId, static_cast<uint32_t>(policy));
    const HksParam params[] = {
        {.tag = HKS_TAG_AUTH_STORAGE_LEVEL, .uint32Param = HKS_AUTH_STORAGE_LEVEL_CE},
        {.tag = HKS_TAG_SPECIFIC_USER_ID, .int32Param = static_cast<int32_t>(userId)},
    };
    HksParamSet *paramSet = nullptr;
    if (BuildParamSet(&paramSet, params, static_cast<uint32_t>(std::size(params))) != HKS_SUCCESS) {
        return PRIVACY_PROTOCOL_CRYPTO_ERROR;
    }
    std::string aliasName = ROOT_KEY_ALIAS;
    const HksBlob alias{static_cast<uint32_t>(aliasName.size()), reinterpret_cast<uint8_t *>(aliasName.data())};
    std::lock_guard<std::mutex> lock(g_rootMutex);
    const auto ret = HksKeyExist(&alias, paramSet);
    HksFreeParamSet(&paramSet);
    if (ret == HKS_SUCCESS) {
        LOGI("HUKS root exists, userId=%{private}u", userId);
        return PRIVACY_PROTOCOL_SUCCESS;
    }
    if (ret == HKS_ERROR_NOT_EXIST && policy == RootKeyPolicy::CREATE_IF_MISSING) {
        return GenerateRoot(alias, userId);
    }
    LOGE("Query privacy HUKS root failed, ret=%{public}d, policy=%{public}u", ret, static_cast<uint32_t>(policy));
    return PRIVACY_PROTOCOL_CRYPTO_ERROR;
}

int32_t HuksSeedDeriver::Derive(const CallerIdentity &identity,
                                const std::array<uint8_t, kIdentityHashSize> &materialHash, HeSeed &seed) const noexcept
{
    if (identity.userId > static_cast<uint32_t>(std::numeric_limits<int32_t>::max())) {
        LOGE("Invalid HUKS derivation CE userId");
        return PRIVACY_PROTOCOL_INVALID_ARGUMENT;
    }
    LOGI("HUKS derive begin, userId=%{private}u", identity.userId);
    std::string aliasName = ROOT_KEY_ALIAS;
    const HksBlob alias{static_cast<uint32_t>(aliasName.size()), reinterpret_cast<uint8_t *>(aliasName.data())};
    const HksBlob salt{static_cast<uint32_t>(materialHash.size()), const_cast<uint8_t *>(materialHash.data())};
    const HksParam params[] = {
        {.tag = HKS_TAG_ALGORITHM, .uint32Param = HKS_ALG_HKDF},
        {.tag = HKS_TAG_PURPOSE, .uint32Param = HKS_KEY_PURPOSE_DERIVE},
        {.tag = HKS_TAG_DIGEST, .uint32Param = HKS_DIGEST_SHA256},
        {.tag = HKS_TAG_DERIVE_KEY_SIZE, .uint32Param = kHeSeedSize},
        {.tag = HKS_TAG_SALT, .blob = salt},
        {.tag = HKS_TAG_DERIVE_AGREE_KEY_STORAGE_FLAG, .uint32Param = HKS_STORAGE_ALLOW_KEY_EXPORTED},
        {.tag = HKS_TAG_AUTH_STORAGE_LEVEL, .uint32Param = HKS_AUTH_STORAGE_LEVEL_CE},
        {.tag = HKS_TAG_SPECIFIC_USER_ID, .int32Param = static_cast<int32_t>(identity.userId)},
    };
    HksParamSet *paramSet = nullptr;
    auto ret = BuildParamSet(&paramSet, params, static_cast<uint32_t>(std::size(params)));
    if (ret != HKS_SUCCESS) {
        return PRIVACY_PROTOCOL_CRYPTO_ERROR;
    }
    ret = DeriveSeed(alias, salt, paramSet, seed);
    HksFreeParamSet(&paramSet);
    return ret;
}
} // namespace OHOS::Security::PrivacyComputation
