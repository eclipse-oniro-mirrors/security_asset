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

#include "psi_pir_protocol_impl.h"

#include "provider_registry.h"
#include "psi_pir_log.h"

#include <algorithm>
#include <limits>
#include <utility>

namespace psi_pir::internal {
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
    const PrivacySearchResult &searchResult;
    const ProtocolParams &params;
    const ProtocolEncoder &encoder;
    std::size_t groupsPerItem;
    std::size_t capacity;
};

struct MatchLocation {
    std::size_t batch;
    std::size_t item;
};

bool IsPir(const ProtocolParams &params) noexcept
{
    return params.mode == ProtocolMode::PIR;
}

std::size_t GetGroupsPerItem(const ProtocolParams &params) noexcept
{
    return IsPir(params) ? kPirGroupsPerItem : kPsiGroupsPerItem;
}

int32_t ValidateElements(const std::vector<Element> &elements, const ProtocolEncoder &encoder) noexcept
{
    for (std::size_t index = 0; index < elements.size(); ++index) {
        const auto &element = elements[index];
        const auto ret = encoder.ValidateHashAlgorithm(element.hashAlg);
        if (ret != PSI_PIR_SUCCESS) {
            PSI_PIR_LOGE("element validation failed: index=%{private}zu, hashAlg=%{public}u, "
                         "ret=%{public}d",
                         index, static_cast<uint32_t>(element.hashAlg), ret);
            return ret;
        }
    }
    return PSI_PIR_SUCCESS;
}

int32_t ValidateEvaluateInput(const Bytes &privacyTarget, const std::vector<Element> &elements,
                              const ProtocolParams &params, const ProtocolCodec &codec,
                              const ProtocolEncoder &encoder) noexcept
{
    auto ret = codec.ValidateProtocol(params);
    if (ret != PSI_PIR_SUCCESS) {
        PSI_PIR_LOGE("ValidateProtocol failed, ret=%{public}d", ret);
        return ret;
    }
    ret = codec.ValidatePrivacyTarget(privacyTarget);
    if (ret != PSI_PIR_SUCCESS) {
        PSI_PIR_LOGE("ValidatePrivacyTarget failed, ret=%{public}d", ret);
        return ret;
    }
    return ValidateElements(elements, encoder);
}

void SetEmptyEvaluationResult(const ProtocolParams &params, PrivacySearchResult &searchResult)
{
    PSI_PIR_LOGI("Evaluate completed with an empty element set, mode=%{public}u", static_cast<uint32_t>(params.mode));
    PrivacySearchResult emptyResult;
    if (IsPir(params)) {
        emptyResult.symCipherText.emplace();
    }
    searchResult = std::move(emptyResult);
}

int32_t GetCompatibleEngine(const ProtocolParams &params, std::shared_ptr<const IHEEngine> &engine) noexcept
{
    engine = GetHEEngine();
    if (!engine) {
        PSI_PIR_LOGE("HE engine not registered");
        return PSI_PIR_HE_ERROR;
    }
    if (!engine->IsCompatible(params)) {
        PSI_PIR_LOGE("HE engine incompatible: degree=%{public}zu, modulus=%{public}llu", params.polyModulusDegree,
                     static_cast<unsigned long long>(params.plainModulus));
        return PSI_PIR_INCOMPATIBLE_HE_PARAMETERS;
    }
    PSI_PIR_LOGI("HE engine compatible, algorithm=%{public}u, degree=%{public}zu, "
                 "modulus=%{public}llu",
                 static_cast<uint32_t>(params.fheAlgorithm), params.polyModulusDegree,
                 static_cast<unsigned long long>(params.plainModulus));
    return PSI_PIR_SUCCESS;
}

int32_t EncodeServerVectors(const std::vector<Element> &elements, const ProtocolParams &params,
                            const ProtocolEncoder &encoder, std::vector<Slots> &serverVectors) noexcept
{
    serverVectors.reserve(elements.size());
    for (std::size_t i = 0; i < elements.size(); ++i) {
        Slots serverVector;
        const auto ret = encoder.EncodeServerVector(elements[i], params, serverVector);
        if (ret != PSI_PIR_SUCCESS) {
            PSI_PIR_LOGE("EncodeServerVector failed at index=%{private}zu, ret=%{public}d", i, ret);
            return ret;
        }
        serverVectors.push_back(std::move(serverVector));
    }
    PSI_PIR_LOGI("server vector encoding complete, count=%{public}zu", serverVectors.size());
    return PSI_PIR_SUCCESS;
}

int32_t EncryptElementValue(const Element &element, const EncryptionContext &context, Bytes &seed,
                            Bytes &ciphertext) noexcept
{
    auto ret = context.encoder.GenerateSeed(context.params, seed);
    if (ret != PSI_PIR_SUCCESS) {
        PSI_PIR_LOGE("GenerateSeed failed, ret=%{public}d", ret);
        return ret;
    }
    ret = context.pirValueCrypto.encrypt(seed, element.elemValue, context.params, ciphertext);
    if (ret != PSI_PIR_SUCCESS) {
        PSI_PIR_LOGE("pirValueCrypto encrypt failed, ret=%{public}d", ret);
        return ret;
    }
    if (ciphertext.empty()) {
        PSI_PIR_LOGE("encrypted ciphertext is empty");
        return PSI_PIR_CRYPTO_ERROR;
    }
    return PSI_PIR_SUCCESS;
}

int32_t PreparePirPayload(const std::vector<Element> &elements, const ProtocolParams &params,
                          const ProtocolEncoder &encoder, PirPayload &payload) noexcept
{
    const auto pirValueCrypto = GetPirValueCrypto();
    if (!pirValueCrypto) {
        PSI_PIR_LOGE("pir value crypto not registered");
        return PSI_PIR_CRYPTO_ERROR;
    }
    const EncryptionContext context{params, encoder, *pirValueCrypto};
    payload.seeds.reserve(elements.size());
    payload.ciphertexts.reserve(elements.size());
    for (std::size_t i = 0; i < elements.size(); ++i) {
        Bytes seed;
        Bytes ciphertext;
        const auto ret = EncryptElementValue(elements[i], context, seed, ciphertext);
        if (ret != PSI_PIR_SUCCESS) {
            PSI_PIR_LOGE("EncryptElementValue failed at index=%{private}zu, ret=%{public}d", i, ret);
            return ret;
        }
        payload.seeds.push_back(std::move(seed));
        payload.ciphertexts.push_back(std::move(ciphertext));
    }
    PSI_PIR_LOGI("PIR payload prepared, elements=%{public}zu, symmetricCount=%{public}zu", payload.seeds.size(),
                 payload.ciphertexts.size());
    return PSI_PIR_SUCCESS;
}

int32_t PreparePayload(const std::vector<Element> &elements, const ProtocolParams &params,
                       const ProtocolEncoder &encoder, PirPayload &payload) noexcept
{
    if (!IsPir(params)) {
        return PSI_PIR_SUCCESS;
    }
    return PreparePirPayload(elements, params, encoder, payload);
}

int32_t CalculateBatchLayout(const ProtocolParams &params, std::size_t elementCount, BatchLayout &layout) noexcept
{
    layout.capacity = params.dataNumPerPlaintext / GetGroupsPerItem(params);
    if (layout.capacity == 0 || elementCount > std::numeric_limits<std::size_t>::max() - (layout.capacity - 1)) {
        PSI_PIR_LOGE("invalid batch layout: capacity=%{public}zu, elementCount=%{public}zu", layout.capacity,
                     elementCount);
        return PSI_PIR_INVALID_ARGUMENT;
    }
    layout.batchCount = (elementCount + layout.capacity - 1) / layout.capacity;
    PSI_PIR_LOGI("batch layout: capacity=%{public}zu, batchCount=%{public}zu", layout.capacity, layout.batchCount);
    return PSI_PIR_SUCCESS;
}

void CopyServerVector(const EvaluationContext &context, const Slots &serverVector, std::size_t item,
                      Slots &differencePlain)
{
    const auto groupsPerItem = GetGroupsPerItem(context.params);
    for (std::size_t group = 0; group < groupsPerItem; ++group) {
        const auto offset = (item * groupsPerItem + group) * context.params.slotsPerData;
        std::copy(serverVector.begin(), serverVector.end(),
                  differencePlain.begin() + static_cast<std::ptrdiff_t>(offset));
    }
}

Slots BuildDifferencePlain(const EvaluationContext &context, std::size_t batchStart, std::size_t batchElementCount)
{
    Slots differencePlain(context.params.dataNumPerPlaintext * context.params.slotsPerData, 0);
    for (std::size_t item = 0; item < batchElementCount; ++item) {
        CopyServerVector(context, context.serverVectors[batchStart + item], item, differencePlain);
    }
    return differencePlain;
}

int32_t EvaluateDifference(const EvaluationContext &context, const Slots &differencePlain, Bytes &evaluated) noexcept
{
    Bytes differenceCiphertext;
    auto ret = context.engine.addPlain(context.privacyTarget, differencePlain, differenceCiphertext);
    if (ret != PSI_PIR_SUCCESS) {
        PSI_PIR_LOGE("HE addPlain difference failed: ciphertextBytes=%{public}zu, "
                     "plainSlots=%{public}zu, ret=%{public}d",
                     context.privacyTarget.size(), differencePlain.size(), ret);
        return ret;
    }
    if (differenceCiphertext.empty()) {
        PSI_PIR_LOGE("HE addPlain returned empty ciphertext");
        return PSI_PIR_HE_ERROR;
    }
    PSI_PIR_LOGI("HE addPlain difference complete, inputBytes=%{public}zu, "
                 "plainSlots=%{public}zu, outputBytes=%{public}zu",
                 context.privacyTarget.size(), differencePlain.size(), differenceCiphertext.size());
    Slots maskVector;
    ret = context.encoder.GenerateMaskVector(context.params, maskVector);
    if (ret != PSI_PIR_SUCCESS) {
        PSI_PIR_LOGE("GenerateMaskVector failed, ret=%{public}d", ret);
        return ret;
    }
    ret = context.engine.multiplyPlain(differenceCiphertext, maskVector, evaluated);
    if (ret != PSI_PIR_SUCCESS) {
        PSI_PIR_LOGE("HE multiplyPlain failed: ciphertextBytes=%{public}zu, "
                     "plainSlots=%{public}zu, ret=%{public}d",
                     differenceCiphertext.size(), maskVector.size(), ret);
        return ret;
    }
    if (evaluated.empty()) {
        PSI_PIR_LOGE("HE multiplyPlain returned empty ciphertext");
        return PSI_PIR_HE_ERROR;
    }
    PSI_PIR_LOGI("HE multiplyPlain mask complete, inputBytes=%{public}zu, "
                 "plainSlots=%{public}zu, outputBytes=%{public}zu",
                 differenceCiphertext.size(), maskVector.size(), evaluated.size());
    return PSI_PIR_SUCCESS;
}

int32_t BuildSeedPlain(const EvaluationContext &context, std::size_t batchStart, std::size_t batchElementCount,
                       std::size_t slotCount, Slots &seedPlain) noexcept
{
    seedPlain.assign(slotCount, 0);
    for (std::size_t item = 0; item < batchElementCount; ++item) {
        Slots seedSlots;
        const auto ret =
            context.encoder.EncodeSeed(context.payload.seeds[batchStart + item], context.params, seedSlots);
        if (ret != PSI_PIR_SUCCESS) {
            PSI_PIR_LOGE("EncodeSeed failed at batchStart=%{public}zu, item=%{private}zu, ret=%{public}d", batchStart,
                         item, ret);
            return ret;
        }
        const auto offset = (item * kPirGroupsPerItem + kPsiGroupsPerItem) * context.params.slotsPerData;
        std::copy(seedSlots.begin(), seedSlots.end(), seedPlain.begin() + static_cast<std::ptrdiff_t>(offset));
    }
    return PSI_PIR_SUCCESS;
}

int32_t AddSeeds(const EvaluationContext &context, std::size_t batchStart, std::size_t batchElementCount,
                 std::size_t slotCount, Bytes &evaluated) noexcept
{
    Slots seedPlain;
    auto ret = BuildSeedPlain(context, batchStart, batchElementCount, slotCount, seedPlain);
    if (ret != PSI_PIR_SUCCESS) {
        return ret;
    }
    Bytes seededEvaluation;
    ret = context.engine.addPlain(evaluated, seedPlain, seededEvaluation);
    if (ret != PSI_PIR_SUCCESS) {
        PSI_PIR_LOGE("HE addPlain seed failed: ciphertextBytes=%{public}zu, "
                     "plainSlots=%{public}zu, ret=%{public}d",
                     evaluated.size(), seedPlain.size(), ret);
        return ret;
    }
    evaluated = std::move(seededEvaluation);
    if (evaluated.empty()) {
        PSI_PIR_LOGE("HE addPlain (seeds) returned empty ciphertext");
        return PSI_PIR_HE_ERROR;
    }
    PSI_PIR_LOGI("HE addPlain seed complete, plainSlots=%{public}zu, outputBytes=%{public}zu", seedPlain.size(),
                 evaluated.size());
    return PSI_PIR_SUCCESS;
}

int32_t RerandResult(const EvaluationContext &context, Bytes &evaluated)
{
    Bytes rerandEvaluation;
    auto ret = context.engine.RandCipherText(evaluated, rerandEvaluation);
    if (ret != PSI_PIR_SUCCESS) {
        PSI_PIR_LOGE("HE Rerand failed: ciphertextBytes=%{public}zu, ret=%{public}d", evaluated.size(), ret);
        return ret;
    }
    std::size_t inputSize = evaluated.size();
    evaluated = std::move(rerandEvaluation);
    PSI_PIR_LOGI("HE Rerand complete, inputBytes=%{public}zu, outputBytes=%{public}zu", inputSize, evaluated.size());
    return ret;
}

int32_t EvaluateBatch(const EvaluationContext &context, std::size_t batchStart, std::size_t batchElementCount,
                      Bytes &evaluated) noexcept
{
    const auto differencePlain = BuildDifferencePlain(context, batchStart, batchElementCount);
    auto ret = EvaluateDifference(context, differencePlain, evaluated);
    if (ret != PSI_PIR_SUCCESS || !IsPir(context.params)) {
        PSI_PIR_LOGW("EvaluateDifference failed or is psi protocol(if ret = 0), ret=%{public}d", ret);
        return ret;
    }
    ret = AddSeeds(context, batchStart, batchElementCount, differencePlain.size(), evaluated);
    if (ret != PSI_PIR_SUCCESS) {
        PSI_PIR_LOGE("AddSeeds failed, ret=%{public}d", ret);
        return ret;
    }
    return RerandResult(context, evaluated);
}

int32_t EvaluateBatches(const EvaluationContext &context, const BatchLayout &layout,
                        std::vector<Bytes> &batches) noexcept
{
    batches.reserve(layout.batchCount);
    for (std::size_t start = 0; start < context.serverVectors.size(); start += layout.capacity) {
        const auto batch = batches.size();
        const auto count = std::min(layout.capacity, context.serverVectors.size() - start);
        PSI_PIR_LOGI("Evaluate batch begin, batch=%{public}zu, start=%{public}zu, "
                     "count=%{public}zu, capacity=%{public}zu",
                     batch, start, count, layout.capacity);
        Bytes evaluated;
        const auto ret = EvaluateBatch(context, start, count, evaluated);
        if (ret != PSI_PIR_SUCCESS) {
            PSI_PIR_LOGE("Evaluate batch failed, batch=%{public}zu, start=%{public}zu, "
                         "count=%{public}zu, ret=%{public}d",
                         batch, start, count, ret);
            return ret;
        }
        PSI_PIR_LOGI("Evaluate batch complete, batch=%{public}zu, count=%{public}zu, "
                     "outputBytes=%{public}zu",
                     batch, count, evaluated.size());
        batches.push_back(std::move(evaluated));
    }
    PSI_PIR_LOGI("all Evaluate batches complete, batchCount=%{public}zu", batches.size());
    return PSI_PIR_SUCCESS;
}

PrivacySearchResult BuildEvaluationResult(std::vector<Bytes> batches, PirPayload payload, const ProtocolParams &params)
{
    PrivacySearchResult result;
    result.fheCipherText = std::move(batches);
    if (IsPir(params)) {
        result.symCipherText = std::move(payload.ciphertexts);
    }
    return result;
}

int32_t ValidateCiphertextIndex(const SearchResultContext &context, std::size_t globalIndex) noexcept
{
    if (!context.searchResult.symCipherText.has_value()) {
        PSI_PIR_LOGE("symCipherText not present in PIR mode");
        return PSI_PIR_CODEC_ERROR;
    }
    if (globalIndex >= context.searchResult.symCipherText->size()) {
        PSI_PIR_LOGE("symmetric index out of range: index=%{private}zu, available=%{public}zu", globalIndex,
                     context.searchResult.symCipherText->size());
        return PSI_PIR_CODEC_ERROR;
    }
    return PSI_PIR_SUCCESS;
}

int32_t DecryptAttachedValue(const SearchResultContext &context, const Slots &plain, std::size_t offset,
                             std::size_t globalIndex, std::optional<std::vector<Bytes>> &attachedValues) noexcept
{
    auto ret = ValidateCiphertextIndex(context, globalIndex);
    if (ret != PSI_PIR_SUCCESS) {
        return ret;
    }
    const auto pirValueCrypto = GetPirValueCrypto();
    if (!pirValueCrypto) {
        PSI_PIR_LOGE("value crypto not registered");
        return PSI_PIR_CRYPTO_ERROR;
    }
    Bytes seed;
    ret = context.encoder.ExtractSeed(plain, offset + context.params.slotsPerData, context.params, seed);
    if (ret != PSI_PIR_SUCCESS) {
        PSI_PIR_LOGE("ExtractSeed failed at offset=%{private}zu, ret=%{public}d", offset + context.params.slotsPerData,
                     ret);
        return ret;
    }
    std::optional<Bytes> decrypted;
    ret = pirValueCrypto->decrypt(seed, (*context.searchResult.symCipherText)[globalIndex], context.params, decrypted);
    if (ret != PSI_PIR_SUCCESS) {
        const auto frameSize = (*context.searchResult.symCipherText)[globalIndex].size();
        PSI_PIR_LOGE("value crypto decrypt failed: index=%{private}zu, frameBytes=%{public}zu, ret=%{public}d",
                     globalIndex, frameSize, ret);
        return ret;
    }
    if (decrypted.has_value()) {
        if (!attachedValues.has_value()) {
            attachedValues.emplace();
        }
        attachedValues->push_back(std::move(*decrypted));
    }
    return PSI_PIR_SUCCESS;
}

int32_t BuildMatchedResult(const SearchResultContext &context, const Slots &plain, std::size_t batchIndex,
                           std::size_t item, SearchResult &result) noexcept
{
    result.matchedResult = true;
    if (!IsPir(context.params)) {
        return PSI_PIR_SUCCESS;
    }
    const auto offset = item * context.groupsPerItem * context.params.slotsPerData;
    const auto globalIndex = batchIndex * context.capacity + item;
    return DecryptAttachedValue(context, plain, offset, globalIndex, result.attachedValues);
}

int32_t ProcessSearchResultPlaintexts(const SearchResultContext &context, const std::vector<Slots> &plaintexts,
                                      SearchResult &resolvedResult) noexcept
{
    resolvedResult = {};
    for (std::size_t batch = 0; batch < plaintexts.size(); ++batch) {
        for (std::size_t item = 0; item < context.capacity; ++item) {
            const auto offset = item * context.groupsPerItem * context.params.slotsPerData;
            bool matched = false;
            auto ret = context.encoder.IsMatch(plaintexts[batch], offset, context.params, matched);
            if (ret != PSI_PIR_SUCCESS) {
                PSI_PIR_LOGE("IsMatch failed at batch=%{public}zu, item=%{private}zu, ret=%{public}d", batch, item,
                             ret);
                return ret;
            }
            if (!matched) {
                continue;
            }
            ret = BuildMatchedResult(context, plaintexts[batch], batch, item, resolvedResult);
            if (ret != PSI_PIR_SUCCESS) {
                return ret;
            }
        }
    }
    if (IsPir(context.params)) {
        const std::size_t matchedCount =
            resolvedResult.attachedValues.has_value() ? resolvedResult.attachedValues->size() : 0;
        PSI_PIR_LOGI("GetSearchResult scan complete, PIR batchCount=%{public}zu, matched=%{public}d, matchedCount="
                     "%{public}zu",
                     plaintexts.size(), resolvedResult.matchedResult, matchedCount);
    } else {
        PSI_PIR_LOGI("GetSearchResult scan complete, PSI batchCount=%{public}zu, matched=%{public}d", plaintexts.size(),
                     resolvedResult.matchedResult);
    }
    return PSI_PIR_SUCCESS;
}

} // namespace

int32_t PsiPirProtocolImpl::privacySearch(const Bytes &privacyTarget, const std::vector<Element> &elements,
                                          const ProtocolParams &params,
                                          PrivacySearchResult &searchResult) const noexcept
{
    auto ret = ValidateEvaluateInput(privacyTarget, elements, params, codec_, encoder_);
    if (ret != PSI_PIR_SUCCESS) {
        return ret;
    }
    if (elements.empty()) {
        SetEmptyEvaluationResult(params, searchResult);
        return PSI_PIR_SUCCESS;
    }
    std::shared_ptr<const IHEEngine> engine;
    ret = GetCompatibleEngine(params, engine);
    if (ret != PSI_PIR_SUCCESS) {
        return ret;
    }
    std::vector<Slots> serverVectors;
    ret = EncodeServerVectors(elements, params, encoder_, serverVectors);
    if (ret != PSI_PIR_SUCCESS) {
        return ret;
    }
    PirPayload payload;
    ret = PreparePayload(elements, params, encoder_, payload);
    if (ret != PSI_PIR_SUCCESS) {
        return ret;
    }
    BatchLayout layout;
    ret = CalculateBatchLayout(params, elements.size(), layout);
    if (ret != PSI_PIR_SUCCESS) {
        return ret;
    }
    std::vector<Bytes> batches;
    const EvaluationContext context{privacyTarget, params, encoder_, *engine, serverVectors, payload};
    ret = EvaluateBatches(context, layout, batches);
    if (ret != PSI_PIR_SUCCESS) {
        return ret;
    }
    searchResult = BuildEvaluationResult(std::move(batches), std::move(payload), params);
    return PSI_PIR_SUCCESS;
}

int32_t PsiPirProtocolImpl::getSearchResult(const PrivacySearchResult &privacySearchResult,
                                            const ProtocolParams &params, SearchResult &resolvedResult) const noexcept
{
    auto ret = codec_.ValidateProtocol(params);
    if (ret != PSI_PIR_SUCCESS) {
        PSI_PIR_LOGE("ValidateProtocol failed, ret=%{public}d", ret);
        return ret;
    }
    std::vector<Slots> plaintexts;
    ret = codec_.ParsePlaintexts(privacySearchResult, params, plaintexts);
    if (ret != PSI_PIR_SUCCESS) {
        PSI_PIR_LOGE("ParsePlaintexts failed, ret=%{public}d", ret);
        return ret;
    }
    const auto groupsPerItem = GetGroupsPerItem(params);
    const SearchResultContext context{privacySearchResult, params, encoder_, groupsPerItem,
                                      params.dataNumPerPlaintext / groupsPerItem};
    return ProcessSearchResultPlaintexts(context, plaintexts, resolvedResult);
}

} // namespace psi_pir::internal
