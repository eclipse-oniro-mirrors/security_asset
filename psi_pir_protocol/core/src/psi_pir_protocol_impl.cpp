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

#include "he_key_deriver.h"
#include "protocol_evaluation.h"
#include "protocol_mapper.h"
#include "provider_registry.h"
#include "psi_pir_log.h"
#include "search_result_processor.h"
#include "soft_he_engine.h"

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
    const std::optional<std::vector<Bytes>> &symCipherText;
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

int32_t EvaluateDifference(const EvaluationContext &context, const Slots &differencePlain,
                           HeEvaluation &evaluation) noexcept
{
    auto ret = evaluation.AddPlain(differencePlain);
    if (ret != PSI_PIR_SUCCESS) {
        PSI_PIR_LOGE("EvaluateDifference subtraction failed, ret=%{public}d", ret);
        return ret;
    }
    Slots mask;
    ret = context.encoder.GenerateMaskVector(context.params, mask);
    if (ret != PSI_PIR_SUCCESS) {
        PSI_PIR_LOGE("GenerateMaskVector failed, ret=%{public}d", ret);
        return ret;
    }
    ret = evaluation.MultiplyPlain(mask);
    if (ret != PSI_PIR_SUCCESS) {
        PSI_PIR_LOGE("EvaluateDifference mask multiplication failed, ret=%{public}d", ret);
    } else {
        PSI_PIR_LOGI("batch EvaluateDifference success");
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
                 HeEvaluation &evaluation) noexcept
{
    Slots seedPlain;
    auto ret = BuildSeedPlain(context, batchStart, batchElementCount, context.params.polyModulusDegree, seedPlain);
    if (ret != PSI_PIR_SUCCESS) {
        return ret;
    }
    ret = evaluation.AddPlain(seedPlain);
    if (ret != PSI_PIR_SUCCESS) {
        PSI_PIR_LOGE("AddSeeds failed, ret=%{public}d", ret);
    } else {
        PSI_PIR_LOGI("batch AddSeeds success");
    }
    return ret;
}

int32_t RerandResult(HeEvaluation &evaluation) noexcept
{
    const auto ret = evaluation.Rerandomize();
    if (ret != PSI_PIR_SUCCESS) {
        PSI_PIR_LOGE("RerandResult failed, ret=%{public}d", ret);
    } else {
        PSI_PIR_LOGI("batch RerandResult success");
    }
    return ret;
}

int32_t EvaluateBatch(const EvaluationContext &context, std::size_t batchStart, std::size_t batchElementCount,
                      Bytes &evaluated) noexcept
{
    std::unique_ptr<HeEvaluation> evaluation;
    auto ret = CreateHeEvaluation(context.engine, context.privacyTarget, evaluation);
    if (ret != PSI_PIR_SUCCESS) {
        return ret;
    }
    const auto differencePlain = BuildDifferencePlain(context, batchStart, batchElementCount);
    ret = EvaluateDifference(context, differencePlain, *evaluation);
    if (ret != PSI_PIR_SUCCESS) {
        return ret;
    }
    if (IsPir(context.params)) {
        ret = AddSeeds(context, batchStart, batchElementCount, *evaluation);
        if (ret != PSI_PIR_SUCCESS) {
            return ret;
        }
        ret = RerandResult(*evaluation);
        if (ret != PSI_PIR_SUCCESS) {
            return ret;
        }
    }
    ret = evaluation->Serialize(evaluated);
    if (ret != PSI_PIR_SUCCESS) {
        PSI_PIR_LOGE("Serialize batch result failed, ret=%{public}d", ret);
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
    if (!context.symCipherText.has_value()) {
        PSI_PIR_LOGE("symCipherText not present in PIR mode");
        return PSI_PIR_CODEC_ERROR;
    }
    if (globalIndex >= context.symCipherText->size()) {
        PSI_PIR_LOGE("symmetric index out of range: index=%{private}zu, available=%{public}zu", globalIndex,
                     context.symCipherText->size());
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
    ret = pirValueCrypto->decrypt(seed, (*context.symCipherText)[globalIndex], context.params, decrypted);
    if (ret != PSI_PIR_SUCCESS) {
        const auto frameSize = (*context.symCipherText)[globalIndex].size();
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

int32_t ProcessDecryptedBatch(const SearchResultContext &context, const Slots &plain, std::size_t batch,
                              SearchResult &result) noexcept
{
    for (std::size_t item = 0; item < context.capacity; ++item) {
        const auto offset = item * context.groupsPerItem * context.params.slotsPerData;
        bool matched = false;
        auto ret = context.encoder.IsMatch(plain, offset, context.params, matched);
        if (ret != PSI_PIR_SUCCESS) {
            PSI_PIR_LOGE("IsMatch failed at batch=%{public}zu, item=%{private}zu, ret=%{public}d", batch, item, ret);
            return ret;
        }
        if (!matched) {
            continue;
        }
        ret = BuildMatchedResult(context, plain, batch, item, result);
        if (ret != PSI_PIR_SUCCESS) {
            return ret;
        }
    }
    return PSI_PIR_SUCCESS;
}

int32_t ProcessSearchResultPlaintexts(const SearchResultContext &context, const std::vector<Slots> &plaintexts,
                                      SearchResult &resolvedResult) noexcept
{
    SearchResult result;
    for (size_t batch = 0; batch < plaintexts.size(); ++batch) {
        const auto ret = ProcessDecryptedBatch(context, plaintexts[batch], batch, result);
        if (ret != PSI_PIR_SUCCESS) {
            return ret;
        }
    }
    resolvedResult = std::move(result);
    return PSI_PIR_SUCCESS;
}

} // namespace

int32_t PsiPirProtocolImpl::GenPrivacyTarget(const TargetInput &input, const ProtocolParams &params,
                                             const CallerIdentity &identity, Bytes &result) const noexcept
{
    auto ret = codec_.ValidateProtocol(params);
    if (ret != PSI_PIR_SUCCESS) {
        return ret;
    }
    if (input.elemData.empty() || encoder_.ValidateHashAlgorithm(input.hashAlg) != PSI_PIR_SUCCESS) {
        PSI_PIR_LOGE("invalid plaintext target or hash algorithm");
        return PSI_PIR_INVALID_ARGUMENT;
    }
    Slots target;
    const Element element{input.elemData, input.hashAlg, std::nullopt};
    ret = encoder_.EncodeTargetVector(element, params, target);
    if (ret != PSI_PIR_SUCCESS) {
        return ret;
    }
    PSI_PIR_LOGI("target vector encoded, slots=%{public}zu", target.size());
    HeSeed seed;
    ret = HeKeyDeriver().DeriveKey(identity, seed);
    if (ret != PSI_PIR_SUCCESS) {
        PSI_PIR_LOGE("target seed derivation failed, ret=%{public}d", ret);
        return ret;
    }
    return SoftHeEngine().Encrypt(target, params, seed, result);
}

int32_t PsiPirProtocolImpl::PrivacySearch(const Bytes &privacyTarget, const std::vector<Element> &elements,
                                          const ProtocolParams &params,
                                          PrivacySearchResult &searchResult) const noexcept
{
    auto ret = EnsureDefaultHeEngineInstalled();
    if (ret != PSI_PIR_SUCCESS) {
        return ret;
    }
    ret = ValidateEvaluateInput(privacyTarget, elements, params, codec_, encoder_);
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

int32_t PsiPirProtocolImpl::DecryptCiphertext(const Bytes &ciphertext, const ProtocolParams &params,
    const CallerIdentity &identity, Bytes &output) const noexcept
{
    auto ret = codec_.ValidateProtocol(params);
    if (ret != PSI_PIR_SUCCESS) {
        return ret;
    }
    if (ciphertext.empty()) {
        return PSI_PIR_INVALID_ARGUMENT;
    }
    HeSeed seed;
    ret = HeKeyDeriver().DeriveKey(identity, seed);
    if (ret != PSI_PIR_SUCCESS) {
        PSI_PIR_LOGE("HE seed derivation failed, ret=%{public}d", ret);
        return ret;
    }
    Slots slots;
    ret = SoftHeEngine().Decrypt(ciphertext, params, seed, slots);
    if (ret != PSI_PIR_SUCCESS) {
        return ret;
    }
    return codec_.SerializePlaintext(slots, params, output);
}

int32_t PsiPirProtocolImpl::PostProcessSearchResult(const std::vector<Bytes> &fhePlainText,
    const std::optional<std::vector<Bytes>> &symCipherText, const ProtocolParams &params,
    SearchResult &result) const noexcept
{
    auto ret = codec_.ValidateProtocol(params);
    if (ret != PSI_PIR_SUCCESS) {
        return ret;
    }
    std::vector<Slots> plaintexts;
    ret = codec_.ParsePlaintexts(fhePlainText, symCipherText, params, plaintexts);
    if (ret != PSI_PIR_SUCCESS) {
        PSI_PIR_LOGE("ParsePlaintexts failed, ret=%{public}d", ret);
        return ret;
    }
    const auto groupsPerItem = GetGroupsPerItem(params);
    const SearchResultContext context{symCipherText, params, encoder_, groupsPerItem,
        params.dataNumPerPlaintext / groupsPerItem};
    return ProcessSearchResultPlaintexts(context, plaintexts, result);
}

int32_t PsiPirProtocolImpl::ProcessDecryptedSearchResult(const PrivacySearchResult &input,
    const ProtocolParams &params, SearchResult &result) const noexcept
{
    return PostProcessSearchResult(input.fheCipherText, input.symCipherText, params, result);
}

int32_t ProcessDecryptedSearchResult(const PrivacySearchResult &input, const PrivacyProtocol &protocol,
                                     SearchResult &output) noexcept
{
    ProtocolParams params;
    const auto ret = MapPrivacyProtocol(protocol, params);
    if (ret != PSI_PIR_SUCCESS) {
        return ret;
    }
    return PsiPirProtocolImpl().ProcessDecryptedSearchResult(input, params, output);
}

} // namespace psi_pir::internal
