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

#include "os_account_wrapper_test.h"

#include <cstring>
#include <gtest/gtest.h>

#include "os_account_wrapper.h"

using namespace testing::ext;
namespace UnitTest::AssetOsAccountWrapperTest {
class AssetOsAccountWrapperTest : public testing::Test {
public:
    static void SetUpTestCase(void);

    static void TearDownTestCase(void);

    void SetUp(void);

    void TearDown(void);
};

void AssetOsAccountWrapperTest::SetUpTestCase(void)
{
}

void AssetOsAccountWrapperTest::TearDownTestCase(void)
{
}

void AssetOsAccountWrapperTest::SetUp(void)
{
}

void AssetOsAccountWrapperTest::TearDown(void)
{
}

/**
 * @tc.name: AssetOsAccountWrapperTest.AssetOsAccountWrapperTest001
 * @tc.desc: Test asset func GetOwnerInfo, expect ACCESS_TOKEN_ERROR
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetOsAccountWrapperTest, AssetOsAccountWrapperTest001, TestSize.Level0)
{
    int32_t userId = 1000;
    uint32_t uid = 6226;
    ASSERT_EQ(true, GetUserIdByUid(uid, &userId));
}

/**
 * @tc.name: AssetOsAccountWrapperTest.AssetOsAccountWrapperTest002
 * @tc.desc: Test asset func GetOwnerInfo, expect BMS_ERROR
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetOsAccountWrapperTest, AssetOsAccountWrapperTest002, TestSize.Level0)
{
    int32_t userId = 1000;
    uint32_t uid = 100;
    ASSERT_EQ(true, GetUserIdByUid(uid, &userId));
}
}