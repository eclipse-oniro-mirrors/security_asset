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

namespace {
using namespace testing::ext;
using namespace OHOS::Security::PrivacyComputation;
class ClosedLoaderTest : public testing::Test {
public:
    void SetUp() override
    {
        PrivacyHost::Reset();
    }
    napi_env__ env_;
};

HWTEST_F(ClosedLoaderTest, OpenFailure, TestSize.Level2)
{
    PrivacyHost::State().dlopenFailure = true;
    EXPECT_TRUE(IsComputationLibConfigured());
    for (const auto function : {"GenPrivacyTarget", "GetSearchResult"}) {
        auto promise = CallComputationNapiFunc(&env_, nullptr, function);
        ASSERT_NE(promise, nullptr);
        EXPECT_TRUE(promise->rejected);
        EXPECT_EQ(promise->result->properties.at("code")->number, PRIVACY_COMPUTATION_UNAVAILABLE);
    }
}

HWTEST_F(ClosedLoaderTest, SymbolsAndClosedErrorsRejectPromise, TestSize.Level2)
{
    PrivacyHost::State().symbolFailure = true;
    for (const auto function : {"GenPrivacyTarget", "GetSearchResult"}) {
        auto promise = CallComputationNapiFunc(&env_, nullptr, function);
        ASSERT_NE(promise, nullptr);
        EXPECT_TRUE(promise->rejected);
        EXPECT_EQ(promise->result->properties.at("code")->number, PRIVACY_COMPUTATION_UNAVAILABLE);
    }
    PrivacyHost::State().symbolFailure = false;
    PrivacyHost::State().closedStatus = PRIVACY_COMPUTATION_CRYPTO_ERROR;
    auto promise = PrivacyHost::Call(&env_, "getSearchResult", {});
    ASSERT_NE(promise, nullptr);
    EXPECT_TRUE(promise->rejected);
    EXPECT_EQ(promise->result->properties.at("code")->number, PRIVACY_COMPUTATION_CRYPTO_ERROR);
}

HWTEST_F(ClosedLoaderTest, HardRouteDelegatesToClosedNapi, TestSize.Level1)
{
    auto first = PrivacyHost::Call(&env_, "genPrivacyTarget", {});
    ASSERT_NE(first, nullptr);
    ASSERT_TRUE(first->settled);
    ASSERT_FALSE(first->rejected);
    EXPECT_EQ(first->result->text, "ClosedGenPrivacyTarget");
    auto third = PrivacyHost::Call(&env_, "getSearchResult", {});
    ASSERT_NE(third, nullptr);
    ASSERT_TRUE(third->settled);
    ASSERT_FALSE(third->rejected);
    EXPECT_EQ(third->result->text, "ClosedGetSearchResult");
    EXPECT_TRUE(env_.queued.empty());
    EXPECT_EQ(PrivacyHost::State().calls["ClosedGenPrivacyTarget"], 1U);
    EXPECT_EQ(PrivacyHost::State().calls["ClosedGetSearchResult"], 1U);
    EXPECT_EQ(PrivacyHost::State().calls["Deserialize"], 0U);
}
} // namespace
