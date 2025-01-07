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

#include "asset_system_type.h"
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
 * @tc.desc: Test asset func GetCallingProcessInfo, expect SEC_ASSET_ACCESS_TOKEN_ERROR
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetBmsWrapperTest, AssetBmsWrapperTest001, TestSize.Level0)
{
    const uint32_t processNameLen = 256;
    uint8_t processName[processNameLen] = { 0 };

    uint32_t userId = 0;
    uint32_t uid = 0;

    ProcessInfo processInfo = { 0 };
    processInfo.processName = processName;
    processInfo.processNameLen = processNameLen;
    int32_t ret = GetCallingProcessInfo(userId, uid, &processInfo);
    ASSERT_EQ(ret, 0);
}
}