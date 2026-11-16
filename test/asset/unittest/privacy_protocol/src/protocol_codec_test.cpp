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

#include "protocol_codec.h"

#include <algorithm>
#include <array>
#include <gtest/gtest.h>
#include <limits>

namespace {
using namespace testing::ext;
using namespace OHOS::Security::PrivacyComputation;
using namespace OHOS::Security::PrivacyComputation::internal;

/**
 * @tc.name: SerializePlaintext_001
 * @tc.desc: Preserve little-endian slot layout and round-trip through plaintext parsing.
 * @tc.type: FUNC
 */
HWTEST(PrivacyProtocolCodecTest, SerializePlaintext_001, TestSize.Level1)
{
    const ProtocolParams params;
    Slots slots(params.polyModulusDegree, 0);
    slots.front() = 1;
    slots.back() = params.plainModulus - 1;
    Bytes bytes;
    ASSERT_EQ(ProtocolCodec().SerializePlaintext(slots, params, bytes), PRIVACY_PROTOCOL_SUCCESS);
    ASSERT_EQ(bytes.size(), slots.size() * sizeof(uint64_t));
    const std::array<uint8_t, sizeof(uint64_t)> firstSlot{1, 0, 0, 0, 0, 0, 0, 0};
    const std::array<uint8_t, sizeof(uint64_t)> lastSlot{0, 0, 1, 0, 0, 0, 0, 0};
    EXPECT_TRUE(std::equal(firstSlot.begin(), firstSlot.end(), bytes.begin()));
    EXPECT_TRUE(std::equal(lastSlot.begin(), lastSlot.end(), bytes.end() - sizeof(uint64_t)));
    const PrivacySearchResult plaintexts{{bytes}, std::nullopt};
    std::vector<Slots> decoded;
    ASSERT_EQ(ProtocolCodec().ParsePlaintexts(plaintexts.resultCipherText, plaintexts.valueCipherText, params, decoded),
              PRIVACY_PROTOCOL_SUCCESS);
    ASSERT_EQ(decoded.size(), 1U);
    EXPECT_EQ(decoded.front(), slots);
}

/**
 * @tc.name: SerializePlaintext_002
 * @tc.desc: Reject invalid slot values and counts without replacing the caller output.
 * @tc.type: FUNC
 */
HWTEST(PrivacyProtocolCodecTest, SerializePlaintext_002, TestSize.Level2)
{
    const ProtocolParams params;
    Slots slots(params.polyModulusDegree, 0);
    Bytes output{0x99};
    slots.back() = params.plainModulus;
    EXPECT_EQ(ProtocolCodec().SerializePlaintext(slots, params, output), PRIVACY_PROTOCOL_CODEC_ERROR);
    EXPECT_EQ(output, Bytes({0x99}));
    slots.pop_back();
    EXPECT_EQ(ProtocolCodec().SerializePlaintext(slots, params, output), PRIVACY_PROTOCOL_CODEC_ERROR);
    EXPECT_EQ(output, Bytes({0x99}));
}

/*
 * @tc.name: ValidateProtocolRejectsInvalidDimensions
 * @tc.desc: Reject invalid dimensions before using them to calculate a slot layout.
 * @tc.type: FUNC
 */
HWTEST(PrivacyProtocolCodecTest, ValidateProtocolRejectsInvalidDimensions, TestSize.Level2)
{
    constexpr auto MAX_SIZE = std::numeric_limits<size_t>::max();
    constexpr auto INVALID = PRIVACY_PROTOCOL_INVALID_ARGUMENT;
    constexpr auto UNSUPPORTED = PRIVACY_PROTOCOL_UNSUPPORTED_PROTOCOL;
    ProtocolParams defaults;
    const ProtocolCodec codec;
    ASSERT_EQ(codec.ValidateProtocol(defaults), PRIVACY_PROTOCOL_SUCCESS);
    for (const auto field : {&ProtocolParams::compareBits, &ProtocolParams::bitsPerSlot,
                             &ProtocolParams::slotsPerData, &ProtocolParams::dataNumPerPlaintext}) {
        auto params = defaults;
        params.*field = 0;
        EXPECT_EQ(codec.ValidateProtocol(params), INVALID);
    }
    auto params = defaults;
    params.slotsPerData = MAX_SIZE;
    EXPECT_EQ(codec.ValidateProtocol(params), INVALID);
    params = defaults;
    params.dataNumPerPlaintext = MAX_SIZE;
    EXPECT_EQ(codec.ValidateProtocol(params), INVALID);
    params = defaults;
    ++params.compareBits;
    EXPECT_EQ(codec.ValidateProtocol(params), INVALID);
    params.compareBits = 15;
    params.bitsPerSlot = 15;
    params.slotsPerData = 1;
    EXPECT_EQ(codec.ValidateProtocol(params), UNSUPPORTED);
    params.compareBits = kMaxBitsPerSlot;
    params.bitsPerSlot = kMaxBitsPerSlot;
    EXPECT_EQ(codec.ValidateProtocol(params), UNSUPPORTED);
    params = defaults;
    params.plainModulus = 2;
    EXPECT_EQ(codec.ValidateProtocol(params), INVALID);
}

/*
 * @tc.name: ParsePlaintextsRejectsOverflow
 * @tc.desc: Reject slot and byte count overflow before allocating or replacing the output.
 * @tc.type: FUNC
 */
HWTEST(PrivacyProtocolCodecTest, ParsePlaintextsRejectsOverflow, TestSize.Level2)
{
    constexpr auto MAX_SIZE = std::numeric_limits<size_t>::max();
    ProtocolParams params;
    const std::vector<Slots> sentinel{{7}};
    auto decoded = sentinel;
    params.dataNumPerPlaintext = MAX_SIZE;
    EXPECT_EQ(ProtocolCodec().ParsePlaintexts({}, std::nullopt, params, decoded), PRIVACY_PROTOCOL_INVALID_ARGUMENT);
    EXPECT_EQ(decoded, sentinel);
    params.slotsPerData = 1;
    EXPECT_EQ(ProtocolCodec().ParsePlaintexts({}, std::nullopt, params, decoded), PRIVACY_PROTOCOL_INVALID_ARGUMENT);
    EXPECT_EQ(decoded, sentinel);
}
} // namespace
