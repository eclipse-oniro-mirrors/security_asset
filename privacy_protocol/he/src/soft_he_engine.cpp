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

#include "soft_he_engine.h"
#include "hifhe_adapter.h"
#include "protocol_mapper.h"
#include "provider_registry.h"
#include "privacy_error_code.h"
#include "privacy_log.h"
#include <new>
#include <utility>

namespace OHOS::Security::PrivacyComputation::internal {
namespace {
constexpr size_t PIR128_COMPARE_BITS = 128;
constexpr size_t PIR128_SLOTS_PER_DATA = 8;
constexpr size_t PIR128_DATA_PER_PLAINTEXT = 256;
} // namespace
bool SoftHeEngine::IsCompatible(const ProtocolParams &params) const noexcept
{
    const bool layout =
        (params.compareBits == kDefaultCompareBits && params.slotsPerData == kDefaultSlotsPerData &&
         params.dataNumPerPlaintext == kDefaultDataNumberPerPlaintext &&
         (params.mode == ProtocolMode::INTERSECTION || params.mode == ProtocolMode::PIR)) ||
        (params.mode == ProtocolMode::PIR && params.compareBits == PIR128_COMPARE_BITS &&
         params.slotsPerData == PIR128_SLOTS_PER_DATA && params.dataNumPerPlaintext == PIR128_DATA_PER_PLAINTEXT);
    return layout && params.fheAlgorithm == FheAlgorithm::BGV2048_48 && params.bitsPerSlot == kDefaultBitsPerSlot &&
           params.plainModulus == kDefaultPlainModulus && params.polyModulusDegree == kDefaultPolyModulusDegree;
}

int32_t SoftHeEngine::Encrypt(const Slots &slots, const ProtocolParams &params, const HeSeed &seed,
                              Bytes &output) const noexcept
{
    HifheAdapter adapter;
    auto ret = adapter.InitializeKeyed(params, seed);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        return ret;
    }
    HifheText plaintext;
    ret = adapter.Encode(slots, plaintext);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        return ret;
    }
    HifheText ciphertext;
    ret = adapter.Encrypt(plaintext, ciphertext);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        return ret;
    }
    return adapter.Serialize(ciphertext, output);
}

int32_t SoftHeEngine::Decrypt(const Bytes &ciphertext, const ProtocolParams &params, const HeSeed &seed,
                              Slots &output) const noexcept
{
    if (ciphertext.empty()) {
        return PRIVACY_PROTOCOL_INVALID_ARGUMENT;
    }
    HifheAdapter adapter;
    auto ret = adapter.InitializeKeyed(params, seed);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        return ret;
    }
    HifheText encrypted;
    ret = adapter.Deserialize(ciphertext, encrypted);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        return PRIVACY_PROTOCOL_CRYPTO_ERROR;
    }
    HifheText plaintext;
    ret = adapter.Decrypt(encrypted, plaintext);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        return PRIVACY_PROTOCOL_CRYPTO_ERROR;
    }
    ret = adapter.Decode(plaintext, output);
    return ret == PRIVACY_PROTOCOL_SUCCESS ? ret : PRIVACY_PROTOCOL_CRYPTO_ERROR;
}

namespace {
class HifheEvaluation final : public HeEvaluation {
public:
    int32_t Initialize(const Bytes &ciphertext) noexcept
    {
        status_ = adapter_.InitializeEvaluation(ProtocolParams{});
        if (status_ == PRIVACY_PROTOCOL_SUCCESS) {
            status_ = adapter_.Deserialize(ciphertext, current_);
        }
        return status_;
    }

    int32_t AddPlain(const Slots &slots) noexcept override;

    int32_t MultiplyPlain(const Slots &slots) noexcept override;

    int32_t Rerandomize() noexcept override
    {
        if (status_ == PRIVACY_PROTOCOL_SUCCESS) {
            status_ = adapter_.Rerandomize(current_, current_);
        }
        return status_;
    }

    int32_t Serialize(Bytes &output) const noexcept override
    {
        return status_ == PRIVACY_PROTOCOL_SUCCESS ? adapter_.Serialize(current_, output) : status_;
    }

private:
    HifheAdapter adapter_;
    HifheText current_;
    int32_t status_ = PRIVACY_PROTOCOL_HE_ERROR;
};

int32_t HifheEvaluation::AddPlain(const Slots &slots) noexcept
{
    if (status_ != PRIVACY_PROTOCOL_SUCCESS) {
        return status_;
    }
    HifheText plaintext;
    status_ = adapter_.Encode(slots, plaintext);
    if (status_ == PRIVACY_PROTOCOL_SUCCESS) {
        status_ = adapter_.SubtractFromPlaintext(current_, plaintext, current_);
    }
    return status_;
}

int32_t HifheEvaluation::MultiplyPlain(const Slots &slots) noexcept
{
    if (status_ != PRIVACY_PROTOCOL_SUCCESS) {
        return status_;
    }
    HifheText plaintext;
    status_ = adapter_.Encode(slots, plaintext);
    if (status_ == PRIVACY_PROTOCOL_SUCCESS) {
        status_ = adapter_.MultiplyPlaintext(current_, plaintext, current_);
    }
    return status_;
}

} // namespace

int32_t SoftHeEngine::CreateEvaluation(const Bytes &ciphertext,
                                       std::unique_ptr<HeEvaluation> &evaluation) const noexcept
{
    if (ciphertext.empty()) {
        return PRIVACY_PROTOCOL_INVALID_ARGUMENT;
    }
    auto result = std::unique_ptr<HifheEvaluation>(new (std::nothrow) HifheEvaluation());
    if (!result) {
        LOGE("Allocate HiFHE evaluation failed");
        return PRIVACY_PROTOCOL_INTERNAL_ERROR;
    }
    const auto ret = result->Initialize(ciphertext);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        LOGE("Import HiFHE evaluation ciphertext failed, ret=%{public}d", ret);
        return ret;
    }
    evaluation = std::move(result);
    return PRIVACY_PROTOCOL_SUCCESS;
}

std::shared_ptr<const IHEEngine> GetSoftHeEngine() noexcept
{
    static const auto engine = []() -> std::shared_ptr<const SoftHeEngine> {
        auto *instance = new (std::nothrow) const SoftHeEngine();
        if (instance == nullptr) {
            LOGE("Allocate default HE engine failed");
            return nullptr;
        }
        return std::shared_ptr<const SoftHeEngine>(instance);
    }();
    return engine;
}

int32_t EnsureDefaultHeEngineInstalled() noexcept
{
    return InstallDefaultHEEngineFactory(GetSoftHeEngine)
               ? PRIVACY_PROTOCOL_SUCCESS
               : PRIVACY_PROTOCOL_INTERNAL_ERROR;
}
} // namespace OHOS::Security::PrivacyComputation::internal
