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

#include "napi_host.h"
#include "host_faults.h"
#include "privacy_computation_napi_common.h"
#include "privacy_computation_napi_loader.h"
#include "privacy_request.h"
#include "privacy_client.h"
#include "he_key_deriver.h"
#include "huks_seed_deriver.h"
#include "provider_registry.h"
#include "hifhe_adapter.h"
#include "protocol_mapper.h"
#include "soft_he_engine.h"
#include <limits>
#include <cstring>
#include <cstdlib>

namespace {
using namespace testing::ext;
using namespace OHOS::Security::PrivacyComputation;
using namespace psi_pir;
using namespace psi_pir::internal;
using PrivacyHost::State;

class PrivacyBoundaryTest : public testing::Test {
public:
    void SetUp() override
    {
        PrivacyHost::Reset();
        SetHeSeedDeriver(std::make_shared<HuksSeedDeriver>());
        SetHashProvider(nullptr);
        SetRandomSource(nullptr);
        SetPirValueCrypto(nullptr);
        SetHEEngine(nullptr);
    }
    napi_value Protocol(bool pir = false)
    {
        auto result = PrivacyHost::Value(&env_);
        result->properties["dataSetSize"] = PrivacyHost::Number(&env_, pir ? 1 : 2);
        result->properties["protocolType"] = PrivacyHost::Number(&env_, pir ? 1 : 0);
        return result;
    }
    napi_value Target()
    {
        auto result = PrivacyHost::Value(&env_);
        result->properties["elemData"] = PrivacyHost::Bytes(&env_, psi_pir::Bytes(8, 0x45));
        result->properties["hashAlg"] = PrivacyHost::Number(&env_, 1);
        return result;
    }
    napi_value Invoke(const char *name, std::vector<napi_value> args)
    {
        auto promise = PrivacyHost::Call(&env_, name, std::move(args));
        PrivacyHost::Drain(&env_);
        return promise;
    }
    napi_env__ env_;
};

HWTEST_F(PrivacyBoundaryTest, NapiPsiAndPirRecoverThroughSa, TestSize.Level1)
{
    for (bool pir : {false, true}) {
        auto protocol = Protocol(pir);
        auto generated = Invoke("genPrivacyTarget", {Target(), protocol});
        ASSERT_NE(generated, nullptr);
        ASSERT_TRUE(generated->settled);
        ASSERT_FALSE(generated->rejected);
        auto element = PrivacyHost::Value(&env_);
        element->properties["elemKey"] = PrivacyHost::Bytes(&env_, psi_pir::Bytes(8, 0x45));
        element->properties["hashAlg"] = PrivacyHost::Number(&env_, 1);
        if (pir) {
            element->properties["elemValue"] = PrivacyHost::Bytes(&env_, {0, 0xFF, 0x45});
        }
        auto search = Invoke("privacySearch", {generated->result, PrivacyHost::Array(&env_, {element}), protocol});
        ASSERT_NE(search, nullptr);
        ASSERT_FALSE(search->rejected);
        ASSERT_NE(search->result->properties.count("resultCipherText"), 0U);
        auto result = Invoke("getSearchResult", {search->result, protocol});
        ASSERT_NE(result, nullptr);
        ASSERT_FALSE(result->rejected);
        EXPECT_TRUE(result->result->properties.at("matchedResult")->boolean);
        if (pir) {
            auto values = result->result->properties.at("attachedValues");
            ASSERT_EQ(values->elements.size(), 1U);
            EXPECT_EQ(values->elements[0]->buffer->storage, (psi_pir::Bytes{0, 0xFF, 0x45}));
        }
    }
    EXPECT_EQ(env_.deletedWorks, 6U);
}

HWTEST_F(PrivacyBoundaryTest, NapiRejectsMissingArgumentsAndMalformedObjects, TestSize.Level2)
{
    for (const auto name : {"genPrivacyTarget", "privacySearch", "getSearchResult"}) {
        auto result = Invoke(name, {});
        ASSERT_NE(result, nullptr);
        ASSERT_TRUE(result->rejected);
        EXPECT_EQ(result->result->properties.at("code")->number, PRIVACY_COMPUTATION_INVALID_ARGUMENT);
    }
    for (auto value : {PrivacyHost::Value(&env_), PrivacyHost::Number(&env_, 1)}) {
        auto result = Invoke("genPrivacyTarget", {value, Protocol()});
        ASSERT_TRUE(result->rejected);
    }
}

HWTEST_F(PrivacyBoundaryTest, NumericAndEnumBoundariesAreChecked, TestSize.Level2)
{
    for (double invalid :
         {-1.0, 1.5, 4294967296.0, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()}) {
        auto target = Target();
        target->properties["hashAlg"] = PrivacyHost::Number(&env_, invalid);
        EXPECT_TRUE(Invoke("genPrivacyTarget", {target, Protocol()})->rejected);
    }
    for (uint32_t invalid : {3U, 255U}) {
        auto protocol = Protocol();
        protocol->properties["dataSetSize"] = PrivacyHost::Number(&env_, invalid);
        EXPECT_TRUE(Invoke("genPrivacyTarget", {Target(), protocol})->rejected);
    }
    auto target = Target();
    target->properties["hashAlg"] = PrivacyHost::Number(&env_, 4);
    EXPECT_TRUE(Invoke("genPrivacyTarget", {target, Protocol()})->rejected);
    target = Target();
    target->properties["hashAlg"] = PrivacyHost::Value(&env_, napi_string);
    EXPECT_TRUE(Invoke("genPrivacyTarget", {target, Protocol()})->rejected);
}

HWTEST_F(PrivacyBoundaryTest, ByteArrayBoundariesAndOptionalHashes, TestSize.Level2)
{
    for (uint32_t hash = 0; hash <= 3; ++hash) {
        auto target = Target();
        target->properties["hashAlg"] = PrivacyHost::Number(&env_, hash);
        EXPECT_FALSE(Invoke("genPrivacyTarget", {target, Protocol()})->rejected);
    }
    auto target = Target();
    target->properties.erase("hashAlg");
    EXPECT_FALSE(Invoke("genPrivacyTarget", {target, Protocol()})->rejected);
    for (uint32_t mode = 0; mode < 4; ++mode) {
        auto bytes = PrivacyHost::Bytes(&env_, {1});
        if (mode == 0) {
            bytes->typed = false;
        }
        if (mode == 1) {
            bytes->typedKind = napi_int32_array;
        }
        if (mode == 2) {
            bytes->storage.clear();
        }
        if (mode == 3) {
            bytes->count = MAX_PRIVACY_BYTES + 1;
        }
        target->properties["elemData"] = bytes;
        EXPECT_TRUE(Invoke("genPrivacyTarget", {target, Protocol()})->rejected);
    }
}

HWTEST_F(PrivacyBoundaryTest, ParseSearchArraysAndValueArrays, TestSize.Level2)
{
    std::vector<Element> elements;
    EXPECT_NE(NapiGetProperty(&env_, PrivacyHost::Value(&env_), elements), napi_ok);
    env_.exception = nullptr;
    auto missing = PrivacyHost::Value(&env_);
    EXPECT_NE(NapiGetProperty(&env_, PrivacyHost::Array(&env_, {missing}), elements), napi_ok);
    env_.exception = nullptr;
    PrivacySearchResult result;
    auto object = PrivacyHost::Value(&env_);
    object->properties["resultCipherText"] = PrivacyHost::Array(&env_, {PrivacyHost::Bytes(&env_, {1, 2})});
    object->properties["valueCipherText"] = PrivacyHost::Array(&env_, {PrivacyHost::Bytes(&env_, {})});
    ASSERT_EQ(NapiGetProperty(&env_, object, result), napi_ok);
    EXPECT_EQ(result.resultCipherText, (std::vector<psi_pir::Bytes>{{1, 2}}));
    ASSERT_TRUE(result.valueCipherText.has_value());
    EXPECT_TRUE(result.valueCipherText->front().empty());
    object->properties["valueCipherText"] = PrivacyHost::Number(&env_, 1);
    EXPECT_NE(NapiGetProperty(&env_, object, result), napi_ok);
}

HWTEST_F(PrivacyBoundaryTest, AsyncSetupAndCompletionFailuresReleaseWork, TestSize.Level2)
{
    for (const auto operation : {"cb_info", "create_work", "queue_work"}) {
        PrivacyHost::Reset();
        State().failAt = operation;
        const auto deleted = env_.deletedWorks;
        auto promise = Invoke("genPrivacyTarget", {Target(), Protocol()});
        ASSERT_NE(promise, nullptr) << operation;
        EXPECT_TRUE(promise->rejected) << operation;
        EXPECT_TRUE(env_.queued.empty());
        EXPECT_EQ(env_.deletedWorks - deleted, std::strcmp(operation, "queue_work") == 0 ? 1U : 0U);
    }
    PrivacyHost::Reset();
    env_.completion = napi_cancelled;
    EXPECT_TRUE(Invoke("genPrivacyTarget", {Target(), Protocol()})->rejected);
    env_.completion = napi_ok;
    auto promise = PrivacyHost::Call(&env_, "genPrivacyTarget", {Target(), Protocol()});
    State().failAt = "create_buffer";
    PrivacyHost::Drain(&env_);
    EXPECT_TRUE(promise->rejected);
}

HWTEST_F(PrivacyBoundaryTest, GetterExceptionIsPreservedAndErrorsAreMapped, TestSize.Level2)
{
    auto original = PrivacyHost::Value(&env_);
    env_.exception = original;
    EXPECT_EQ(ThrowJsError(&env_, 401, "ignored"), napi_pending_exception);
    auto promise = CreateRejectedPromise(&env_, 401, "ignored");
    ASSERT_NE(promise, nullptr);
    EXPECT_EQ(promise->result, original);
    EXPECT_EQ(env_.exception, nullptr);
    for (const auto code : {PSI_PIR_SUCCESS, PSI_PIR_INVALID_ARGUMENT, PSI_PIR_CODEC_ERROR, PSI_PIR_HASH_ERROR,
                            PSI_PIR_RANDOM_ERROR, PSI_PIR_CRYPTO_ERROR, PSI_PIR_UNSUPPORTED_PROTOCOL,
                            PSI_PIR_INCOMPATIBLE_HE_PARAMETERS, PSI_PIR_HE_ERROR, PSI_PIR_INTERNAL_ERROR}) {
        const auto mapped = MapProtocolError(code);
        EXPECT_NE(GetErrorMessage(mapped), nullptr);
        EXPECT_EQ(mapped == 0, code == 0);
    }
    for (const auto code : {PRIVACY_COMPUTATION_OUT_OF_MEMORY, PRIVACY_COMPUTATION_ACCOUNT_ERROR}) {
        EXPECT_NE(GetErrorMessage(code), nullptr);
    }
    EXPECT_EQ(CheckNapiStatus(&env_, napi_invalid_arg), napi_invalid_arg);
    EXPECT_NE(env_.exception, nullptr);
}

HWTEST_F(PrivacyBoundaryTest, SaValidatesPointersIdentityAndOutputCapacity, TestSize.Level2)
{
    psi_pir::Bytes storage(MAX_TARGET_RESULT_BYTES);
    PrivacyBuffer output{storage.data(), static_cast<uint32_t>(storage.size())};
    psi_pir::Bytes element(8, 0x45);
    PrivacyTargetRequest input{{2, 0}, 1, {element.data(), static_cast<uint32_t>(element.size())}};
    EXPECT_EQ(AssetGenPrivacyTarget(1, nullptr, &output), PSI_PIR_INVALID_ARGUMENT);
    EXPECT_EQ(AssetGenPrivacyTarget(1, &input, nullptr), PSI_PIR_INVALID_ARGUMENT);
    EXPECT_EQ(AssetGenPrivacyTarget(UINT32_MAX, &input, &output), PSI_PIR_INVALID_ARGUMENT);
    State().accountStatus = -1;
    EXPECT_EQ(AssetGenPrivacyTarget(1, &input, &output), PSI_PIR_INTERNAL_ERROR);
    State().accountStatus = 0;
    State().userId = -1;
    EXPECT_EQ(AssetGenPrivacyTarget(1, &input, &output), PSI_PIR_INTERNAL_ERROR);
    State().userId = 100;
    output.size = 1;
    EXPECT_EQ(AssetGenPrivacyTarget(1, &input, &output), PSI_PIR_CODEC_ERROR);
    output.size = static_cast<uint32_t>(storage.size());
    input.hashAlg = UINT32_MAX;
    EXPECT_EQ(AssetGenPrivacyTarget(1, &input, &output), PSI_PIR_INVALID_ARGUMENT);
    input.hashAlg = 1;
    input.protocol.dataSetSize = UINT32_MAX;
    EXPECT_EQ(AssetGenPrivacyTarget(1, &input, &output), PSI_PIR_INVALID_ARGUMENT);
    input.protocol = {2, 0};
    input.element.data = nullptr;
    EXPECT_EQ(AssetGenPrivacyTarget(1, &input, &output), PSI_PIR_INVALID_ARGUMENT);
    PrivacyDecryptRequest request{{2, 0}, {element.data(), 0}};
    EXPECT_EQ(AssetDecryptPrivacyCiphertext(1, nullptr, &output), PSI_PIR_INVALID_ARGUMENT);
    EXPECT_EQ(AssetDecryptPrivacyCiphertext(1, &request, &output), PSI_PIR_INVALID_ARGUMENT);
}

HWTEST_F(PrivacyBoundaryTest, SaPropagatesHuksAndHeFailuresWithoutCommittingOutput, TestSize.Level2)
{
    for (const auto failure : {"New", "SetPara", "KeyGen", "Encode", "Encrypt", "SerializeQuery", "Serialize"}) {
        PrivacyHost::Reset();
        State().failAt = failure;
        psi_pir::Bytes output{0x99};
        EXPECT_NE(GenPrivacyTargetRemote({psi_pir::Bytes(8, 0x45), HashAlg::SHA256},
                                         {DataSetSize::SIZE_512, ProtocolType::PSI_PROTOCOL}, output),
                  PSI_PIR_SUCCESS);
        EXPECT_EQ(output, (psi_pir::Bytes{0x99}));
    }
    PrivacyHost::Reset();
    State().huksInitStatus = -1;
    EXPECT_TRUE(Invoke("genPrivacyTarget", {Target(), Protocol()})->rejected);
    PrivacyHost::Reset();
    State().keyExistsStatus = -1;
    EXPECT_TRUE(Invoke("genPrivacyTarget", {Target(), Protocol()})->rejected);
}

HWTEST_F(PrivacyBoundaryTest, DrbgInitializationStatus, TestSize.Level2)
{
    const char *mode = std::getenv("PRIVACY_TEST_DRBG");
    const bool failed = mode != nullptr && std::strcmp(mode, "failure") == 0;
    HifheAdapter adapter;
    EXPECT_EQ(adapter.InitializeEvaluation(ProtocolParams{}), failed ? PSI_PIR_HE_ERROR : PSI_PIR_SUCCESS);
}

HWTEST_F(PrivacyBoundaryTest, EachBatchImportsOnceAndExportsOnlyAfterEvaluation, TestSize.Level1)
{
    const std::vector<psi_pir::PrivacyProtocol> protocols{{DataSetSize::SIZE_512, ProtocolType::PSI_PROTOCOL},
                                                          {DataSetSize::SIZE_256, ProtocolType::PIR_PROTOCOL},
                                                          {DataSetSize::SIZE_128, ProtocolType::PIR_PROTOCOL}};
    for (const auto &protocol : protocols) {
        ProtocolParams params;
        ASSERT_EQ(MapPrivacyProtocol(protocol, params), PSI_PIR_SUCCESS);
        const bool pir = protocol.protocolType == ProtocolType::PIR_PROTOCOL;
        const size_t capacity = params.dataNumPerPlaintext / (pir ? kPirGroupsPerItem : kPsiGroupsPerItem);
        const Element element{psi_pir::Bytes(8, 0x45), HashAlg::SHA256, psi_pir::Bytes{0xFF}};
        const std::vector<Element> elements(capacity + 1, element);
        psi_pir::Bytes target;
        ASSERT_EQ(GenPrivacyTarget({element.elemKey, element.hashAlg}, protocol, {20010001, 100}, target),
                  PSI_PIR_SUCCESS);
        for (bool custom : {false, true}) {
            SetHEEngine(custom ? std::make_shared<const SoftHeEngine>() : nullptr);
            State().calls.clear();
            PrivacySearchResult output;
            ASSERT_EQ(PrivacySearch(target, elements, protocol, output), PSI_PIR_SUCCESS);
            ASSERT_EQ(output.resultCipherText.size(), 2U);
            EXPECT_EQ(State().calls["Deserialize"], 2U);
            EXPECT_EQ(State().calls["SerializeQuery"], 2U);
            EXPECT_EQ(State().calls["Serialize"], 2U);
            EXPECT_EQ(State().calls["New"], 2U);
        }
    }
}

HWTEST_F(PrivacyBoundaryTest, FailedBatchNeverSerializesIntermediateResults, TestSize.Level2)
{
    const psi_pir::PrivacyProtocol protocol{DataSetSize::SIZE_256, ProtocolType::PIR_PROTOCOL};
    const Element element{psi_pir::Bytes(8, 0x45), HashAlg::SHA256, psi_pir::Bytes{0xFF}};
    psi_pir::Bytes target;
    ASSERT_EQ(GenPrivacyTarget({element.elemKey, element.hashAlg}, protocol, {20010001, 100}, target), PSI_PIR_SUCCESS);
    SetHEEngine(std::make_shared<const SoftHeEngine>());
    const std::vector<std::pair<const char *, uint32_t>> failures{
        {"Deserialize", 1}, {"Encode", 1}, {"Negate", 1}, {"Add", 1},        {"Mul", 1},
        {"Encode", 3},      {"Negate", 2}, {"Add", 2},    {"Rerandomize", 1}};
    for (const auto &[operation, occurrence] : failures) {
        State().calls.clear();
        State().failAt = operation;
        State().failOccurrence = occurrence;
        PrivacySearchResult output{{psi_pir::Bytes{0x99}}, std::vector<psi_pir::Bytes>{{0x88}}};
        EXPECT_EQ(PrivacySearch(target, {element}, protocol, output), PSI_PIR_HE_ERROR);
        EXPECT_EQ(State().calls["SerializeQuery"], 0U) << operation;
        EXPECT_EQ(State().calls["Serialize"], 0U) << operation;
        EXPECT_EQ(output.resultCipherText, (std::vector<psi_pir::Bytes>{{0x99}}));
        ASSERT_TRUE(output.valueCipherText.has_value());
        EXPECT_EQ(*output.valueCipherText, (std::vector<psi_pir::Bytes>{{0x88}}));
    }
}

HWTEST_F(PrivacyBoundaryTest, FinalExportFailurePreservesOutput, TestSize.Level2)
{
    const psi_pir::PrivacyProtocol protocol{DataSetSize::SIZE_256, ProtocolType::PIR_PROTOCOL};
    const Element element{psi_pir::Bytes(8, 0x45), HashAlg::SHA256, psi_pir::Bytes{0xFF}};
    psi_pir::Bytes target;
    ASSERT_EQ(GenPrivacyTarget({element.elemKey, element.hashAlg}, protocol, {20010001, 100}, target), PSI_PIR_SUCCESS);
    for (const auto operation : {"SerializeQuery", "Serialize"}) {
        State().calls.clear();
        State().failAt = operation;
        PrivacySearchResult output{{psi_pir::Bytes{0x99}}, std::nullopt};
        EXPECT_EQ(PrivacySearch(target, {element}, protocol, output), PSI_PIR_HE_ERROR);
        EXPECT_EQ(output.resultCipherText, (std::vector<psi_pir::Bytes>{{0x99}}));
        EXPECT_FALSE(output.valueCipherText.has_value());
        EXPECT_EQ(State().calls["Deserialize"], 1U);
        EXPECT_EQ(State().calls["Add"], 2U);
        EXPECT_EQ(State().calls["Mul"], 1U);
        EXPECT_EQ(State().calls["Rerandomize"], 1U);
        EXPECT_EQ(State().calls["SerializeQuery"], 1U);
    }
}
} // namespace
