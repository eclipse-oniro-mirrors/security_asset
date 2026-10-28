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
#include "psi_pir_protocol.h"
#include "soft_he_engine.h"
#include <atomic>
#include <gtest/gtest.h>
#include <thread>

namespace {
using namespace testing::ext;
using namespace psi_pir;
using namespace psi_pir::internal;
constexpr PrivacyProtocol PSI{DataSetSize::SIZE_512, ProtocolType::PSI_PROTOCOL};

class PsiPirHeTest : public testing::Test {
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
        return PSI_PIR_SUCCESS;
    }
};

HWTEST_F(PsiPirHeTest, SeedRequiresServiceProviderAndBindsIdentity, TestSize.Level1)
{
    SetHeSeedDeriver(nullptr);
    HeSeed first;
    first.bytes.fill(0x99);
    EXPECT_EQ(HeKeyDeriver().DeriveKey({1000, 100}, first), PSI_PIR_HE_ERROR);
    EXPECT_EQ(first.bytes[0], 0x99);
    const auto provider = std::make_shared<RecordingSeedDeriver>();
    SetHeSeedDeriver(provider);
    HeSeed repeated;
    HeSeed other;
    ASSERT_EQ(HeKeyDeriver().DeriveKey({1000, 100}, first), PSI_PIR_SUCCESS);
    ASSERT_EQ(HeKeyDeriver().DeriveKey({1000, 100}, repeated), PSI_PIR_SUCCESS);
    EXPECT_EQ(first.bytes, repeated.bytes);
    ASSERT_EQ(HeKeyDeriver().DeriveKey({1001, 100}, other), PSI_PIR_SUCCESS);
    EXPECT_NE(first.bytes, other.bytes);
    ASSERT_EQ(HeKeyDeriver().DeriveKey({1000, 101}, other), PSI_PIR_SUCCESS);
    EXPECT_NE(first.bytes, other.bytes);
    EXPECT_EQ(provider->lastIdentity.uid, 1000U);
    EXPECT_EQ(provider->lastIdentity.userId, 101U);
}

/* @tc.name: FailedOperationsPreserveOutputs
 * @tc.desc: Uninitialized operations and missing seed providers never commit partial results.
 * @tc.type: FUNC
 */
HWTEST_F(PsiPirHeTest, FailedOperationsPreserveOutputs, TestSize.Level1)
{
    HifheAdapter adapter;
    Bytes output{0x99};
    SetHeSeedDeriver(nullptr);
    EXPECT_EQ(adapter.AddPlaintext(Bytes{1}, Bytes{2}, output), PSI_PIR_HE_ERROR);
    EXPECT_EQ(output, Bytes({0x99}));
    EXPECT_EQ(genPrivacyTarget(TargetInput{Bytes{'a'}, HashAlg::SHA256}, PSI, {1000, 100}, output),
              PSI_PIR_HE_ERROR);
    EXPECT_EQ(output, Bytes({0x99}));
    PrivacySearchResult encrypted{{Bytes{1}}, std::nullopt};
    SearchResult result{true, std::vector<Bytes>{Bytes{0x99}}};
    EXPECT_EQ(getSearchResult(encrypted, PSI, {1000, 100}, result), PSI_PIR_HE_ERROR);
    EXPECT_TRUE(result.matchedResult);
    ASSERT_TRUE(result.attachedValues.has_value());
    EXPECT_EQ(*result.attachedValues, std::vector<Bytes>({Bytes{0x99}}));
    EXPECT_EQ(encrypted.fheCipherText, std::vector<Bytes>({Bytes{1}}));
    PrivacySearchResult searchOutput{{Bytes{0x99}}, std::nullopt};
    EXPECT_EQ(privacySearch(Bytes{1}, {Element{Bytes{'a'}, HashAlg::SHA256, std::nullopt}}, PSI, searchOutput),
              PSI_PIR_HE_ERROR);
    EXPECT_EQ(searchOutput.fheCipherText, std::vector<Bytes>({Bytes{0x99}}));
}

/* @tc.name: CompleteResultEntryValidatesProtocol
 * @tc.desc: Invalid protocols preserve output; empty inputs still execute result processing.
 * @tc.type: FUNC
 */
HWTEST_F(PsiPirHeTest, CompleteResultEntryValidatesProtocol, TestSize.Level1)
{
    SearchResult result{true, std::vector<Bytes>{Bytes{0x99}}};
    const PrivacyProtocol invalid{static_cast<DataSetSize>(-1), ProtocolType::PSI_PROTOCOL};
    EXPECT_EQ(getSearchResult({}, invalid, {1000, 100}, result), PSI_PIR_INVALID_ARGUMENT);
    EXPECT_TRUE(result.matchedResult);
    ASSERT_TRUE(result.attachedValues.has_value());
    EXPECT_EQ(*result.attachedValues, std::vector<Bytes>({Bytes{0x99}}));

    // Empty input needs no HE operation, but still runs result processing.
    EXPECT_EQ(getSearchResult({}, PSI, {1000, 100}, result), PSI_PIR_SUCCESS);
    EXPECT_FALSE(result.matchedResult);
    EXPECT_FALSE(result.attachedValues.has_value());
}

/* @tc.name: DefaultFactoryNeverOverwritesCustom
 * @tc.desc: Registration/clear racing with default installation retains the explicit provider.
 * @tc.type: FUNC
 */
HWTEST_F(PsiPirHeTest, DefaultFactoryNeverOverwritesCustom, TestSize.Level1)
{
    ASSERT_EQ(EnsureDefaultHeEngineInstalled(), PSI_PIR_SUCCESS);
    auto custom = std::make_shared<const SoftHeEngine>();
    SetHEEngine(custom);
    const auto snapshot = GetHEEngine();
    std::atomic<bool> good{true};
    std::thread install([&good]() {
        for (int i = 0; i < 1000; ++i) {
            if (EnsureDefaultHeEngineInstalled() != PSI_PIR_SUCCESS || !GetHEEngine()) {
                good.store(false);
            }
        }
    });
    for (int i = 0; i < 1000; ++i) {
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
