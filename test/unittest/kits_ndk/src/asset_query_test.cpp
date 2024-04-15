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

#include "asset_add_test.h"

#include <string>
#include <gtest/gtest.h>

#include "asset_system_api.h"
#include "asset_test_common.h"

using namespace testing::ext;
namespace UnitTest::AssetQueryTest {
class AssetQueryTest : public testing::Test {
public:
    static void SetUpTestCase(void);

    static void TearDownTestCase(void);

    void SetUp(void);

    void TearDown(void);
};

void AssetQueryTest::SetUpTestCase(void)
{
}

void AssetQueryTest::TearDownTestCase(void)
{
}

void AssetQueryTest::SetUp(void)
{
}

void AssetQueryTest::TearDown(void)
{
}

/**
 * @tc.name: AssetQueryTest.AssetQueryTest001
 * @tc.desc: Add asset, then query with correct attr, expect success
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetQueryTest, AssetQueryTest001, TestSize.Level0)
{
    AssetBlob funcName = { .size = strlen(__func__), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    AssetAttr addAttr[] = {
        { .tag = ASSET_SYSTEM_TAG_ALIAS, .value.blob = funcName },
        { .tag = ASSET_SYSTEM_TAG_SECRET, .value.blob = funcName },
        { .tag = ASSET_SYSTEM_TAG_ACCESSIBILITY, .value.u32 = ASSET_SYSTEM_ACCESSIBILITY_DEVICE_POWERED_ON },
    };
    ASSERT_EQ(ASSET_SYSTEM_SUCCESS, AssetAdd(addAttr, ARRAY_SIZE(addAttr)));

    AssetResultSet resultSet = { 0 };
    AssetAttr queryAttr[] = {
        { .tag = ASSET_SYSTEM_TAG_ALIAS, .value.blob = funcName },
        { .tag = ASSET_SYSTEM_TAG_RETURN_TYPE, .value.u32 = ASSET_SYSTEM_RETURN_ALL }
    };
    ASSERT_EQ(ASSET_SYSTEM_SUCCESS, AssetQuery(queryAttr, ARRAY_SIZE(queryAttr), &resultSet));
    ASSERT_EQ(1, resultSet.count);

    AssetFreeResultSet(&resultSet);
    ASSERT_EQ(ASSET_SYSTEM_SUCCESS, RemoveByAlias(__func__));
}

/**
 * @tc.name: AssetQueryTest.AssetQueryTest002
 * @tc.desc: Query non-exist asset, expect ASSET_SYSTEM_NOT_FOUND
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetQueryTest, AssetQueryTest002, TestSize.Level0)
{
    AssetBlob funcName = { .size = strlen(__func__), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    AssetResultSet resultSet = { 0 };
    AssetAttr attr[] = {
        { .tag = ASSET_SYSTEM_TAG_ALIAS, .value.blob = funcName },
        { .tag = ASSET_SYSTEM_TAG_RETURN_TYPE, .value.u32 = ASSET_SYSTEM_RETURN_ALL }
    };
    ASSERT_EQ(ASSET_SYSTEM_NOT_FOUND, AssetQuery(attr, ARRAY_SIZE(attr), &resultSet));

    AssetFreeResultSet(&resultSet);
}

/**
 * @tc.name: AssetQueryTest.AssetQueryTest003
 * @tc.desc: Query non-exist asset, expect ASSET_SYSTEM_NOT_FOUND
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetQueryTest, AssetQueryTest003, TestSize.Level0)
{
    AssetBlob funcName = { .size = strlen(__func__), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    AssetResultSet resultSet = { 0 };
    AssetAttr attr[] = {
        { .tag = ASSET_SYSTEM_TAG_ALIAS, .value.blob = funcName },
        { .tag = ASSET_SYSTEM_TAG_RETURN_TYPE, .value.u32 = ASSET_SYSTEM_RETURN_ALL }
    };
    ASSERT_EQ(ASSET_SYSTEM_NOT_FOUND, AssetQuery(attr, ARRAY_SIZE(attr), &resultSet));

    AssetFreeResultSet(&resultSet);
}

/**
 * @tc.name: AssetQueryTest.AssetQueryTest004
 * @tc.desc: Query without attr, expect ASSET_SYSTEM_INVALID_ARGUMENT
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetQueryTest, AssetQueryTest004, TestSize.Level0)
{
    AssetResultSet resultSet = { 0 };
    ASSERT_EQ(ASSET_SYSTEM_INVALID_ARGUMENT, AssetQuery(nullptr, 1, &resultSet));
}

/**
 * @tc.name: AssetQueryTest.AssetQueryTest005
 * @tc.desc: Query with attr but count is wrong, expect ASSET_SYSTEM_INVALID_ARGUMENT
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetQueryTest, AssetQueryTest005, TestSize.Level0)
{
    AssetBlob funcName = { .size = strlen(__func__), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    AssetResultSet resultSet = { 0 };
    AssetAttr attr[] = {
        { .tag = ASSET_SYSTEM_TAG_ALIAS, .value.blob = funcName },
        { .tag = ASSET_SYSTEM_TAG_RETURN_TYPE, .value.u32 = ASSET_SYSTEM_RETURN_ALL }
    };
    ASSERT_EQ(ASSET_SYSTEM_NOT_FOUND, AssetQuery(attr, 0, &resultSet));
}

/**
 * @tc.name: AssetQueryTest.AssetQueryTest006
 * @tc.desc: Query without attr but count is wrong, expect ASSET_SYSTEM_INVALID_ARGUMENT
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetQueryTest, AssetQueryTest006, TestSize.Level0)
{
    AssetResultSet resultSet = { 0 };
    ASSERT_EQ(ASSET_SYSTEM_INVALID_ARGUMENT, AssetQuery(nullptr, 1, &resultSet));
}

/**
 * @tc.name: AssetQueryTest.AssetQueryTest007
 * @tc.desc: Query without resultSet, expect ASSET_SYSTEM_INVALID_ARGUMENT
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetQueryTest, AssetQueryTest007, TestSize.Level0)
{
    AssetBlob funcName = { .size = strlen(__func__), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    AssetAttr attr[] = {
        { .tag = ASSET_SYSTEM_TAG_ALIAS, .value.blob = funcName },
        { .tag = ASSET_SYSTEM_TAG_RETURN_TYPE, .value.u32 = ASSET_SYSTEM_RETURN_ALL }
    };
    ASSERT_EQ(ASSET_SYSTEM_INVALID_ARGUMENT, AssetQuery(attr, ARRAY_SIZE(attr), nullptr));
}
}