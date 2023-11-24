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

extern "C" bool LoadService(int32_t saId);

using namespace testing::ext;
namespace UnitTest::AssetSamgrWrapperTest {
class AssetSamgrWrapperTest : public testing::Test {
public:
    static void SetUpTestCase(void);

    static void TearDownTestCase(void);

    void SetUp(void);

    void TearDown(void);
};

void AssetSamgrWrapperTest::SetUpTestCase(void)
{
}

void AssetSamgrWrapperTest::TearDownTestCase(void)
{
}

void AssetSamgrWrapperTest::SetUp(void)
{
}

void AssetSamgrWrapperTest::TearDown(void)
{
}

/**
 * @tc.name: AssetSamgrWrapperTest.AssetSamgrWrapperTest001
 * @tc.desc: Test asset func LoadService, expect
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetSamgrWrapperTest, AssetSamgrWrapperTest001, TestSize.Level0)
{
    int32_t asId = 3510;
    ASSERT_EQ(true, LoadService(asId));
}

/**
 * @tc.name: AssetSamgrWrapperTest.AssetSamgrWrapperTest002
 * @tc.desc: Test asset func LoadService, expect
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetSamgrWrapperTest, AssetSamgrWrapperTest002, TestSize.Level0)
{
    int32_t asId = -1;
    ASSERT_EQ(false, LoadService(asId));
}

/**
 * @tc.name: AssetSamgrWrapperTest.AssetSamgrWrapperTest003
 * @tc.desc: Test asset func LoadService, expect
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetSamgrWrapperTest, AssetSamgrWrapperTest003, TestSize.Level0)
{
    int32_t asId = 99999;
    ASSERT_EQ(false, LoadService(asId));
}
}