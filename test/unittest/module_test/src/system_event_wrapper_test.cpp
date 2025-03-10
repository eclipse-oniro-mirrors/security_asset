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

class MockWant {
public:
    MOCK_METHOD(int, GetIntParam, (const std::string&, int), ());
    MOCK_METHOD(std::string, GetStringParam, (const std::string&), ());
    MOCK_METHOD(std::string, GetBundle, (), ());
};

/**
 * @tc.name: AssetSystemEventWrapperTest.AssetSystemEventWrapperTest007
 * @tc.desc: Test asset func HandlePackageRemoved, expect true
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetSystemEventWrapperTest, AssetSystemEventWrapperTest007, TestSize.Level0)
{
    MockWant want;
    EXPECT_CALL(want, GetIntParam(USER_ID, INVALID_USERID)).WillOnce(Return(1));
    EXPECT_CALL(want, GetStringParam(APP_ID)).WillOnce(Return("com.example.app"));
    EXPECT_CALL(want, GetIntParam(SANDBOX_APP_INDEX, -1)).WillOnce(Return(0));
    EXPECT_CALL(want, GetBundle()).WillOnce(Return("com.example.bundle"));

    bool onPackageRemovedCalled = false;
    OnPackageRemoved onPackageRemoved = [&](const PackageRemovedInfo& info) {
        onPackageRemovedCalled = true;
        EXPECT_EQ(info.userId, 1);
        EXPECT_EQ(reinterpret_cast<const char*>(info.ownerBlob.data), "com.example.app0");
    };

    HandlePackageRemoved(want, true, onPackageRemoved);
    EXPECT_TRUE(onPackageRemovedCalled);
}

/**
 * @tc.name: AssetSystemEventWrapperTest.AssetSystemEventWrapperTest008
 * @tc.desc: Test asset func HandlePackageRemoved, expect not crash
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetSystemEventWrapperTest, AssetSystemEventWrapperTest008, TestSize.Level0)
{
    MockWant want;
    EXPECT_CALL(want, GetIntParam(USER_ID, INVALID_USERID)).WillOnce(Return(INVALID_USERID));
    EXPECT_CALL(want, GetStringParam(APP_ID)).WillOnce(Return(""));
    EXPECT_CALL(want, GetIntParam(SANDBOX_APP_INDEX, -1)).WillOnce(Return(-1));

    HandlePackageRemoved(want, true, nullptr); // Test with null callback
}

/**
 * @tc.name: AssetSystemEventWrapperTest.AssetSystemEventWrapperTest009
 * @tc.desc: Test asset func HandleAppRestore, expect true
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetSystemEventWrapperTest, AssetSystemEventWrapperTest009, TestSize.Level0)
{
    MockWant want;
    EXPECT_CALL(want, GetIntParam(USER_ID, INVALID_USERID)).WillOnce(Return(1));
    EXPECT_CALL(want, GetStringParam(BUNDLE_NAME)).WillOnce(Return("com.example.bundle"));
    EXPECT_CALL(want, GetIntParam(SANDBOX_APP_INDEX, -1)).WillOnce(Return(0));

    bool onAppRestoreCalled = false;
    OnAppRestore onAppRestore = [&](int userId, const uint8_t* bundleName, int appIndex) {
        onAppRestoreCalled = true;
        EXPECT_EQ(userId, 1);
        EXPECT_STREQ(reinterpret_cast<const char*>(bundleName), "com.example.bundle");
        EXPECT_EQ(appIndex, 0);
    };

    HandleAppRestore(want, onAppRestore);
    EXPECT_TRUE(onAppRestoreCalled);
}

/**
 * @tc.name: AssetSystemEventWrapperTest.AssetSystemEventWrapperTest010
 * @tc.desc: Test asset func HandleAppRestore, expect true
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetSystemEventWrapperTest, AssetSystemEventWrapperTest010, TestSize.Level0)
{
    MockWant want;
    EXPECT_CALL(want, GetIntParam(USER_ID, INVALID_USERID)).WillOnce(Return(INVALID_USERID));
    EXPECT_CALL(want, GetStringParam(BUNDLE_NAME)).WillOnce(Return(""));
    EXPECT_CALL(want, GetIntParam(SANDBOX_APP_INDEX, -1)).WillOnce(Return(-1));

    HandleAppRestore(want, nullptr); // Test with null callback
}
}