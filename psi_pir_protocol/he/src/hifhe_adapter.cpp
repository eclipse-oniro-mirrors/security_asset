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

#include "hifhe_adapter.h"
#include "crypt_eal_init.h"
#include "psi_pir_error_code.h"
#include "psi_pir_log.h"
#include <algorithm>
#include <utility>

namespace psi_pir::internal {
namespace {
constexpr uint32_t MAX_HE_BYTES = 256 * 1024;

} // namespace

HifheText::~HifheText()
{
    HIFHE_HETextFree(value);
}

int32_t HifheAdapter::Serialize(const HifheText &ciphertext, Bytes &output) const noexcept
{
    if (ctx_ == nullptr) {
        return PSI_PIR_HE_ERROR;
    }
    uint32_t size = 0;
    auto ret = HIFHE_Serialize(ctx_, ciphertext.value, nullptr, &size);
    if (ret != HIFHE_BUFFER_TOO_SMALL || size == 0 || size > MAX_HE_BYTES) {
        PSI_PIR_LOGE("Query HiFHE serialized length failed, ret=%{public}d, size=%{public}u", ret, size);
        return PSI_PIR_HE_ERROR;
    }
    Bytes buffer(size);
    ret = HIFHE_Serialize(ctx_, ciphertext.value, buffer.data(), &size);
    if (ret != HIFHE_SUCCESS || size == 0 || size > buffer.size()) {
        PSI_PIR_LOGE("Serialize HiFHE object failed, ret=%{public}d, size=%{public}u", ret, size);
        return PSI_PIR_HE_ERROR;
    }
    buffer.resize(size);
    output = std::move(buffer);
    PSI_PIR_LOGI("HPIC HiFHE Serialize success, bytes=%{public}zu", output.size());
    return PSI_PIR_SUCCESS;
}

int32_t HifheAdapter::Deserialize(const Bytes &ciphertext, HifheText &output) noexcept
{
    if (ctx_ == nullptr) {
        return PSI_PIR_HE_ERROR;
    }
    if (ciphertext.empty() || ciphertext.size() > MAX_HE_BYTES) {
        return PSI_PIR_INVALID_ARGUMENT;
    }
    HifheText result;
    const auto ret =
        HIFHE_Deserialize(ctx_, ciphertext.data(), static_cast<uint32_t>(ciphertext.size()), &result.value);
    if (ret != HIFHE_SUCCESS || result.value == nullptr) {
        PSI_PIR_LOGE("Deserialize HiFHE object failed, ret=%{public}d", ret);
        return PSI_PIR_HE_ERROR;
    }
    std::swap(output.value, result.value);
    PSI_PIR_LOGI("HPIC HiFHE Deserialize success, bytes=%{public}zu", ciphertext.size());
    return PSI_PIR_SUCCESS;
}

HifheAdapter::~HifheAdapter()
{
    Reset();
}

void HifheAdapter::Reset() noexcept
{
    HIFHE_Free(ctx_);
    ctx_ = nullptr;
}

int32_t HifheAdapter::ConfigureContext() noexcept
{
    uint32_t useLutCache = 1;
    auto ret = HIFHE_Ctrl(ctx_, HIFHE_CTRL_BGV_SET_LUT_CACHE, nullptr, &useLutCache, sizeof(useLutCache));
    if (ret == HIFHE_SUCCESS) {
        ret = HIFHE_SetParaById(ctx_, HIFHE_BGV_2048_48_17BIT);
    }
    uint32_t degree = 0;
    if (ret == HIFHE_SUCCESS) {
        ret = HIFHE_Ctrl(ctx_, HIFHE_CTRL_BGV_GET_POLY_DEGREE, nullptr, &degree, sizeof(degree));
    }
    if (ret != HIFHE_SUCCESS || degree != kDefaultPolyModulusDegree) {
        PSI_PIR_LOGE("Configure HiFHE parameters failed, ret=%{public}d, degree=%{public}u", ret, degree);
        return PSI_PIR_HE_ERROR;
    }
    PSI_PIR_LOGI("HPIC HiFHE parameters configured, degree=%{public}u", degree);
    return PSI_PIR_SUCCESS;
}

int32_t HifheAdapter::InitializeEvaluation(const ProtocolParams &params) noexcept
{
    if (ctx_ != nullptr || params.fheAlgorithm != FheAlgorithm::BGV2048_48 ||
        params.polyModulusDegree != kDefaultPolyModulusDegree || params.plainModulus != kDefaultPlainModulus) {
        PSI_PIR_LOGE("Invalid HiFHE protocol parameters or repeated initialization");
        return PSI_PIR_INVALID_ARGUMENT;
    }
    PSI_PIR_LOGI("HPIC HiFHE context initialize begin, degree=%{public}zu", params.polyModulusDegree);
    // Shared random facilities live for the process; an adapter must not globally clean them up.
    static const auto initRet = CRYPT_EAL_Init(CRYPT_EAL_INIT_CPU | CRYPT_EAL_INIT_BSL | CRYPT_EAL_INIT_RAND);
    if (initRet != HIFHE_SUCCESS) {
        PSI_PIR_LOGE("Initialize openHiTLS failed, ret=%{public}d", initRet);
        return PSI_PIR_HE_ERROR;
    }
    ctx_ = HIFHE_New(HIFHE_BGV);
    if (ctx_ == nullptr) {
        PSI_PIR_LOGE("Create native HiFHE BGV context failed");
        return PSI_PIR_HE_ERROR;
    }
    const auto ret = ConfigureContext();
    if (ret != PSI_PIR_SUCCESS) {
        Reset();
        return ret;
    }
    PSI_PIR_LOGI("HPIC HiFHE evaluation context initialized");
    return PSI_PIR_SUCCESS;
}

int32_t HifheAdapter::InitializeKeyed(const ProtocolParams &params, const HeSeed &seed) noexcept
{
    auto ret = InitializeEvaluation(params);
    if (ret != PSI_PIR_SUCCESS) {
        return ret;
    }
    ret = HIFHE_Ctrl(ctx_, HIFHE_CTRL_BGV_SET_SEED, nullptr, const_cast<uint8_t *>(seed.bytes.data()),
                     static_cast<uint32_t>(seed.bytes.size()));
    if (ret == HIFHE_SUCCESS) {
        PSI_PIR_LOGI("HPIC HiFHE seed configured, key generation begin");
        ret = HIFHE_KeyGen(ctx_);
    }
    if (ret != HIFHE_SUCCESS) {
        PSI_PIR_LOGE("Generate native HiFHE key from seed failed, ret=%{public}d", ret);
        Reset();
        return PSI_PIR_HE_ERROR;
    }
    PSI_PIR_LOGI("HPIC HiFHE key generation success");
    return PSI_PIR_SUCCESS;
}

int32_t HifheAdapter::Encode(const Slots &slots, HifheText &output) noexcept
{
    if (ctx_ == nullptr) {
        return PSI_PIR_HE_ERROR;
    }
    if (slots.size() != kDefaultPolyModulusDegree ||
        std::any_of(slots.begin(), slots.end(), [](uint64_t slot) { return slot >= kDefaultPlainModulus; })) {
        PSI_PIR_LOGE("Invalid HiFHE slot count or slot value");
        return PSI_PIR_INVALID_ARGUMENT;
    }
    HifheText result;
    const auto ret = HIFHE_Encode(ctx_, slots.data(), static_cast<uint32_t>(slots.size()), &result.value);
    if (ret != HIFHE_SUCCESS || result.value == nullptr) {
        PSI_PIR_LOGE("HIFHE_Encode failed, ret=%{public}d", ret);
        return PSI_PIR_HE_ERROR;
    }
    std::swap(output.value, result.value);
    PSI_PIR_LOGI("HPIC HiFHE Encode success, slots=%{public}zu", slots.size());
    return PSI_PIR_SUCCESS;
}

int32_t HifheAdapter::Decode(const HifheText &plaintext, Slots &output) noexcept
{
    if (ctx_ == nullptr) {
        return PSI_PIR_HE_ERROR;
    }
    Slots values(kDefaultPolyModulusDegree);
    uint32_t count = static_cast<uint32_t>(values.size());
    const auto ret = HIFHE_Decode(ctx_, plaintext.value, values.data(), &count);
    if (ret != HIFHE_SUCCESS || count != values.size() ||
        std::any_of(values.begin(), values.end(), [](uint64_t slot) { return slot >= kDefaultPlainModulus; })) {
        PSI_PIR_LOGE("HIFHE_Decode failed, ret=%{public}d, count=%{public}u", ret, count);
        return PSI_PIR_HE_ERROR;
    }
    output = std::move(values);
    PSI_PIR_LOGI("HPIC HiFHE Decode success, slots=%{public}zu", output.size());
    return PSI_PIR_SUCCESS;
}

int32_t HifheAdapter::Encrypt(const HifheText &plaintext, HifheText &output) noexcept
{
    if (ctx_ == nullptr) {
        return PSI_PIR_HE_ERROR;
    }
    HifheText result;
    const auto ret = HIFHE_Encrypt(ctx_, plaintext.value, &result.value);
    if (ret != HIFHE_SUCCESS || result.value == nullptr) {
        PSI_PIR_LOGE("HIFHE_Encrypt failed, ret=%{public}d", ret);
        return PSI_PIR_HE_ERROR;
    }
    std::swap(output.value, result.value);
    PSI_PIR_LOGI("HPIC HiFHE Encrypt success");
    return PSI_PIR_SUCCESS;
}

int32_t HifheAdapter::Decrypt(const HifheText &ciphertext, HifheText &output) noexcept
{
    if (ctx_ == nullptr) {
        return PSI_PIR_HE_ERROR;
    }
    HifheText result;
    const auto ret = HIFHE_Decrypt(ctx_, ciphertext.value, &result.value);
    if (ret != HIFHE_SUCCESS || result.value == nullptr) {
        PSI_PIR_LOGE("HIFHE_Decrypt failed, ret=%{public}d", ret);
        return PSI_PIR_HE_ERROR;
    }
    std::swap(output.value, result.value);
    PSI_PIR_LOGI("HPIC HiFHE Decrypt success");
    return PSI_PIR_SUCCESS;
}

int32_t HifheAdapter::SubtractFromPlaintext(HifheText &ciphertext, const HifheText &plaintext,
                                            HifheText &output) noexcept
{
    if (ctx_ == nullptr) {
        return PSI_PIR_HE_ERROR;
    }
    auto ret = HIFHE_Ctrl(ctx_, HIFHE_CTRL_BGV_SET_NEGATE, ciphertext.value, nullptr, 0);
    if (ret != HIFHE_SUCCESS) {
        PSI_PIR_LOGE("Set native HiFHE negate flag failed, ret=%{public}d", ret);
        return PSI_PIR_HE_ERROR;
    }
    HifheText result;
    ret = HIFHE_Add(ctx_, ciphertext.value, plaintext.value, &result.value);
    if (ret != HIFHE_SUCCESS || result.value == nullptr) {
        PSI_PIR_LOGE("HIFHE_Add for PT-CT failed, ret=%{public}d", ret);
        return PSI_PIR_HE_ERROR;
    }
    std::swap(output.value, result.value);
    PSI_PIR_LOGI("HPIC HiFHE AddPlain success");
    return PSI_PIR_SUCCESS;
}

int32_t HifheAdapter::MultiplyPlaintext(const HifheText &ciphertext, const HifheText &plaintext,
                                        HifheText &output) noexcept
{
    if (ctx_ == nullptr) {
        return PSI_PIR_HE_ERROR;
    }
    HifheText result;
    const auto ret = HIFHE_Mul(ctx_, ciphertext.value, plaintext.value, &result.value);
    if (ret != HIFHE_SUCCESS || result.value == nullptr) {
        PSI_PIR_LOGE("HIFHE_Mul failed, ret=%{public}d", ret);
        return PSI_PIR_HE_ERROR;
    }
    std::swap(output.value, result.value);
    PSI_PIR_LOGI("HPIC HiFHE MultiplyPlain success");
    return PSI_PIR_SUCCESS;
}

int32_t HifheAdapter::Rerandomize(const HifheText &ciphertext, HifheText &output) noexcept
{
    if (ctx_ == nullptr) {
        return PSI_PIR_HE_ERROR;
    }
    HifheText result;
    const auto ret =
        HIFHE_Ctrl(ctx_, HIFHE_CTRL_BGV_RERANDOMIZE, ciphertext.value, &result.value, sizeof(result.value));
    if (ret != HIFHE_SUCCESS || result.value == nullptr) {
        PSI_PIR_LOGE("Native HiFHE rerandomization failed, ret=%{public}d", ret);
        return PSI_PIR_HE_ERROR;
    }
    std::swap(output.value, result.value);
    PSI_PIR_LOGI("HPIC HiFHE Rerandomize success");
    return PSI_PIR_SUCCESS;
}
} // namespace psi_pir::internal
