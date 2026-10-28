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

// Native objects are local to an operation and always freed, including partially failed calls.
struct HifheText final {
    HifheText() = default;
    ~HifheText()
    {
        HIFHE_HETextFree(value);
    }
    HifheText(const HifheText &) = delete;
    HifheText &operator=(const HifheText &) = delete;
    HifheText(HifheText &&) = delete;
    HifheText &operator=(HifheText &&) = delete;
    HIFHE_HEText *value = nullptr;
};

int32_t SerializeText(const HIFHE_Ctx *ctx, const HifheText &text, Bytes &output)
{
    uint32_t size = 0;
    auto ret = HIFHE_Serialize(ctx, text.value, nullptr, &size);
    if (ret != HIFHE_BUFFER_TOO_SMALL || size == 0 || size > MAX_HE_BYTES) {
        PSI_PIR_LOGE("Query HiFHE serialized length failed, ret=%{public}d, size=%{public}u", ret, size);
        return PSI_PIR_HE_ERROR;
    }
    Bytes buffer(size);
    ret = HIFHE_Serialize(ctx, text.value, buffer.data(), &size);
    if (ret != HIFHE_SUCCESS || size == 0 || size > buffer.size()) {
        PSI_PIR_LOGE("Serialize HiFHE object failed, ret=%{public}d, size=%{public}u", ret, size);
        return PSI_PIR_HE_ERROR;
    }
    buffer.resize(size);
    output = std::move(buffer);
    return PSI_PIR_SUCCESS;
}

int32_t DeserializeText(const HIFHE_Ctx *ctx, const Bytes &input, HifheText &output)
{
    if (ctx == nullptr) {
        return PSI_PIR_HE_ERROR;
    }
    if (input.empty() || input.size() > MAX_HE_BYTES) {
        return PSI_PIR_INVALID_ARGUMENT;
    }
    const auto ret = HIFHE_Deserialize(ctx, input.data(), static_cast<uint32_t>(input.size()), &output.value);
    if (ret != HIFHE_SUCCESS || output.value == nullptr) {
        PSI_PIR_LOGE("Deserialize HiFHE object failed, ret=%{public}d", ret);
        return PSI_PIR_HE_ERROR;
    }
    return PSI_PIR_SUCCESS;
}
} // namespace

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
    uint32_t plaintextBytes = 0;
    uint32_t ciphertextBytes = 0;
    if (ret == HIFHE_SUCCESS) {
        ret = HIFHE_Ctrl(ctx_, HIFHE_CTRL_BGV_GET_POLY_DEGREE, nullptr, &degree, sizeof(degree));
    }
    if (ret == HIFHE_SUCCESS) {
        ret = HIFHE_Ctrl(ctx_, HIFHE_CTRL_BGV_GET_PLAINTEXT_LEN, nullptr, &plaintextBytes, sizeof(plaintextBytes));
    }
    if (ret == HIFHE_SUCCESS) {
        ret = HIFHE_Ctrl(ctx_, HIFHE_CTRL_BGV_GET_CIPHERTEXT_LEN, nullptr, &ciphertextBytes, sizeof(ciphertextBytes));
    }
    if (ret != HIFHE_SUCCESS || degree != kDefaultPolyModulusDegree || plaintextBytes == 0 ||
        plaintextBytes > MAX_HE_BYTES || ciphertextBytes == 0 || ciphertextBytes > MAX_HE_BYTES) {
        PSI_PIR_LOGE("Configure native HiFHE parameters failed, ret=%{public}d, degree=%{public}u", ret, degree);
        return PSI_PIR_HE_ERROR;
    }
    return PSI_PIR_SUCCESS;
}

int32_t HifheAdapter::InitializeEvaluation(const ProtocolParams &params) noexcept
{
    if (ctx_ != nullptr || params.fheAlgorithm != FheAlgorithm::BGV2048_48 ||
        params.polyModulusDegree != kDefaultPolyModulusDegree || params.plainModulus != kDefaultPlainModulus) {
        PSI_PIR_LOGE("Invalid HiFHE protocol parameters or repeated initialization");
        return PSI_PIR_INVALID_ARGUMENT;
    }
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
        ret = HIFHE_KeyGen(ctx_);
    }
    if (ret != HIFHE_SUCCESS) {
        PSI_PIR_LOGE("Generate native HiFHE key from seed failed, ret=%{public}d", ret);
        Reset();
        return PSI_PIR_HE_ERROR;
    }
    return PSI_PIR_SUCCESS;
}

int32_t HifheAdapter::Encode(const Slots &slots, Bytes &output) noexcept
{
    if (ctx_ == nullptr) {
        return PSI_PIR_HE_ERROR;
    }
    if (slots.size() != kDefaultPolyModulusDegree ||
        std::any_of(slots.begin(), slots.end(), [](uint64_t slot) { return slot >= kDefaultPlainModulus; })) {
        PSI_PIR_LOGE("Invalid HiFHE slot count or slot value");
        return PSI_PIR_INVALID_ARGUMENT;
    }
    // The protocol uses uint64 slots; the native API requires checked uint32 values.
    const std::vector<uint32_t> values(slots.begin(), slots.end());
    HifheText plaintext;
    const auto ret = HIFHE_Encode(ctx_, values.data(), static_cast<uint32_t>(values.size()), &plaintext.value);
    if (ret != HIFHE_SUCCESS) {
        PSI_PIR_LOGE("HIFHE_Encode failed, ret=%{public}d", ret);
        return PSI_PIR_HE_ERROR;
    }
    return SerializeText(ctx_, plaintext, output);
}

int32_t HifheAdapter::Decode(const Bytes &plaintext, Slots &output) noexcept
{
    HifheText decoded;
    auto ret = DeserializeText(ctx_, plaintext, decoded);
    if (ret != PSI_PIR_SUCCESS) {
        return ret;
    }
    std::vector<uint32_t> values(kDefaultPolyModulusDegree);
    uint32_t count = static_cast<uint32_t>(values.size());
    ret = HIFHE_Decode(ctx_, decoded.value, values.data(), &count);
    if (ret != HIFHE_SUCCESS || count != values.size() ||
        std::any_of(values.begin(), values.end(), [](uint32_t slot) { return slot >= kDefaultPlainModulus; })) {
        PSI_PIR_LOGE("HIFHE_Decode failed, ret=%{public}d, count=%{public}u", ret, count);
        return PSI_PIR_HE_ERROR;
    }
    Slots result(values.begin(), values.end());
    output = std::move(result);
    return PSI_PIR_SUCCESS;
}

int32_t HifheAdapter::Encrypt(const Bytes &plaintext, Bytes &output) noexcept
{
    HifheText plain;
    auto ret = DeserializeText(ctx_, plaintext, plain);
    if (ret != PSI_PIR_SUCCESS) {
        return ret;
    }
    HifheText ciphertext;
    ret = HIFHE_Encrypt(ctx_, plain.value, &ciphertext.value);
    if (ret != HIFHE_SUCCESS) {
        PSI_PIR_LOGE("HIFHE_Encrypt failed, ret=%{public}d", ret);
        return PSI_PIR_HE_ERROR;
    }
    return SerializeText(ctx_, ciphertext, output);
}

int32_t HifheAdapter::Decrypt(const Bytes &ciphertext, Bytes &output) noexcept
{
    HifheText cipher;
    auto ret = DeserializeText(ctx_, ciphertext, cipher);
    if (ret != PSI_PIR_SUCCESS) {
        return ret;
    }
    HifheText plaintext;
    ret = HIFHE_Decrypt(ctx_, cipher.value, &plaintext.value);
    if (ret != HIFHE_SUCCESS) {
        PSI_PIR_LOGE("HIFHE_Decrypt failed, ret=%{public}d", ret);
        return PSI_PIR_HE_ERROR;
    }
    return SerializeText(ctx_, plaintext, output);
}

int32_t HifheAdapter::SubtractFromPlaintext(const Bytes &ciphertext, const Bytes &plaintext, Bytes &output) noexcept
{
    HifheText cipher;
    HifheText plain;
    auto ret = DeserializeText(ctx_, ciphertext, cipher);
    if (ret != PSI_PIR_SUCCESS) {
        return ret;
    }
    ret = DeserializeText(ctx_, plaintext, plain);
    if (ret != PSI_PIR_SUCCESS) {
        return ret;
    }
    ret = HIFHE_Ctrl(ctx_, HIFHE_CTRL_BGV_SET_NEGATE, cipher.value, nullptr, 0);
    if (ret != HIFHE_SUCCESS) {
        PSI_PIR_LOGE("Set native HiFHE negate flag failed, ret=%{public}d", ret);
        return PSI_PIR_HE_ERROR;
    }
    // The native negate flag is consumed by Add and cannot be serialized by itself.
    HifheText result;
    ret = HIFHE_Add(ctx_, cipher.value, plain.value, &result.value);
    if (ret != HIFHE_SUCCESS) {
        PSI_PIR_LOGE("HIFHE_Add for PT-CT failed, ret=%{public}d", ret);
        return PSI_PIR_HE_ERROR;
    }
    return SerializeText(ctx_, result, output);
}

int32_t HifheAdapter::AddPlaintext(const Bytes &ciphertext, const Bytes &plaintext, Bytes &output) noexcept
{
    HifheText cipher;
    HifheText plain;
    auto ret = DeserializeText(ctx_, ciphertext, cipher);
    if (ret != PSI_PIR_SUCCESS) {
        return ret;
    }
    ret = DeserializeText(ctx_, plaintext, plain);
    if (ret != PSI_PIR_SUCCESS) {
        return ret;
    }
    HifheText result;
    ret = HIFHE_Add(ctx_, cipher.value, plain.value, &result.value);
    if (ret != HIFHE_SUCCESS) {
        PSI_PIR_LOGE("HIFHE_Add failed, ret=%{public}d", ret);
        return PSI_PIR_HE_ERROR;
    }
    return SerializeText(ctx_, result, output);
}

int32_t HifheAdapter::MultiplyPlaintext(const Bytes &ciphertext, const Bytes &plaintext, Bytes &output) noexcept
{
    HifheText cipher;
    HifheText plain;
    auto ret = DeserializeText(ctx_, ciphertext, cipher);
    if (ret != PSI_PIR_SUCCESS) {
        return ret;
    }
    ret = DeserializeText(ctx_, plaintext, plain);
    if (ret != PSI_PIR_SUCCESS) {
        return ret;
    }
    HifheText result;
    ret = HIFHE_Mul(ctx_, cipher.value, plain.value, &result.value);
    if (ret != HIFHE_SUCCESS) {
        PSI_PIR_LOGE("HIFHE_Mul failed, ret=%{public}d", ret);
        return PSI_PIR_HE_ERROR;
    }
    return SerializeText(ctx_, result, output);
}

int32_t HifheAdapter::Rerandomize(const Bytes &ciphertext, Bytes &output) noexcept
{
    HifheText cipher;
    auto ret = DeserializeText(ctx_, ciphertext, cipher);
    if (ret != PSI_PIR_SUCCESS) {
        return ret;
    }
    HifheText result;
    ret = HIFHE_Ctrl(ctx_, HIFHE_CTRL_BGV_RERANDOMIZE, cipher.value, &result.value, sizeof(result.value));
    if (ret != HIFHE_SUCCESS) {
        PSI_PIR_LOGE("Native HiFHE rerandomization failed, ret=%{public}d", ret);
        return PSI_PIR_HE_ERROR;
    }
    return SerializeText(ctx_, result, output);
}
} // namespace psi_pir::internal
