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
#include "psi_pir_error_code.h"
#include <utility>

namespace psi_pir::internal {
namespace {
constexpr size_t PIR128_COMPARE_BITS = 128;
constexpr size_t PIR128_SLOTS_PER_DATA = 8;
constexpr size_t PIR128_DATA_PER_PLAINTEXT = 256;
} // namespace
bool SoftHeEngine::IsCompatible(const ProtocolParams &params) const noexcept
{
    using namespace psi_pir::internal;
    const bool layout =
        (params.compareBits == kDefaultCompareBits && params.slotsPerData == kDefaultSlotsPerData &&
         params.dataNumPerPlaintext == kDefaultDataNumberPerPlaintext &&
         (params.mode == ProtocolMode::PSI || params.mode == ProtocolMode::PIR)) ||
        (params.mode == ProtocolMode::PIR && params.compareBits == PIR128_COMPARE_BITS &&
         params.slotsPerData == PIR128_SLOTS_PER_DATA && params.dataNumPerPlaintext == PIR128_DATA_PER_PLAINTEXT);
    return layout && params.fheAlgorithm == FheAlgorithm::BGV2048_48 && params.bitsPerSlot == kDefaultBitsPerSlot &&
           params.plainModulus == kDefaultPlainModulus && params.polyModulusDegree == kDefaultPolyModulusDegree;
}

namespace {
ProtocolParams EvaluationParams()
{
    return ProtocolParams{};
}
} // namespace

int32_t SoftHeEngine::addPlain(const Bytes &ciphertext, const Slots &plaintext, Bytes &output) const noexcept
{
    HifheAdapter adapter;
    auto ret = adapter.InitializeEvaluation(EvaluationParams());
    if (ret != psi_pir::PSI_PIR_SUCCESS) {
        return ret;
    }
    Bytes encoded;
    Bytes negative;
    Bytes result;
    ret = adapter.Encode(plaintext, encoded);
    if (ret != psi_pir::PSI_PIR_SUCCESS) {
        return ret;
    }
    ret = adapter.NegateCiphertext(ciphertext, negative);
    if (ret != psi_pir::PSI_PIR_SUCCESS) {
        return ret;
    }
    ret = adapter.AddPlaintext(negative, encoded, result);
    if (ret == psi_pir::PSI_PIR_SUCCESS) {
        output = std::move(result);
    }
    return ret;
}

int32_t SoftHeEngine::multiplyPlain(const Bytes &ciphertext, const Slots &plaintext, Bytes &output) const noexcept
{
    HifheAdapter adapter;
    auto ret = adapter.InitializeEvaluation(EvaluationParams());
    if (ret != psi_pir::PSI_PIR_SUCCESS) {
        return ret;
    }
    Bytes encoded;
    Bytes result;
    ret = adapter.Encode(plaintext, encoded);
    if (ret != psi_pir::PSI_PIR_SUCCESS) {
        return ret;
    }
    ret = adapter.MultiplyPlaintext(ciphertext, encoded, result);
    if (ret == psi_pir::PSI_PIR_SUCCESS) {
        output = std::move(result);
    }
    return ret;
}

int32_t SoftHeEngine::RandCipherText(const Bytes &ciphertext, Bytes &output) const noexcept
{
    HifheAdapter adapter;
    const auto ret = adapter.InitializeEvaluation(EvaluationParams());
    if (ret != psi_pir::PSI_PIR_SUCCESS) {
        return ret;
    }
    Bytes result;
    const auto operationRet = adapter.Rerandomize(ciphertext, result);
    if (operationRet == psi_pir::PSI_PIR_SUCCESS) {
        output = std::move(result);
    }
    return operationRet;
}

std::shared_ptr<const psi_pir::internal::IHEEngine> GetSoftHeEngine() noexcept
{
    static const auto engine = std::make_shared<const SoftHeEngine>();
    return engine;
}

int32_t EnsureDefaultHeEngineInstalled() noexcept
{
    return psi_pir::internal::InstallDefaultHEEngineFactory(GetSoftHeEngine) ? psi_pir::PSI_PIR_SUCCESS
                                                                             : psi_pir::PSI_PIR_INTERNAL_ERROR;
}
} // namespace psi_pir::internal
