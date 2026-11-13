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

#include "privacy_protocol_impl.h"

#include "he_key_deriver.h"
#include "protocol_evaluation.h"
#include "provider_registry.h"
#include "privacy_log.h"
#include "soft_he_engine.h"
#include "securec.h"

#include <algorithm>
#include <limits>
#include <utility>

namespace OHOS::Security::PrivacyComputation::internal {
namespace {

struct BatchLayout {
    std::size_t capacity{0};
    std::size_t batchCount{0};
};

struct PirPayload {
    std::vector<Bytes> seeds;
    std::vector<Bytes> ciphertexts;
};

struct EncryptionContext {
    const ProtocolParams &params;
    const ProtocolEncoder &encoder;
    const IPirValueCrypto &pirValueCrypto;
};

struct EvaluationContext {
    const Bytes &privacyTarget;
    const ProtocolParams &params;
    const ProtocolEncoder &encoder;
    const IHEEngine &engine;
    const std::vector<Slots> &serverVectors;
    const PirPayload &payload;
};

struct SearchResultContext {
    const std::optional<std::vector<Bytes>> &valueCipherText;
    const ProtocolParams &params;
    const ProtocolEncoder &encoder;
    std::size_t groupsPerItem;
    std::size_t capacity;
};

bool IsPir(const ProtocolParams &params) noexcept
{
    return params.mode == ProtocolMode::PIR;
}

std::size_t GetGroupsPerItem(const ProtocolParams &params) noexcept
{
    return IsPir(params) ? kPirGroupsPerItem : kIntersectionGroupsPerItem;
}

int32_t ValidateElements(const std::vector<Element> &elements, const ProtocolEncoder &encoder) noexcept
{
    for (std::size_t index = 0; index < elements.size(); ++index) {
        const auto &element = elements[index];
        const auto ret = encoder.ValidateHashAlgorithm(element.hashAlg);
        if (ret != PRIVACY_PROTOCOL_SUCCESS) {
            LOGE("element validation failed: index=%{private}zu, hashAlg=%{public}u, "
                 "ret=%{public}d",
                 index, static_cast<uint32_t>(element.hashAlg), ret);
            return ret;
        }
    }
    return PRIVACY_PROTOCOL_SUCCESS;
}

int32_t ValidateEvaluateInput(const Bytes &privacyTarget, const std::vector<Element> &elements,
                              const ProtocolParams &params, const ProtocolCodec &codec,
                              const ProtocolEncoder &encoder) noexcept
{
    auto ret = codec.ValidateProtocol(params);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        LOGE("ValidateProtocol failed, ret=%{public}d", ret);
        return ret;
    }
    ret = codec.ValidatePrivacyTarget(privacyTarget);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        LOGE("ValidatePrivacyTarget failed, ret=%{public}d", ret);
        return ret;
    }
    return ValidateElements(elements, encoder);
}

void SetEmptyEvaluationResult(const ProtocolParams &params, PrivacySearchResult &searchResult)
{
    LOGI("Evaluate completed with an empty element set, mode=%{public}u", static_cast<uint32_t>(params.mode));
    PrivacySearchResult emptyResult;
    if (IsPir(params)) {
        emptyResult.valueCipherText.emplace();
    }
    searchResult = std::move(emptyResult);
}

int32_t GetCompatibleEngine(const ProtocolParams &params, std::shared_ptr<const IHEEngine> &engine) noexcept
{
    engine = GetHEEngine();
    if (!engine) {
        LOGE("HE engine not registered");
        return PRIVACY_PROTOCOL_HE_ERROR;
    }
    if (!engine->IsCompatible(params)) {
        LOGE("HE engine incompatible: degree=%{public}zu, modulus=%{public}llu", params.polyModulusDegree,
             static_cast<unsigned long long>(params.plainModulus));
        return PRIVACY_PROTOCOL_INCOMPATIBLE_HE_PARAMETERS;
    }
    LOGI("HE engine compatible, algorithm=%{public}u, degree=%{public}zu, "
         "modulus=%{public}llu",
         static_cast<uint32_t>(params.fheAlgorithm), params.polyModulusDegree,
         static_cast<unsigned long long>(params.plainModulus));
    return PRIVACY_PROTOCOL_SUCCESS;
}

int32_t EncodeServerVectors(const std::vector<Element> &elements, const ProtocolParams &params,
                            const ProtocolEncoder &encoder, std::vector<Slots> &serverVectors) noexcept
{
    serverVectors.reserve(elements.size());
    for (std::size_t i = 0; i < elements.size(); ++i) {
        Slots serverVector;
        const auto ret = encoder.EncodeServerVector(elements[i], params, serverVector);
        if (ret != PRIVACY_PROTOCOL_SUCCESS) {
            LOGE("EncodeServerVector failed at index=%{private}zu, ret=%{public}d", i, ret);
            return ret;
        }
        serverVectors.push_back(std::move(serverVector));
    }
    LOGI("server vector encoding complete, count=%{public}zu", serverVectors.size());
    return PRIVACY_PROTOCOL_SUCCESS;
}

int32_t EncryptElementValue(const Element &element, const EncryptionContext &context, Bytes &seed,
                            Bytes &ciphertext) noexcept
{
    auto ret = context.encoder.GenerateSeed(context.params, seed);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        LOGE("GenerateSeed failed, ret=%{public}d", ret);
        return ret;
    }
    ret = context.pirValueCrypto.Encrypt(seed, element.elemValue, context.params, ciphertext);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        LOGE("pirValueCrypto encrypt failed, ret=%{public}d", ret);
        return ret;
    }
    if (ciphertext.empty()) {
        LOGE("encrypted ciphertext is empty");
        return PRIVACY_PROTOCOL_CRYPTO_ERROR;
    }
    return PRIVACY_PROTOCOL_SUCCESS;
}

int32_t PreparePirPayload(const std::vector<Element> &elements, const ProtocolParams &params,
                          const ProtocolEncoder &encoder, PirPayload &payload) noexcept
{
    const auto pirValueCrypto = GetPirValueCrypto();
    if (!pirValueCrypto) {
        LOGE("pir value crypto not registered");
        return PRIVACY_PROTOCOL_CRYPTO_ERROR;
    }
    const EncryptionContext context{params, encoder, *pirValueCrypto};
    payload.seeds.reserve(elements.size());
    payload.ciphertexts.reserve(elements.size());
    for (std::size_t i = 0; i < elements.size(); ++i) {
        Bytes seed;
        Bytes ciphertext;
        const auto ret = EncryptElementValue(elements[i], context, seed, ciphertext);
        if (ret != PRIVACY_PROTOCOL_SUCCESS) {
            LOGE("EncryptElementValue failed at index=%{private}zu, ret=%{public}d", i, ret);
            return ret;
        }
        payload.seeds.push_back(std::move(seed));
        payload.ciphertexts.push_back(std::move(ciphertext));
    }
    LOGI("PIR payload prepared, elements=%{public}zu, symmetricCount=%{public}zu", payload.seeds.size(),
         payload.ciphertexts.size());
    return PRIVACY_PROTOCOL_SUCCESS;
}

int32_t PreparePayload(const std::vector<Element> &elements, const ProtocolParams &params,
                       const ProtocolEncoder &encoder, PirPayload &payload) noexcept
{
    if (!IsPir(params)) {
        return PRIVACY_PROTOCOL_SUCCESS;
    }
    return PreparePirPayload(elements, params, encoder, payload);
}

int32_t CalculateBatchLayout(const ProtocolParams &params, std::size_t elementCount, BatchLayout &layout) noexcept
{
    layout.capacity = params.dataNumPerPlaintext / GetGroupsPerItem(params);
    if (layout.capacity == 0 || elementCount > std::numeric_limits<std::size_t>::max() - (layout.capacity - 1)) {
        LOGE("invalid batch layout: capacity=%{public}zu, elementCount=%{public}zu", layout.capacity, elementCount);
        return PRIVACY_PROTOCOL_INVALID_ARGUMENT;
    }
    layout.batchCount = (elementCount + layout.capacity - 1) / layout.capacity;
    LOGI("batch layout: capacity=%{public}zu, batchCount=%{public}zu", layout.capacity, layout.batchCount);
    return PRIVACY_PROTOCOL_SUCCESS;
}

int32_t CopyServerVector(const EvaluationContext &context, const Slots &serverVector, std::size_t item,
                         Slots &differencePlain)
{
    const auto groupsPerItem = GetGroupsPerItem(context.params);
    for (std::size_t group = 0; group < groupsPerItem; ++group) {
        const auto offset = (item * groupsPerItem + group) * context.params.slotsPerData;
        if (offset > differencePlain.size() || serverVector.size() > differencePlain.size() - offset ||
            memcpy_s(differencePlain.data() + offset, (differencePlain.size() - offset) * sizeof(uint64_t),
                     serverVector.data(), serverVector.size() * sizeof(uint64_t)) != EOK) {
            LOGE("Failed to copy server vector, offset=%{public}zu", offset);
            return PRIVACY_PROTOCOL_CODEC_ERROR;
        }
    }
    return PRIVACY_PROTOCOL_SUCCESS;
}

int32_t BuildDifferencePlain(const EvaluationContext &context, std::size_t batchStart, std::size_t batchElementCount,
                             Slots &differencePlain)
{
    differencePlain.assign(context.params.dataNumPerPlaintext * context.params.slotsPerData, 0);
    for (std::size_t item = 0; item < batchElementCount; ++item) {
        const auto ret = CopyServerVector(context, context.serverVectors[batchStart + item], item, differencePlain);
        if (ret != PRIVACY_PROTOCOL_SUCCESS) {
            return ret;
        }
    }
    return PRIVACY_PROTOCOL_SUCCESS;
}

int32_t EvaluateDifference(const EvaluationContext &context, const Slots &differencePlain,
                           HeEvaluation &evaluation) noexcept
{
    auto ret = evaluation.AddPlain(differencePlain);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        LOGE("EvaluateDifference subtraction failed, ret=%{public}d", ret);
        return ret;
    }
    Slots mask;
    ret = context.encoder.GenerateMaskVector(context.params, mask);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        LOGE("GenerateMaskVector failed, ret=%{public}d", ret);
        return ret;
    }
    ret = evaluation.MultiplyPlain(mask);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        LOGE("EvaluateDifference mask multiplication failed, ret=%{public}d", ret);
    } else {
        LOGI("batch EvaluateDifference success");
    }
    return ret;
}

int32_t BuildSeedPlain(const EvaluationContext &context, std::size_t batchStart, std::size_t batchElementCount,
                       std::size_t slotCount, Slots &seedPlain) noexcept
{
    seedPlain.assign(slotCount, 0);
    for (std::size_t item = 0; item < batchElementCount; ++item) {
        Slots seedSlots;
        const auto ret =
            context.encoder.EncodeSeed(context.payload.seeds[batchStart + item], context.params, seedSlots);
        if (ret != PRIVACY_PROTOCOL_SUCCESS) {
            LOGE("EncodeSeed failed at batchStart=%{public}zu, item=%{private}zu, ret=%{public}d", batchStart, item,
                 ret);
            return ret;
        }
        const auto offset = (item * kPirGroupsPerItem + kIntersectionGroupsPerItem) * context.params.slotsPerData;
        if (offset > seedPlain.size() || seedSlots.size() > seedPlain.size() - offset ||
            memcpy_s(seedPlain.data() + offset, (seedPlain.size() - offset) * sizeof(uint64_t), seedSlots.data(),
                     seedSlots.size() * sizeof(uint64_t)) != EOK) {
            LOGE("Failed to copy PIR seed slots, offset=%{public}zu", offset);
            return PRIVACY_PROTOCOL_CODEC_ERROR;
        }
    }
    return PRIVACY_PROTOCOL_SUCCESS;
}

int32_t AddSeeds(const EvaluationContext &context, std::size_t batchStart, std::size_t batchElementCount,
                 HeEvaluation &evaluation) noexcept
{
    Slots seedPlain;
    auto ret = BuildSeedPlain(context, batchStart, batchElementCount, context.params.polyModulusDegree, seedPlain);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        return ret;
    }
    ret = evaluation.AddPlain(seedPlain);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        LOGE("AddSeeds failed, ret=%{public}d", ret);
    } else {
        LOGI("batch AddSeeds success");
    }
    return ret;
}

int32_t RerandResult(HeEvaluation &evaluation) noexcept
{
    const auto ret = evaluation.Rerandomize();
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        LOGE("RerandResult failed, ret=%{public}d", ret);
    } else {
        LOGI("batch RerandResult success");
    }
    return ret;
}

int32_t EvaluateBatch(const EvaluationContext &context, std::size_t batchStart, std::size_t batchElementCount,
                      Bytes &evaluated) noexcept
{
    std::unique_ptr<HeEvaluation> evaluation;
    auto ret = context.engine.CreateEvaluation(context.privacyTarget, evaluation);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        return ret;
    }
    if (!evaluation) {
        LOGE("HE engine returned success without an evaluation handle");
        return PRIVACY_PROTOCOL_HE_ERROR;
    }
    Slots differencePlain;
    ret = BuildDifferencePlain(context, batchStart, batchElementCount, differencePlain);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        return ret;
    }
    ret = EvaluateDifference(context, differencePlain, *evaluation);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        return ret;
    }
    if (IsPir(context.params)) {
        ret = AddSeeds(context, batchStart, batchElementCount, *evaluation);
        if (ret != PRIVACY_PROTOCOL_SUCCESS) {
            return ret;
        }
        ret = RerandResult(*evaluation);
        if (ret != PRIVACY_PROTOCOL_SUCCESS) {
            return ret;
        }
    }
    ret = evaluation->Serialize(evaluated);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        LOGE("Serialize batch result failed, ret=%{public}d", ret);
    }
    return ret;
}

int32_t EvaluateBatches(const EvaluationContext &context, const BatchLayout &layout,
                        std::vector<Bytes> &batches) noexcept
{
    batches.reserve(layout.batchCount);
    for (std::size_t start = 0; start < context.serverVectors.size(); start += layout.capacity) {
        const auto batch = batches.size();
        const auto count = std::min(layout.capacity, context.serverVectors.size() - start);
        LOGI("Evaluate batch begin, batch=%{public}zu, start=%{public}zu, "
             "count=%{public}zu, capacity=%{public}zu",
             batch, start, count, layout.capacity);
        Bytes evaluated;
        const auto ret = EvaluateBatch(context, start, count, evaluated);
        if (ret != PRIVACY_PROTOCOL_SUCCESS) {
            LOGE("Evaluate batch failed, batch=%{public}zu, start=%{public}zu, "
                 "count=%{public}zu, ret=%{public}d",
                 batch, start, count, ret);
            return ret;
        }
        LOGI("Evaluate batch complete, batch=%{public}zu, count=%{public}zu, "
             "outputBytes=%{public}zu",
             batch, count, evaluated.size());
        batches.push_back(std::move(evaluated));
    }
    LOGI("all Evaluate batches complete, batchCount=%{public}zu", batches.size());
    return PRIVACY_PROTOCOL_SUCCESS;
}

PrivacySearchResult BuildEvaluationResult(std::vector<Bytes> batches, PirPayload payload, const ProtocolParams &params)
{
    PrivacySearchResult result;
    result.resultCipherText = std::move(batches);
    if (IsPir(params)) {
        result.valueCipherText = std::move(payload.ciphertexts);
    }
    return result;
}

int32_t ValidateCiphertextIndex(const SearchResultContext &context, std::size_t globalIndex) noexcept
{
    if (!context.valueCipherText.has_value()) {
        LOGE("valueCipherText not present in PIR mode");
        return PRIVACY_PROTOCOL_CODEC_ERROR;
    }
    if (globalIndex >= context.valueCipherText->size()) {
        LOGE("symmetric index out of range: index=%{private}zu, available=%{public}zu", globalIndex,
             context.valueCipherText->size());
        return PRIVACY_PROTOCOL_CODEC_ERROR;
    }
    return PRIVACY_PROTOCOL_SUCCESS;
}

int32_t DecryptAttachedValue(const SearchResultContext &context, const Slots &plain, std::size_t offset,
                             std::size_t globalIndex, std::optional<std::vector<Bytes>> &attachedValues) noexcept
{
    auto ret = ValidateCiphertextIndex(context, globalIndex);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        return ret;
    }
    const auto pirValueCrypto = GetPirValueCrypto();
    if (!pirValueCrypto) {
        LOGE("value crypto not registered");
        return PRIVACY_PROTOCOL_CRYPTO_ERROR;
    }
    Bytes seed;
    ret = context.encoder.ExtractSeed(plain, offset + context.params.slotsPerData, context.params, seed);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        LOGE("ExtractSeed failed at offset=%{private}zu, ret=%{public}d", offset + context.params.slotsPerData, ret);
        return ret;
    }
    std::optional<Bytes> decrypted;
    ret = pirValueCrypto->Decrypt(seed, (*context.valueCipherText)[globalIndex], context.params, decrypted);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        const auto frameSize = (*context.valueCipherText)[globalIndex].size();
        LOGE("value crypto decrypt failed: index=%{private}zu, frameBytes=%{public}zu, ret=%{public}d", globalIndex,
             frameSize, ret);
        return ret;
    }
    if (decrypted.has_value()) {
        if (!attachedValues.has_value()) {
            attachedValues.emplace();
        }
        attachedValues->push_back(std::move(*decrypted));
    }
    return PRIVACY_PROTOCOL_SUCCESS;
}

int32_t BuildMatchedResult(const SearchResultContext &context, const Slots &plain, std::size_t batchIndex,
                           std::size_t item, SearchResult &result) noexcept
{
    result.matchedResult = true;
    if (!IsPir(context.params)) {
        return PRIVACY_PROTOCOL_SUCCESS;
    }
    const auto offset = item * context.groupsPerItem * context.params.slotsPerData;
    const auto globalIndex = batchIndex * context.capacity + item;
    return DecryptAttachedValue(context, plain, offset, globalIndex, result.attachedValues);
}

int32_t ProcessDecryptedBatch(const SearchResultContext &context, const Slots &plain, std::size_t batch,
                              SearchResult &result) noexcept
{
    for (std::size_t item = 0; item < context.capacity; ++item) {
        const auto offset = item * context.groupsPerItem * context.params.slotsPerData;
        bool matched = false;
        auto ret = context.encoder.IsMatch(plain, offset, context.params, matched);
        if (ret != PRIVACY_PROTOCOL_SUCCESS) {
            LOGE("IsMatch failed at batch=%{public}zu, item=%{private}zu, ret=%{public}d", batch, item, ret);
            return ret;
        }
        if (!matched) {
            continue;
        }
        ret = BuildMatchedResult(context, plain, batch, item, result);
        if (ret != PRIVACY_PROTOCOL_SUCCESS) {
            return ret;
        }
    }
    return PRIVACY_PROTOCOL_SUCCESS;
}

} // namespace

int32_t PrivacyProtocolImpl::GenPrivacyTarget(const TargetInput &input, const ProtocolParams &params,
                                              const CallerIdentity &identity, Bytes &result) const noexcept
{
    auto ret = codec_.ValidateProtocol(params);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        return ret;
    }
    if (input.elemData.empty() || encoder_.ValidateHashAlgorithm(input.hashAlg) != PRIVACY_PROTOCOL_SUCCESS) {
        LOGE("invalid plaintext target or hash algorithm");
        return PRIVACY_PROTOCOL_INVALID_ARGUMENT;
    }
    Slots target;
    const Element element{input.elemData, input.hashAlg, std::nullopt};
    ret = encoder_.EncodeTargetVector(element, params, target);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        return ret;
    }
    LOGI("target vector encoded, slots=%{public}zu", target.size());
    HeSeed seed;
    ret = HeKeyDeriver().DeriveKey(identity, seed);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        LOGE("target seed derivation failed, ret=%{public}d", ret);
        return ret;
    }
    return SoftHeEngine().Encrypt(target, params, seed, result);
}

int32_t PrivacyProtocolImpl::PrivacySearch(const Bytes &privacyTarget, const std::vector<Element> &elements,
                                           const ProtocolParams &params,
                                           PrivacySearchResult &searchResult) const noexcept
{
    auto ret = EnsureDefaultHeEngineInstalled();
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        return ret;
    }
    ret = ValidateEvaluateInput(privacyTarget, elements, params, codec_, encoder_);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        return ret;
    }
    if (elements.empty()) {
        SetEmptyEvaluationResult(params, searchResult);
        return PRIVACY_PROTOCOL_SUCCESS;
    }
    std::shared_ptr<const IHEEngine> engine;
    ret = GetCompatibleEngine(params, engine);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        return ret;
    }
    std::vector<Slots> serverVectors;
    ret = EncodeServerVectors(elements, params, encoder_, serverVectors);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        return ret;
    }
    PirPayload payload;
    ret = PreparePayload(elements, params, encoder_, payload);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        return ret;
    }
    BatchLayout layout;
    ret = CalculateBatchLayout(params, elements.size(), layout);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        return ret;
    }
    std::vector<Bytes> batches;
    const EvaluationContext context{privacyTarget, params, encoder_, *engine, serverVectors, payload};
    ret = EvaluateBatches(context, layout, batches);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        return ret;
    }
    searchResult = BuildEvaluationResult(std::move(batches), std::move(payload), params);
    return PRIVACY_PROTOCOL_SUCCESS;
}

int32_t PrivacyProtocolImpl::DecryptCiphertext(const Bytes &ciphertext, const ProtocolParams &params,
                                               const CallerIdentity &identity, Bytes &output) const noexcept
{
    auto ret = codec_.ValidateProtocol(params);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        return ret;
    }
    if (ciphertext.empty()) {
        return PRIVACY_PROTOCOL_INVALID_ARGUMENT;
    }
    HeSeed seed;
    ret = HeKeyDeriver().DeriveKey(identity, seed);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        LOGE("HE seed derivation failed, ret=%{public}d", ret);
        return ret;
    }
    Slots slots;
    ret = SoftHeEngine().Decrypt(ciphertext, params, seed, slots);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        return ret;
    }
    return codec_.SerializePlaintext(slots, params, output);
}

int32_t PrivacyProtocolImpl::PostProcessSearchResult(const std::vector<Bytes> &fhePlainText,
                                                     const std::optional<std::vector<Bytes>> &valueCipherText,
                                                     const ProtocolParams &params, SearchResult &result) const noexcept
{
    auto ret = codec_.ValidateProtocol(params);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        return ret;
    }
    std::vector<Slots> plaintexts;
    ret = codec_.ParsePlaintexts(fhePlainText, valueCipherText, params, plaintexts);
    if (ret != PRIVACY_PROTOCOL_SUCCESS) {
        LOGE("ParsePlaintexts failed, ret=%{public}d", ret);
        return ret;
    }
    const auto groupsPerItem = GetGroupsPerItem(params);
    if (groupsPerItem == 0) {
        return PRIVACY_PROTOCOL_CODEC_ERROR;
    }
    const SearchResultContext context{valueCipherText, params, encoder_, groupsPerItem,
                                      params.dataNumPerPlaintext / groupsPerItem};
    SearchResult resolvedResult;
    for (size_t batch = 0; batch < plaintexts.size(); ++batch) {
        ret = ProcessDecryptedBatch(context, plaintexts[batch], batch, resolvedResult);
        if (ret != PRIVACY_PROTOCOL_SUCCESS) {
            return ret;
        }
    }
    result = std::move(resolvedResult);
    return PRIVACY_PROTOCOL_SUCCESS;
}

} // namespace OHOS::Security::PrivacyComputation::internal
