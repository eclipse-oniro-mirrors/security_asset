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

#include <gtest/gtest.h>

#include "asset_api.h"
#include "asset_test_common.h"

using namespace testing::ext;
namespace Unittest::AssetAddTest {
class AssetAddTest : public testing::Test {
public:
    static void SetUpTestCase(void);

    static void TearDownTestCase(void);

    void SetUp();

    void TearDown();
};

void AssetAddTest::SetUpTestCase(void)
{
}

void AssetAddTest::TearDownTestCase(void)
{
}

void AssetAddTest::SetUp()
{
}

void AssetAddTest::TearDown()
{
}

/**
 * @tc.name: AssetAddTest.AssetAddTest001
 * @tc.desc: Add alias and secret, then query, expect success and match
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetAddTest, AssetAddTest001, TestSize.Level0)
{
    Asset_Blob alias = { .size = strlen(__func__), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    Asset_Blob secret = { .size = strlen(__func__), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    Asset_Attr attr[] = {
        {
            .tag = ASSET_TAG_ALIAS,
            .value.blob = alias
        }, {
            .tag = ASSET_TAG_SECRET,
            .value.blob = secret
        }
    };
    ASSERT_EQ(ASSET_SUCCESS, OH_Asset_Add(attr, sizeof(attr) / sizeof(attr[0])));

    Asset_ResultSet resultSet = { 0 };
    ASSERT_EQ(ASSET_SUCCESS, QueryByAlias(__func__, &resultSet));
    ASSERT_EQ(1, resultSet.count);
    Asset_Result result = resultSet.results[0];
    Asset_Attr *alias_query = OH_Asset_ParseAttr(&result, ASSET_TAG_ALIAS);
    ASSERT_NE(nullptr, alias_query);
    Asset_Attr *secret_query = OH_Asset_ParseAttr(&result, ASSET_TAG_SECRET);
    ASSERT_NE(nullptr, secret_query);

    ASSERT_TRUE(CompareBlob(&alias_query->value.blob, &alias));
    ASSERT_TRUE(CompareBlob(&secret_query->value.blob, &secret));
    OH_Asset_FreeResultSet(&resultSet);
    ASSERT_EQ(ASSET_SUCCESS, RemoveByAlias(__func__));
}

/**
 * @tc.name: AssetAddTest.AssetAddTest002
 * @tc.desc: Add empty alias and secret, expect ASSET_INVALID_ARGUMENT
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetAddTest, AssetAddTest002, TestSize.Level0)
{
    Asset_Blob alias = { .size = strlen(__func__), .data = nullptr };
    Asset_Blob secret = { .size = 0, .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    Asset_Attr attr[] = {
        {
            .tag = ASSET_TAG_ALIAS,
            .value.blob = alias
        }, {
            .tag = ASSET_TAG_SECRET,
            .value.blob = secret
        }
    };
    ASSERT_EQ(ASSET_INVALID_ARGUMENT, OH_Asset_Add(attr, sizeof(attr) / sizeof(attr[0])));
}

/**
 * @tc.name: AssetAddTest.AssetAddTest003
 * @tc.desc: Add alias and secret with wrong blob-u32/blob-boolean data type, expect ASSET_INVALID_ARGUMENT
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetAddTest, AssetAddTest003, TestSize.Level0)
{
    Asset_Attr attr[] = {
        {
            .tag = ASSET_TAG_ALIAS,
            .value.u32 = 1
        }, {
            .tag = ASSET_TAG_SECRET,
            .value.boolean = true
        }
    };
    ASSERT_EQ(ASSET_INVALID_ARGUMENT, OH_Asset_Add(attr, sizeof(attr) / sizeof(attr[0])));
}

/**
 * @tc.name: AssetAddTest.AssetAddTest004
 * @tc.desc: Add alias and secret with wrong u32-boolean data type, expect ASSET_INVALID_ARGUMENT
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetAddTest, AssetAddTest004, TestSize.Level0)
{
    Asset_Blob alias = { .size = strlen(__func__), .data = nullptr };
    Asset_Blob secret = { .size = 0, .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    Asset_Attr attr[] = {
        {
            .tag = ASSET_TAG_ALIAS,
            .value.blob = alias
        }, {
            .tag = ASSET_TAG_SECRET,
            .value.blob = secret
        }, {
            .tag = ASSET_TAG_AUTH_TYPE,
            .value.boolean = false
        }
    };
    ASSERT_EQ(ASSET_INVALID_ARGUMENT, OH_Asset_Add(attr, sizeof(attr) / sizeof(attr[0])));
}

/**
 * @tc.name: AssetAddTest.AssetAddTest005
 * @tc.desc: Add alias and secret with wrong bool-blob data type, expect ASSET_INVALID_ARGUMENT
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetAddTest, AssetAddTest005, TestSize.Level0)
{
    Asset_Blob alias = { .size = strlen(__func__), .data = nullptr };
    Asset_Blob secret = { .size = 0, .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    Asset_Attr attr[] = {
        {
            .tag = ASSET_TAG_ALIAS,
            .value.blob = alias
        }, {
            .tag = ASSET_TAG_SECRET,
            .value.blob = secret
        }, {
            .tag = ASSET_TAG_REQUIRE_PASSWORD_SET,
            .value.blob = secret
        }
    };
    ASSERT_EQ(ASSET_INVALID_ARGUMENT, OH_Asset_Add(attr, sizeof(attr) / sizeof(attr[0])));
}

/**
 * @tc.name: AssetAddTest.AssetAddTest006
 * @tc.desc: Add alias and secret, then add again, expect duplicate
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetAddTest, AssetAddTest006, TestSize.Level0)
{
    Asset_Blob alias = { .size = strlen(__func__), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    Asset_Blob secret = { .size = strlen(__func__), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    Asset_Attr attr[] = {
        {
            .tag = ASSET_TAG_ALIAS,
            .value.blob = alias
        }, {
            .tag = ASSET_TAG_SECRET,
            .value.blob = secret
        }
    };
    ASSERT_EQ(ASSET_SUCCESS, OH_Asset_Add(attr, sizeof(attr) / sizeof(attr[0])));
    ASSERT_EQ(ASSET_DUPLICATED, OH_Asset_Add(attr, sizeof(attr) / sizeof(attr[0])));

    ASSERT_EQ(ASSET_SUCCESS, RemoveByAlias(__func__));
}
}