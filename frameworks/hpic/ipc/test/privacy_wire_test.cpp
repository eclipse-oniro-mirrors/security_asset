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

#include "privacy_wire.h"
#include <gtest/gtest.h>
using namespace OHOS::Security::PrivacyComputation;
TEST(PrivacyWireTest, RoundTripPreservesEmptyFramesAndTail)
{
    PrivacyWire wire;
    const std::vector<psi_pir::Bytes> frames{{1, 2}, {}, {3}};
    wire.PutList(frames);
    EXPECT_EQ(wire.GetList(), frames);
    EXPECT_TRUE(wire.Done());
    wire.Put(9);
    EXPECT_FALSE(wire.Done());
}
TEST(PrivacyWireTest, MalformedLengthsNeverReadPastInput)
{
    PrivacyWire truncated;
    truncated.bytes = {1, 0, 0};
    EXPECT_EQ(truncated.Get(), 0U);
    EXPECT_FALSE(truncated.Done());
    PrivacyWire oversized;
    oversized.bytes = {255, 255, 255, 255};
    EXPECT_TRUE(oversized.GetBytes().empty());
    EXPECT_FALSE(oversized.Done());
    PrivacyWire list;
    list.bytes = {2, 0, 0, 0, 0, 0, 0, 0};
    EXPECT_TRUE(list.GetList().empty());
    EXPECT_FALSE(list.Done());
}
