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
#include "privacy_client.h"
#include "huks_seed_deriver.h"
#include "he_key_deriver.h"

namespace {
using namespace testing::ext;
using namespace OHOS::Security::PrivacyComputation;
using namespace psi_pir;
using namespace psi_pir::internal;
class ClosedLoaderTest : public testing::Test {
public:
    void SetUp() override
    {
        PrivacyHost::Reset();
        SetHeSeedDeriver(std::make_shared<HuksSeedDeriver>());
    }
    napi_env__ env_;
};

HWTEST_F(ClosedLoaderTest, HardRouteUsesLocalPostprocessingAndRegisteredEntry, TestSize.Level1)
{
    const psi_pir::PrivacyProtocol protocol{DataSetSize::SIZE_256, ProtocolType::PIR_PROTOCOL};
    psi_pir::Bytes target;
    ASSERT_EQ(GenPrivacyTargetRemote({psi_pir::Bytes(8, 0x45), HashAlg::SHA256}, protocol, target), PSI_PIR_SUCCESS);
    PrivacySearchResult encrypted;
    ASSERT_EQ(PrivacySearch(target, {{psi_pir::Bytes(8, 0x45), HashAlg::SHA256, psi_pir::Bytes{0, 0xFF}}}, protocol,
                            encrypted),
              PSI_PIR_SUCCESS);
    napi_value jsInput = nullptr;
    ASSERT_EQ(NapiSetProperty(&env_, encrypted, jsInput), napi_ok);
    auto jsProtocol = PrivacyHost::Value(&env_);
    jsProtocol->properties["dataSetSize"] = PrivacyHost::Number(&env_, 1);
    jsProtocol->properties["protocolType"] = PrivacyHost::Number(&env_, 1);
    auto promise = PrivacyHost::Call(&env_, "getSearchResult", {jsInput, jsProtocol});
    PrivacyHost::Drain(&env_);
    ASSERT_NE(promise, nullptr);
    ASSERT_FALSE(promise->rejected);
    EXPECT_TRUE(promise->result->properties.at("matchedResult")->boolean);
    const auto values = promise->result->properties.at("attachedValues");
    ASSERT_EQ(values->elements.size(), 1U);
    EXPECT_EQ(values->elements[0]->buffer->storage, (psi_pir::Bytes{0, 0xFF}));
    // The first entry delegates to the configured closed NAPI callback.
    auto first = PrivacyHost::Call(&env_, "genPrivacyTarget", {});
    ASSERT_NE(first, nullptr);
    EXPECT_TRUE(first->rejected);
    PrivacyHost::State().closedStatus = PRIVACY_COMPUTATION_CRYPTO_ERROR;
    promise = PrivacyHost::Call(&env_, "getSearchResult", {jsInput, jsProtocol});
    PrivacyHost::Drain(&env_);
    ASSERT_NE(promise, nullptr);
    EXPECT_TRUE(promise->rejected);
}
} // namespace
