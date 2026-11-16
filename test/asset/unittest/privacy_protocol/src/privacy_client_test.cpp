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

#include "privacy_client.h"
#include "privacy_client_ffi.h"

#include <algorithm>
#include <gtest/gtest.h>

namespace {
using namespace OHOS::Security::PrivacyComputation;
using namespace testing::ext;

std::vector<Bytes> g_received;
size_t g_failBatch = 0;
size_t g_postprocessCalls = 0;
bool g_invalidPlaintext = false;
std::optional<uint32_t> g_targetResponseSize;
const std::optional<std::vector<Bytes>> *g_localSymmetric = nullptr;

class PrivacyClientTest : public testing::Test {
public:
    void SetUp() override
    {
        g_received.clear();
        g_failBatch = 0;
        g_postprocessCalls = 0;
        g_invalidPlaintext = false;
        g_targetResponseSize.reset();
        g_localSymmetric = nullptr;
    }
};
} // namespace

extern "C" int32_t asset_privacy_gen_target(const PrivacyTargetRequest *, PrivacyBuffer *output)
{
    if (!g_targetResponseSize.has_value()) {
        return PRIVACY_PROTOCOL_UNSUPPORTED_PROTOCOL;
    }
    output->size = *g_targetResponseSize;
    return PRIVACY_PROTOCOL_SUCCESS;
}

extern "C" int32_t asset_privacy_decrypt_ciphertext(const PrivacyDecryptRequest *input, PrivacyBuffer *output)
{
    g_received.emplace_back(input->ciphertext.data, input->ciphertext.data + input->ciphertext.size);
    if (g_failBatch != 0 && g_received.size() == g_failBatch) {
        return PRIVACY_PROTOCOL_HE_ERROR;
    }
    std::fill_n(output->data, MAX_DECRYPT_RESULT_BYTES, static_cast<uint8_t>(g_received.size()));
    output->size = g_invalidPlaintext ? 1 : MAX_DECRYPT_RESULT_BYTES;
    return PRIVACY_PROTOCOL_SUCCESS;
}

namespace OHOS::Security::PrivacyComputation {
// This suite isolates client orchestration; the protocol suite tests actual postprocessing.
int32_t PostProcessSearchResult(const std::vector<Bytes> &plaintexts,
                                const std::optional<std::vector<Bytes>> &symmetric, const PrivacyProtocol &,
                                SearchResult &output) noexcept
{
    ++g_postprocessCalls;
    EXPECT_EQ(&symmetric, g_localSymmetric);
    EXPECT_EQ(plaintexts.size(), g_received.size());
    for (size_t batch = 0; batch < plaintexts.size(); ++batch) {
        EXPECT_EQ(plaintexts[batch], Bytes(MAX_DECRYPT_RESULT_BYTES, static_cast<uint8_t>(batch + 1)));
    }
    output = {true, std::nullopt};
    return PRIVACY_PROTOCOL_SUCCESS;
}
} // namespace OHOS::Security::PrivacyComputation

namespace {
HWTEST_F(PrivacyClientTest, SymmetricFramesStayLocalAndBatchesKeepOrder, TestSize.Level1)
{
    const OHOS::Security::PrivacyComputation::PrivacyProtocol protocol{
        DataSetSize::SIZE_512, ProtocolType::PIR_PROTOCOL};
    PrivacySearchResult input{{Bytes(100 * 1024, 0xA5), Bytes(100 * 1024, 0x5A), Bytes{3}},
                              std::vector<Bytes>{Bytes(512 * 1024, 0xFF)}};
    g_localSymmetric = &input.valueCipherText;
    SearchResult output;
    ASSERT_EQ(GetSearchResultRemote(input, protocol, output), PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_EQ(g_received, input.resultCipherText);
    EXPECT_EQ(g_postprocessCalls, 1U);
    EXPECT_TRUE(output.matchedResult);
    EXPECT_EQ(input.valueCipherText->front().size(), 512U * 1024U);
}

HWTEST_F(PrivacyClientTest, FailedBatchStopsBeforePostprocessing, TestSize.Level1)
{
    const OHOS::Security::PrivacyComputation::PrivacyProtocol protocol{
        DataSetSize::SIZE_512, ProtocolType::PSI_PROTOCOL};
    const PrivacySearchResult input{{Bytes{1}, Bytes(193 * 1024, 2), Bytes{3}}, std::nullopt};
    g_failBatch = 2;
    SearchResult output{true, std::vector<Bytes>{Bytes{0x99}}};
    EXPECT_EQ(GetSearchResultRemote(input, protocol, output), PRIVACY_PROTOCOL_HE_ERROR);
    EXPECT_EQ(g_received.size(), 2U);
    EXPECT_EQ(g_received.back(), input.resultCipherText[1]);
    EXPECT_EQ(g_postprocessCalls, 0U);
    ASSERT_TRUE(output.attachedValues.has_value());
    EXPECT_EQ(*output.attachedValues, std::vector<Bytes>({Bytes{0x99}}));
}

HWTEST_F(PrivacyClientTest, InvalidPlaintextStopsBeforePostprocessing, TestSize.Level1)
{
    const OHOS::Security::PrivacyComputation::PrivacyProtocol protocol{
        DataSetSize::SIZE_512, ProtocolType::PSI_PROTOCOL};
    const PrivacySearchResult input{{Bytes{1}}, std::nullopt};
    g_invalidPlaintext = true;
    SearchResult output;
    EXPECT_EQ(GetSearchResultRemote(input, protocol, output), PRIVACY_PROTOCOL_CODEC_ERROR);
    EXPECT_EQ(g_postprocessCalls, 0U);
}

/*
 * @tc.name: ClientRejectsInvalidTargetResponseSize
 * @tc.desc: Reject empty and oversized successful IPC replies without replacing the target.
 * @tc.type: FUNC
 */
HWTEST_F(PrivacyClientTest, ClientRejectsInvalidTargetResponseSize, TestSize.Level2)
{
    const OHOS::Security::PrivacyComputation::PrivacyProtocol protocol{
        DataSetSize::SIZE_512, ProtocolType::PSI_PROTOCOL};
    const TargetInput input{Bytes{1}, HashAlg::SHA256};
    const Bytes sentinel{0x99};
    Bytes output = sentinel;
    for (const uint32_t size : {0U, MAX_TARGET_RESULT_BYTES + 1U}) {
        g_targetResponseSize = size;
        EXPECT_EQ(GenPrivacyTargetRemote(input, protocol, output), PRIVACY_PROTOCOL_CODEC_ERROR);
        EXPECT_EQ(output, sentinel);
    }
}

/*
 * @tc.name: ClientRejectsEmptyHeBatchBeforeIpc
 * @tc.desc: Reject an empty batch before sending any decrypt request or processing results.
 * @tc.type: FUNC
 */
HWTEST_F(PrivacyClientTest, ClientRejectsEmptyHeBatchBeforeIpc, TestSize.Level2)
{
    const OHOS::Security::PrivacyComputation::PrivacyProtocol protocol{
        DataSetSize::SIZE_512, ProtocolType::PSI_PROTOCOL};
    const PrivacySearchResult input{{Bytes{}}, std::nullopt};
    SearchResult output{true, std::nullopt};
    EXPECT_EQ(GetSearchResultRemote(input, protocol, output), PRIVACY_PROTOCOL_INVALID_ARGUMENT);
    EXPECT_TRUE(g_received.empty());
    EXPECT_EQ(g_postprocessCalls, 0U);
    EXPECT_TRUE(output.matchedResult);
}
} // namespace
