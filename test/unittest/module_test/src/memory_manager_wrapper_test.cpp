/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "memory_manager_wrapper_test.h"

#include <cstring>
#include <gtest/gtest.h>

#include "memory_manager_wrapper.h"

using namespace testing::ext;
namespace UnitTest::AssetMemoryManagerWrapperTest {
class AssetMemoryManagerWrapperTest : public testing::Test {
public:
    static void SetUpTestCase(void);

    static void TearDownTestCase(void);

    void SetUp(void);

    void TearDown(void);
};

void AssetMemoryManagerWrapperTest::SetUpTestCase(void)
{
}

void AssetMemoryManagerWrapperTest::TearDownTestCase(void)
{
}

void AssetMemoryManagerWrapperTest::SetUp(void)
{
}

void AssetMemoryManagerWrapperTest::TearDown(void)
{
}

void PackageRemovedCallback(PackageInfo)
{
}

void OnUserRemovedCallback(int32_t userId)
{
}

void OnScreenOffCallback(void)
{
}

void OnChargingCallback(void)
{
}

void OnAppRestore(int32_t packageId, const uint8_t *owner, int32_t appIndex)
{
}

void OnUserUnlocked(int32_t userId)
{
}

/**
 * @tc.name: AssetMemoryManagerWrapperTest.AssetMemoryManagerWrapperTest001
 * @tc.desc: Test asset func SubscribeSystemEvent, expect true
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetMemoryManagerWrapperTest, AssetMemoryManagerWrapperTest001, TestSize.Level0)
{
    ASSERT_EQ(true, CheckMemoryMgr());
    NotifyStatus(1);
    SetCritical(true);
    SetCritical(false);
    NotifyStatus(0);
}
}