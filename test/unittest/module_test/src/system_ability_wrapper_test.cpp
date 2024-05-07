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

#include "system_ability_wrapper_test.h"

#include <cstring>
#include <gtest/gtest.h>

#include "system_ability_wrapper.h"
#include "system_event_wrapper.h"

using namespace testing::ext;
namespace UnitTest::AssetSystemAbilityWrapperTest {
class AssetSystemAbilityWrapperTest : public testing::Test {
public:
    static void SetUpTestCase(void);

    static void TearDownTestCase(void);

    void SetUp(void);

    void TearDown(void);
};

void AssetSystemAbilityWrapperTest::SetUpTestCase(void)
{
}

void AssetSystemAbilityWrapperTest::TearDownTestCase(void)
{
}

void AssetSystemAbilityWrapperTest::SetUp(void)
{
}

void AssetSystemAbilityWrapperTest::TearDown(void)
{
}

void PackageRemovedCallback(int32_t packageId, const uint8_t *owner, uint32_t ownerSize)
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

void OnAppRestore(int32_t packageId, const uint8_t *owner)
{
}

void OnUserUnlocked(int32_t userId)
{
}

/**
 * @tc.name: AssetSystemAbilityWrapperTest.AssetSystemAbilityWrapperTest001
 * @tc.desc: Test asset func SubscribeSystemEvent, expect ACCESS_TOKEN_ERROR
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetSystemAbilityWrapperTest, AssetSystemAbilityWrapperTest001, TestSize.Level0)
{
    EventCallBack call_back = {
        PackageRemovedCallback,
        OnUserRemovedCallback,
        OnScreenOffCallback,
        OnChargingCallback,
        OnAppRestore,
        OnUserUnlocked
    };
    ASSERT_EQ(true, SubscribeSystemEvent(&call_back));
}

/**
 * @tc.name: AssetSystemAbilityWrapperTest.AssetSystemAbilityWrapperTest002
 * @tc.desc: Test asset func UnSubscribeSystemEvent, expect BMS_ERROR
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetSystemAbilityWrapperTest, AssetSystemAbilityWrapperTest002, TestSize.Level0)
{
    ASSERT_EQ(true, UnSubscribeSystemEvent());
}
}