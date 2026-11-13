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

#include "he_key_deriver.h"
#include "hifhe_adapter.h"
#include "provider_registry.h"
#include "privacy_protocol.h"
#include "soft_he_engine.h"
#include <new>
#include <atomic>
#include <gtest/gtest.h>
#include <thread>

namespace {
using namespace testing::ext;
using namespace OHOS::Security::PrivacyComputation;
using namespace OHOS::Security::PrivacyComputation::internal;
constexpr PrivacyProtocol intersection{DataSetSize::SIZE_512, ProtocolType::PSI_PROTOCOL};

class PrivacyHeTest : public testing::Test {
public:
    void TearDown() override
    {
        SetHEEngine(nullptr);
        SetHeSeedDeriver(nullptr);
    }
};

// Test-only provider proves explicit injection and identity binding; it is not a production KDF.
class RecordingSeedDeriver final : public IHeSeedDeriver {
public:
    mutable CallerIdentity lastIdentity{};
    int32_t Derive(const CallerIdentity &identity, const std::array<uint8_t, 32> &hash,
                   HeSeed &seed) const noexcept override
    {
        lastIdentity = identity;
        seed.bytes = hash;
        seed.bytes[0] ^= 0xa5;
        return PRIVACY_PROTOCOL_SUCCESS;
    }
};

HWTEST_F(PrivacyHeTest, SeedRequiresServiceProviderAndBindsIdentity, TestSize.Level1)
{
    SetHeSeedDeriver(nullptr);
    HeSeed first;
    first.bytes.fill(0x99);
    EXPECT_EQ(HeKeyDeriver().DeriveKey({1000, 100}, first), PRIVACY_PROTOCOL_HE_ERROR);
    EXPECT_EQ(first.bytes[0], 0x99);
    const auto provider = std::shared_ptr<RecordingSeedDeriver>(new (std::nothrow) RecordingSeedDeriver());
    ASSERT_NE(provider, nullptr);
    SetHeSeedDeriver(provider);
    HeSeed repeated;
    HeSeed other;
    ASSERT_EQ(HeKeyDeriver().DeriveKey({1000, 100}, first), PRIVACY_PROTOCOL_SUCCESS);
    ASSERT_EQ(HeKeyDeriver().DeriveKey({1000, 100}, repeated), PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_EQ(first.bytes, repeated.bytes);
    ASSERT_EQ(HeKeyDeriver().DeriveKey({1001, 100}, other), PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_NE(first.bytes, other.bytes);
    ASSERT_EQ(HeKeyDeriver().DeriveKey({1000, 101}, other), PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_NE(first.bytes, other.bytes);
    EXPECT_EQ(provider->lastIdentity.uid, 1000U);
    EXPECT_EQ(provider->lastIdentity.userId, 101U);
}

/* @tc.name: FailedOperationsPreserveOutputs
 * @tc.desc: Uninitialized operations and missing seed providers never commit partial results.
 * @tc.type: FUNC
 */
HWTEST_F(PrivacyHeTest, FailedOperationsPreserveOutputs, TestSize.Level1)
{
    HifheAdapter adapter;
    Bytes output{0x99};
    SetHeSeedDeriver(nullptr);
    HifheText plaintext;
    EXPECT_EQ(adapter.Encode(Slots{}, plaintext), PRIVACY_PROTOCOL_HE_ERROR);
    EXPECT_EQ(output, Bytes({0x99}));
    EXPECT_EQ(GenPrivacyTarget(TargetInput{Bytes{'a'}, HashAlg::SHA256}, intersection, {1000, 100}, output),
              PRIVACY_PROTOCOL_HE_ERROR);
    EXPECT_EQ(output, Bytes({0x99}));
    PrivacySearchResult encrypted{{Bytes{1}}, std::nullopt};
    SearchResult result{true, std::vector<Bytes>{Bytes{0x99}}};
    EXPECT_EQ(DecryptCiphertext(encrypted.resultCipherText.front(), intersection, {1000, 100}, output),
              PRIVACY_PROTOCOL_HE_ERROR);
    EXPECT_EQ(output, Bytes({0x99}));
    EXPECT_TRUE(result.matchedResult);
    ASSERT_TRUE(result.attachedValues.has_value());
    EXPECT_EQ(*result.attachedValues, std::vector<Bytes>({Bytes{0x99}}));
    EXPECT_EQ(encrypted.resultCipherText, std::vector<Bytes>({Bytes{1}}));
    PrivacySearchResult searchOutput{{Bytes{0x99}}, std::nullopt};
    EXPECT_EQ(PrivacySearch(Bytes{1}, {Element{Bytes{'a'}, HashAlg::SHA256, std::nullopt}}, intersection, searchOutput),
              PRIVACY_PROTOCOL_HE_ERROR);
    EXPECT_EQ(searchOutput.resultCipherText, std::vector<Bytes>({Bytes{0x99}}));
}

/* @tc.name: CompleteResultEntryValidatesProtocol
 * @tc.desc: Invalid protocols preserve output; empty inputs still execute result processing.
 * @tc.type: FUNC
 */
HWTEST_F(PrivacyHeTest, CompleteResultEntryValidatesProtocol, TestSize.Level1)
{
    SearchResult result{true, std::vector<Bytes>{Bytes{0x99}}};
    const PrivacyProtocol invalid{static_cast<DataSetSize>(-1), ProtocolType::PSI_PROTOCOL};
    EXPECT_EQ(PostProcessSearchResult({}, std::nullopt, invalid, result), PRIVACY_PROTOCOL_INVALID_ARGUMENT);
    EXPECT_TRUE(result.matchedResult);
    ASSERT_TRUE(result.attachedValues.has_value());
    EXPECT_EQ(*result.attachedValues, std::vector<Bytes>({Bytes{0x99}}));

    // Empty input needs no HE operation, but still runs result processing.
    SetHeSeedDeriver(nullptr);
    EXPECT_EQ(PostProcessSearchResult({}, std::nullopt, intersection, result), PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_FALSE(result.matchedResult);
    EXPECT_FALSE(result.attachedValues.has_value());
}

/* @tc.name: DefaultFactoryNeverOverwritesCustom
 * @tc.desc: Registration/clear racing with default installation retains the explicit provider.
 * @tc.type: FUNC
 */
HWTEST_F(PrivacyHeTest, DefaultFactoryNeverOverwritesCustom, TestSize.Level1)
{
    ASSERT_EQ(EnsureDefaultHeEngineInstalled(), PRIVACY_PROTOCOL_SUCCESS);
    auto custom = std::shared_ptr<const SoftHeEngine>(new (std::nothrow) const SoftHeEngine());
    ASSERT_NE(custom, nullptr);
    SetHEEngine(custom);
    const auto snapshot = GetHEEngine();
    std::atomic<bool> good{true};
    std::thread install([&good]() {
        for (int32_t i = 0; i < 1000; ++i) {
            if (EnsureDefaultHeEngineInstalled() != PRIVACY_PROTOCOL_SUCCESS || !GetHEEngine()) {
                good.store(false);
            }
        }
    });
    for (int32_t i = 0; i < 1000; ++i) {
        SetHEEngine(nullptr);
        SetHEEngine(custom);
    }
    install.join();
    EXPECT_TRUE(good.load());
    EXPECT_EQ(GetHEEngine(), custom);
    SetHEEngine(nullptr);
    EXPECT_EQ(GetHEEngine(), GetSoftHeEngine());
    EXPECT_EQ(snapshot, custom);
}
} // namespace
