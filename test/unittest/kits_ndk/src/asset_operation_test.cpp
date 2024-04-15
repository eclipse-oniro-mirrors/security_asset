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

#include "asset_operation_test.h"

#include <string>
#include <gtest/gtest.h>

#include "asset_api.h"
#include "asset_test_common.h"

using namespace testing::ext;
namespace UnitTest::AssetOperationTest {
class AssetOperationTest : public testing::Test {
public:
    static void SetUpTestCase(void);

    static void TearDownTestCase(void);

    void SetUp(void);

    void TearDown(void);
};

void AssetOperationTest::SetUpTestCase(void)
{
}

void AssetOperationTest::TearDownTestCase(void)
{
}

void AssetOperationTest::SetUp(void)
{
}

void AssetOperationTest::TearDown(void)
{
}

/**
 * @tc.name: AssetOperationTest.AssetOperationTest001
 * @tc.desc: Free blob with nullptr, expect non-crsh
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetOperationTest, AssetOperationTest001, TestSize.Level0)
{
    OH_Asset_FreeBlob(nullptr);
}

/**
 * @tc.name: AssetOperationTest.AssetOperationTest002
 * @tc.desc: Free blob with nullptr data, expect non-crsh
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetOperationTest, AssetOperationTest002, TestSize.Level0)
{
    Asset_Blob blob = { .size = 0, .data = nullptr };
    OH_Asset_FreeBlob(&blob);
}

/**
 * @tc.name: AssetOperationTest.AssetOperationTest003
 * @tc.desc: Free blob with nullptr data, expect non-crsh
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetOperationTest, AssetOperationTest003, TestSize.Level0)
{
    Asset_Blob blob = { .size = 0, .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    OH_Asset_FreeBlob(&blob);
}

/**
 * @tc.name: AssetOperationTest.AssetOperationTest004
 * @tc.desc: Free result set with nullptr, expect non-crsh
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetOperationTest, AssetOperationTest004, TestSize.Level0)
{
    OH_Asset_FreeResultSet(nullptr);
}

/**
 * @tc.name: AssetOperationTest.AssetOperationTest005
 * @tc.desc: Free result set with nullptr, expect non-crsh
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetOperationTest, AssetOperationTest005, TestSize.Level0)
{
    Asset_ResultSet resultSet = { .count = 0, .results = nullptr };
    OH_Asset_FreeResultSet(&resultSet);
}
}