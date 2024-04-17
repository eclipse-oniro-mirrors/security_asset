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

#include "bms_wrapper_test.h"

#include <cstring>
#include <gtest/gtest.h>

#include "sec_asset_type.h"
#include "bms_wrapper.h"

using namespace testing::ext;
namespace UnitTest::AssetBmsWrapperTest {
class AssetBmsWrapperTest : public testing::Test {
public:
    static void SetUpTestCase(void);

    static void TearDownTestCase(void);

    void SetUp(void);

    void TearDown(void);
};

void AssetBmsWrapperTest::SetUpTestCase(void)
{
}

void AssetBmsWrapperTest::TearDownTestCase(void)
{
}

void AssetBmsWrapperTest::SetUp(void)
{
}

void AssetBmsWrapperTest::TearDown(void)
{
}

/**
 * @tc.name: AssetBmsWrapperTest.AssetBmsWrapperTest001
 * @tc.desc: Test asset func GetOwnerInfo, expect SUCCESS
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetBmsWrapperTest, AssetBmsWrapperTest001, TestSize.Level0)
{
    OwnerType ownerType = NATIVE;
    uint8_t ownerInfo[256] = { 0 };
    uint32_t infoLen = 256;
    int32_t userId = 0;
    uint64_t uid = 0;
    ASSERT_EQ(SEC_ASSET_SUCCESS, GetOwnerInfo(userId, uid, &ownerType, ownerInfo, &infoLen));
}

/**
 * @tc.name: AssetBmsWrapperTest.AssetBmsWrapperTest002
 * @tc.desc: Test asset func GetOwnerInfo, expect INVALID_ARGUMENT
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetBmsWrapperTest, AssetBmsWrapperTest002, TestSize.Level0)
{
    OwnerType* ownerType = nullptr;
    uint8_t ownerInfo[256] = { 0 };
    uint32_t infoLen = 256;
    int32_t userId = 0;
    uint64_t uid = 0;
    ASSERT_EQ(SEC_ASSET_INVALID_ARGUMENT, GetOwnerInfo(userId, uid, ownerType, ownerInfo, &infoLen));
}

/**
 * @tc.name: AssetBmsWrapperTest.AssetBmsWrapperTest003
 * @tc.desc: Test asset func GetOwnerInfo, expect INVALID_ARGUMENT
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetBmsWrapperTest, AssetBmsWrapperTest003, TestSize.Level0)
{
    OwnerType ownerType = NATIVE;
    uint8_t* ownerInfo = nullptr;
    uint32_t infoLen = 256;
    int32_t userId = 0;
    uint64_t uid = 0;
    ASSERT_EQ(SEC_ASSET_INVALID_ARGUMENT, GetOwnerInfo(userId, uid, &ownerType, ownerInfo, &infoLen));
}

/**
 * @tc.name: AssetBmsWrapperTest.AssetBmsWrapperTest004
 * @tc.desc: Test asset func GetOwnerInfo, expect INVALID_ARGUMENT
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetBmsWrapperTest, AssetBmsWrapperTest004, TestSize.Level0)
{
    OwnerType ownerType = NATIVE;
    uint8_t ownerInfo[256] = { 0 };
    uint32_t* infoLen = nullptr;
    int32_t userId = 0;
    uint64_t uid = 0;
    ASSERT_EQ(SEC_ASSET_INVALID_ARGUMENT, GetOwnerInfo(userId, uid, &ownerType, ownerInfo, infoLen));
}
}