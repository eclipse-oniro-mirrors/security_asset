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
#include "psi_pir_error_code.h"
#include "psi_pir_log.h"
#include <iterator>
#include <mutex>
#include <string>

namespace OHOS::Security::PrivacyComputation {
namespace {
using namespace psi_pir;
using namespace psi_pir::internal;
constexpr uint32_t ROOT_KEY_BITS = 256;
constexpr char ROOT_KEY_ALIAS[] = "asset.hpic.derive.root.v1";
std::mutex g_rootMutex;

int32_t BuildParamSet(HksParamSet **paramSet, const HksParam *params, uint32_t paramCount)
{
    auto ret = HksInitParamSet(paramSet);
    if (ret != HKS_SUCCESS) {
        PSI_PIR_LOGE("Initialize privacy HUKS parameters failed, ret=%{public}d", ret);
        return ret;
    }
    ret = HksAddParams(*paramSet, params, paramCount);
    if (ret != HKS_SUCCESS) {
        PSI_PIR_LOGE("Add privacy HUKS parameters failed, ret=%{public}d", ret);
        HksFreeParamSet(paramSet);
        return ret;
    }
    ret = HksBuildParamSet(paramSet);
    if (ret != HKS_SUCCESS) {
        PSI_PIR_LOGE("Build privacy HUKS parameters failed, ret=%{public}d", ret);
        HksFreeParamSet(paramSet);
    }
    return ret;
}

// The parameter set outlives the session. Any exit before a successful Finish aborts it.
class HuksDeriveSession final {
public:
    explicit HuksDeriveSession(const HksParamSet *params) : params_(params) {}
    ~HuksDeriveSession()
    {
        if (!active_) {
            return;
        }
        const auto ret = HksAbort(&handle_, params_);
        if (ret != HKS_SUCCESS) {
            PSI_PIR_LOGE("Abort privacy HUKS derivation failed, ret=%{public}d", ret);
        }
    }
    HuksDeriveSession(const HuksDeriveSession &) = delete;
    HuksDeriveSession &operator=(const HuksDeriveSession &) = delete;

    int32_t Derive(const HksBlob &alias, const HksBlob &salt, HeSeed &seed)
    {
        auto ret = HksInit(&alias, params_, &handle_, nullptr);
        if (ret != HKS_SUCCESS) {
            PSI_PIR_LOGE("Initialize privacy HUKS derivation failed, ret=%{public}d", ret);
            return PSI_PIR_CRYPTO_ERROR;
        }
        active_ = true;
        HeSeed updateStorage;
        HksBlob updateOutput{static_cast<uint32_t>(updateStorage.bytes.size()), updateStorage.bytes.data()};
        // HKDF consumes SALT/INFO tags; the API still requires an input blob for each stage.
        ret = HksUpdate(&handle_, params_, &salt, &updateOutput);
        if (ret != HKS_SUCCESS) {
            PSI_PIR_LOGE("Update privacy HUKS derivation failed, ret=%{public}d", ret);
            return PSI_PIR_CRYPTO_ERROR;
        }
        HeSeed derived;
        HksBlob output{static_cast<uint32_t>(derived.bytes.size()), derived.bytes.data()};
        ret = HksFinish(&handle_, params_, &salt, &output);
        if (ret != HKS_SUCCESS) {
            PSI_PIR_LOGE("Finish privacy HUKS derivation failed, ret=%{public}d", ret);
            return PSI_PIR_CRYPTO_ERROR;
        }
        active_ = false;
        if (output.size != derived.bytes.size()) {
            PSI_PIR_LOGE("Privacy HUKS seed has invalid length=%{public}u", output.size);
            return PSI_PIR_CRYPTO_ERROR;
        }
        seed.bytes = derived.bytes;
        return PSI_PIR_SUCCESS;
    }

private:
    const HksParamSet *params_;
    std::array<uint8_t, sizeof(uint64_t)> handleData_{};
    HksBlob handle_{static_cast<uint32_t>(handleData_.size()), handleData_.data()};
    bool active_ = false;
};

int32_t GenerateRoot(const HksBlob &alias)
{
    const HksParam params[] = {
        {.tag = HKS_TAG_ALGORITHM, .uint32Param = HKS_ALG_AES},
        {.tag = HKS_TAG_PURPOSE, .uint32Param = HKS_KEY_PURPOSE_DERIVE},
        {.tag = HKS_TAG_KEY_SIZE, .uint32Param = ROOT_KEY_BITS},
        {.tag = HKS_TAG_DIGEST, .uint32Param = HKS_DIGEST_SHA256},
        {.tag = HKS_TAG_AUTH_STORAGE_LEVEL, .uint32Param = HKS_AUTH_STORAGE_LEVEL_CE},
    };
    HksParamSet *paramSet = nullptr;
    const auto ret = BuildParamSet(&paramSet, params, static_cast<uint32_t>(std::size(params)));
    if (ret != HKS_SUCCESS) {
        return PSI_PIR_CRYPTO_ERROR;
    }
    const auto generateRet = HksGenerateKey(&alias, paramSet, nullptr);
    HksFreeParamSet(&paramSet);
    if (generateRet != HKS_SUCCESS) {
        PSI_PIR_LOGE("Generate privacy HUKS derivation root failed, ret=%{public}d", generateRet);
        return PSI_PIR_CRYPTO_ERROR;
    }
    return PSI_PIR_SUCCESS;
}
} // namespace

int32_t HuksSeedDeriver::PrepareRootKey(RootKeyPolicy policy) const
{
    const HksParam params[] = {
        {.tag = HKS_TAG_AUTH_STORAGE_LEVEL, .uint32Param = HKS_AUTH_STORAGE_LEVEL_CE},
    };
    HksParamSet *paramSet = nullptr;
    if (BuildParamSet(&paramSet, params, static_cast<uint32_t>(std::size(params))) != HKS_SUCCESS) {
        return PSI_PIR_CRYPTO_ERROR;
    }
    std::string aliasName = ROOT_KEY_ALIAS;
    const HksBlob alias{static_cast<uint32_t>(aliasName.size()), reinterpret_cast<uint8_t *>(aliasName.data())};
    std::lock_guard<std::mutex> lock(g_rootMutex);
    const auto ret = HksKeyExist(&alias, paramSet);
    HksFreeParamSet(&paramSet);
    if (ret == HKS_SUCCESS) {
        return PSI_PIR_SUCCESS;
    }
    if (ret == HKS_ERROR_NOT_EXIST && policy == RootKeyPolicy::CREATE_IF_MISSING) {
        return GenerateRoot(alias);
    }
    PSI_PIR_LOGE("Query privacy HUKS root failed, ret=%{public}d, policy=%{public}u", ret,
                 static_cast<uint32_t>(policy));
    return PSI_PIR_CRYPTO_ERROR;
}

int32_t HuksSeedDeriver::Derive(const CallerIdentity &, const std::array<uint8_t, kIdentityHashSize> &materialHash,
                                HeSeed &seed) const noexcept
{
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
    };
    HksParamSet *paramSet = nullptr;
    auto ret = BuildParamSet(&paramSet, params, static_cast<uint32_t>(std::size(params)));
    if (ret != HKS_SUCCESS) {
        return PSI_PIR_CRYPTO_ERROR;
    }
    {
        // Abort unfinished sessions before freeing the parameter set they borrow.
        HuksDeriveSession session(paramSet);
        ret = session.Derive(alias, salt, seed);
    }
    HksFreeParamSet(&paramSet);
    return ret;
}
} // namespace OHOS::Security::PrivacyComputation
