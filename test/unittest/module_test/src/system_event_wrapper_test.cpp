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

#include "system_ability_wrapper.h"
#include "system_event_wrapper.h"
#include "system_event_wrapper.cpp"

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
 * @tc.name: AssetSystemEventWrapperTest.AssetSystemEventWrapperTest001
 * @tc.desc: Test asset func SubscribeSystemEvent, expect true
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetSystemEventWrapperTest, AssetSystemEventWrapperTest001, TestSize.Level0)
{
    EventCallBack call_back = {
        PackageRemovedCallback,
        OnUserRemovedCallback,
        OnScreenOffCallback,
        OnChargingCallback,
        OnAppRestore,
        OnUserUnlocked
    };
    ASSERT_EQ(true, SubscribeSystemEvent(call_back));
}

/**
 * @tc.name: AssetSystemEventWrapperTest.AssetSystemEventWrapperTest002
 * @tc.desc: Test asset func UnSubscribeSystemEvent, expect true
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetSystemEventWrapperTest, AssetSystemEventWrapperTest002, TestSize.Level0)
{
    ASSERT_EQ(true, UnSubscribeSystemEvent());
    ASSERT_EQ(false, UnSubscribeSystemEvent());
}

/**
 * @tc.name: AssetSystemEventWrapperTest.AssetSystemEventWrapperTest003
 * @tc.desc: Test asset func ParseDeveloperId, expect true
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetSystemEventWrapperTest, AssetSystemEventWrapperTest003, TestSize.Level0)
{
    ConstAssetBlob blob;
    std::string developerId = "test_developer";

    ParseDeveloperId(developerId, blob);
    ASSERT_EQ(blob.size, developerId.size());
}

/**
 * @tc.name: AssetSystemEventWrapperTest.AssetSystemEventWrapperTest004
 * @tc.desc: Test asset func ParseDeveloperId, expect true
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetSystemEventWrapperTest, AssetSystemEventWrapperTest004, TestSize.Level0)
{
    ConstAssetBlob blob;
    std::string developerId = "";

    ParseDeveloperId(developerId, blob);
    ASSERT_EQ(blob.size, 0);
    ASSERT_EQ(blob.data, nullptr);
}

/**
 * @tc.name: AssetSystemEventWrapperTest.AssetSystemEventWrapperTest005
 * @tc.desc: Test asset func ParseGroupIds, expect true
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetSystemEventWrapperTest, AssetSystemEventWrapperTest005, TestSize.Level0)
{
    std::string groupIds = "group1,group2,group3";
    std::vector<std::string> groupIdStrs;
    std::vector<ConstAssetBlob> groupIdBlobs;
    ConstAssetBlobArray groupIdBlobArray;

    ParseGroupIds(groupIds, groupIdStrs, groupIdBlobs, groupIdBlobArray);

    ASSERT_EQ(groupIdStrs.size(), 3);
    ASSERT_EQ(groupIdStrs[0], "group1");
    ASSERT_EQ(groupIdStrs[1], "group2");
    ASSERT_EQ(groupIdStrs[2], "group3");
    ASSERT_EQ(groupIdBlobArray.size, 3);
}

/**
 * @tc.name: AssetSystemEventWrapperTest.AssetSystemEventWrapperTest006
 * @tc.desc: Test asset func ParseGroupIds, expect true
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetSystemEventWrapperTest, AssetSystemEventWrapperTest006, TestSize.Level0)
{
    std::string groupIds = "";
    std::vector<std::string> groupIdStrs;
    std::vector<ConstAssetBlob> groupIdBlobs;
    ConstAssetBlobArray groupIdBlobArray;

    ParseGroupIds(groupIds, groupIdStrs, groupIdBlobs, groupIdBlobArray);

    ASSERT_EQ(groupIdBlobArray.size, 0);
    ASSERT_EQ(groupIdBlobArray.blob, nullptr);
}
}