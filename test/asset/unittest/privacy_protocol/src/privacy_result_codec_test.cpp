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

#include "privacy_result_codec.h"
#include <array>
#include <gtest/gtest.h>

namespace {
using namespace testing::ext;
using namespace psi_pir;
using namespace OHOS::Security::PrivacyComputation;

/**
 * @tc.name: ResultBuffer_001
 * @tc.desc: Match the Rust little-endian fixture and preserve empty values in a result.
 * @tc.type: FUNC
 */
HWTEST(PrivacyResultCodecTest, ResultBuffer_001, TestSize.Level1)
{
    const Bytes expected{1, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 255};
    SearchResult input;
    input.matchedResult = true;
    input.attachedValues = std::vector<Bytes>{{}, {0, 255}};
    Bytes bytes(expected.size());
    PrivacyBuffer buffer{bytes.data(), static_cast<uint32_t>(bytes.size())};
    ASSERT_TRUE(EncodePrivacyResult(input, buffer));
    EXPECT_EQ(bytes, expected);
    SearchResult decoded;
    ASSERT_TRUE(DecodePrivacyResult(expected.data(), expected.size(), decoded));
    EXPECT_TRUE(decoded.matchedResult);
    EXPECT_EQ(decoded.attachedValues, input.attachedValues);
}

/**
 * @tc.name: ResultBuffer_002
 * @tc.desc: Distinguish absent, present empty and a list containing one empty value.
 * @tc.type: FUNC
 */
HWTEST(PrivacyResultCodecTest, ResultBuffer_002, TestSize.Level1)
{
    const std::array<std::optional<std::vector<Bytes>>, 3> values{std::nullopt, std::vector<Bytes>{},
                                                                  std::vector<Bytes>{{}}};
    for (const auto &value : values) {
        SearchResult input;
        input.attachedValues = value;
        std::array<uint8_t, 32> bytes{};
        PrivacyBuffer buffer{bytes.data(), static_cast<uint32_t>(bytes.size())};
        ASSERT_TRUE(EncodePrivacyResult(input, buffer));
        SearchResult decoded;
        ASSERT_TRUE(DecodePrivacyResult(bytes.data(), buffer.size, decoded));
        EXPECT_EQ(decoded.attachedValues, value);
    }
}

/**
 * @tc.name: ResultBuffer_003
 * @tc.desc: Reject truncated, invalid and trailing data without committing partial output.
 * @tc.type: FUNC
 */
HWTEST(PrivacyResultCodecTest, ResultBuffer_003, TestSize.Level2)
{
    const Bytes encoded{1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 0, 255};
    SearchResult output;
    output.attachedValues = std::vector<Bytes>{{0xAA}};
    for (size_t size = 0; size < encoded.size(); ++size) {
        EXPECT_FALSE(DecodePrivacyResult(encoded.data(), size, output));
        EXPECT_FALSE(output.matchedResult);
        EXPECT_EQ(output.attachedValues, std::optional<std::vector<Bytes>>({{0xAA}}));
    }
    auto invalid = encoded;
    invalid[4] = 2;
    EXPECT_FALSE(DecodePrivacyResult(invalid.data(), invalid.size(), output));
    invalid = encoded;
    std::fill(invalid.begin() + 8, invalid.begin() + 12, 0xFF);
    EXPECT_FALSE(DecodePrivacyResult(invalid.data(), invalid.size(), output));
    invalid = encoded;
    invalid.push_back(0);
    EXPECT_FALSE(DecodePrivacyResult(invalid.data(), invalid.size(), output));
}

/**
 * @tc.name: ResultBuffer_004
 * @tc.desc: Reject insufficient capacity without touching the caller's buffer or length.
 * @tc.type: FUNC
 */
HWTEST(PrivacyResultCodecTest, ResultBuffer_004, TestSize.Level2)
{
    SearchResult input;
    std::array<uint8_t, RESULT_FIXED_BYTES> bytes;
    bytes.fill(0xAA);
    const auto original = bytes;
    PrivacyBuffer buffer{bytes.data(), static_cast<uint32_t>(bytes.size() - 1)};
    EXPECT_FALSE(EncodePrivacyResult(input, buffer));
    EXPECT_EQ(bytes, original);
    EXPECT_EQ(buffer.size, bytes.size() - 1);
}

/**
 * @tc.name: ResultBuffer_005
 * @tc.desc: Accept the exact byte budget and reject one additional byte before writing output.
 * @tc.type: FUNC
 */
HWTEST(PrivacyResultCodecTest, ResultBuffer_005, TestSize.Level2)
{
    constexpr size_t overhead = RESULT_FIXED_BYTES + 2 * PRIVACY_LENGTH_BYTES;
    SearchResult input;
    input.attachedValues = std::vector<Bytes>{Bytes(MAX_PRIVACY_BYTES - overhead, 0)};
    Bytes bytes(MAX_PRIVACY_BYTES, 0xAA);
    PrivacyBuffer buffer{bytes.data(), static_cast<uint32_t>(bytes.size())};
    ASSERT_TRUE(EncodePrivacyResult(input, buffer));
    EXPECT_EQ(buffer.size, MAX_PRIVACY_BYTES);
    SearchResult output;
    ASSERT_TRUE(DecodePrivacyResult(bytes.data(), buffer.size, output));
    EXPECT_EQ(output.attachedValues, input.attachedValues);
    input.attachedValues->front().push_back(0);
    const auto original = bytes;
    EXPECT_FALSE(EncodePrivacyResult(input, buffer));
    EXPECT_EQ(bytes, original);
    EXPECT_EQ(buffer.size, MAX_PRIVACY_BYTES);
}
} // namespace
