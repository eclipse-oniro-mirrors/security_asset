/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>

#include "memory_manager_wrapper.h"

using namespace testing::ext;

namespace UnitTest::SafMemoryManagerWrapperTest {
namespace {
constexpr int32_t START_STATUS = 1;
constexpr int32_t DEAD_STATUS = 0;
}

class SafMemoryManagerWrapperTest : public testing::Test {
public:
    void TearDown() override
    {
        if (registered_) {
            // Cleanup must also run when a fatal assertion ends the test early.
            if (criticalAttempted_) {
                EXPECT_EQ(0, SetCritical(false));
            }
            EXPECT_EQ(0, NotifyStatus(DEAD_STATUS));
        }
    }

protected:
    bool registered_ = false;
    bool criticalAttempted_ = false;
};

/**
 * @tc.name: CheckMemoryMgr_001
 * @tc.desc: Verify that the memory manager service is available.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SafMemoryManagerWrapperTest, CheckMemoryMgr_001, TestSize.Level0)
{
    EXPECT_TRUE(CheckMemoryMgr());
}

/**
 * @tc.name: NotifyStatus_001
 * @tc.desc: Verify SAF process start and stop notifications succeed.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SafMemoryManagerWrapperTest, NotifyStatus_001, TestSize.Level0)
{
    ASSERT_TRUE(CheckMemoryMgr());
    int32_t result = NotifyStatus(START_STATUS);
    registered_ = (result == 0);
    ASSERT_EQ(0, result);

    result = NotifyStatus(DEAD_STATUS);
    EXPECT_EQ(0, result);
    if (result == 0) {
        registered_ = false;
    }
}

/**
 * @tc.name: SetCritical_001
 * @tc.desc: Verify SAF critical protection can be enabled and disabled after registration.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SafMemoryManagerWrapperTest, SetCritical_001, TestSize.Level0)
{
    ASSERT_TRUE(CheckMemoryMgr());
    int32_t result = NotifyStatus(START_STATUS);
    registered_ = (result == 0);
    ASSERT_EQ(0, result);

    criticalAttempted_ = true;
    EXPECT_EQ(0, SetCritical(true));
    result = SetCritical(false);
    EXPECT_EQ(0, result);
    if (result == 0) {
        criticalAttempted_ = false;
    }
}
} // namespace UnitTest::SafMemoryManagerWrapperTest
