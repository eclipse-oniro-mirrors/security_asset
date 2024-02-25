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

#include "system_event_wrapper_test.h"

#include <cstring>
#include <gtest/gtest.h>

#include "system_event_wrapper.h"

using namespace testing::ext;
namespace UnitTest::AssetSystemEventWrapperTest {
class AssetSystemEventWrapperTest : public testing::Test {
public:
    static void SetUpTestCase(void);

    static void TearDownTestCase(void);

    void SetUp(void);

    void TearDown(void);
};

void AssetSystemEventWrapperTest::SetUpTestCase(void)
{
}

void AssetSystemEventWrapperTest::TearDownTestCase(void)
{
}

void AssetSystemEventWrapperTest::SetUp(void)
{
}

void AssetSystemEventWrapperTest::TearDown(void)
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

/**
 * @tc.name: AssetSystemEventWrapperTest.AssetSystemEventWrapperTest001
 * @tc.desc: Test asset func SubscribeSystemEvent, expect ACCESS_TOKEN_ERROR
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetSystemEventWrapperTest, AssetSystemEventWrapperTest001, TestSize.Level0)
{
    OnPackageRemoved onPackageRemovedPtr = &PackageRemovedCallback;
    OnUserRemoved onUserRemovedPtr = &OnUserRemovedCallback;
    OnScreenOff onScreenOffPtr = &OnScreenOffCallback;
    ASSERT_EQ(true, SubscribeSystemEvent(onPackageRemovedPtr, onUserRemovedPtr, onScreenOffPtr));
}

/**
 * @tc.name: AssetSystemEventWrapperTest.AssetSystemEventWrapperTest002
 * @tc.desc: Test asset func UnSubscribeSystemEvent, expect BMS_ERROR
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetSystemEventWrapperTest, AssetSystemEventWrapperTest002, TestSize.Level0)
{
    ASSERT_EQ(true, UnSubscribeSystemEvent());
}
}