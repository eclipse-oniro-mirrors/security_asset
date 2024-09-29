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
#include "asset_auth_query_test.h"

#include <string>
#include <gtest/gtest.h>

#include "asset_api.h"
#include "asset_test_common.h"

using namespace testing::ext;
namespace UnitTest::AssetAuthQueryTest {
class AssetAuthQueryTest : public testing::Test {
public:
    static void SetUpTestCase(void);

    static void TearDownTestCase(void);

    void SetUp(void);

    void TearDown(void);
};

void AssetAuthQueryTest::SetUpTestCase(void)
{
}

void AssetAuthQueryTest::TearDownTestCase(void)
{
}

void AssetAuthQueryTest::SetUp(void)
{
}

void AssetAuthQueryTest::TearDown(void)
{
}

/**
 * @tc.name: AssetAuthQueryTest.AssetAuthQueryTest001
 * @tc.desc: Add auth asset, pre-query, query added auth asset with wrong auth token, post-query,
 *     expect ASSET_INVALID_ARGUMENT.
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetAuthQueryTest, AssetAuthQueryTest001, TestSize.Level0)
{
    Asset_Blob funcName = { .size = strlen(__func__), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    Asset_Attr addAttr[] = {
        { .tag = ASSET_TAG_ALIAS, .value.blob = funcName },
        { .tag = ASSET_TAG_SECRET, .value.blob = funcName },
        { .tag = ASSET_TAG_ACCESSIBILITY, .value.u32 = ASSET_ACCESSIBILITY_DEVICE_POWERED_ON },
        { .tag = ASSET_TAG_AUTH_TYPE, .value.u32 = ASSET_AUTH_TYPE_ANY },
    };
    ASSERT_EQ(ASSET_SUCCESS, OH_Asset_Add(addAttr, ARRAY_SIZE(addAttr)));

    Asset_Attr preQueryAttr[] = {};
    Asset_Blob challenge = { 0 };
    ASSERT_EQ(ASSET_SUCCESS, OH_Asset_PreQuery(preQueryAttr, ARRAY_SIZE(preQueryAttr), &challenge));

    Asset_ResultSet queryResultSet = { 0 };
    Asset_Attr queryAttr[] = {
        { .tag = ASSET_TAG_ALIAS, .value.blob = funcName },
        { .tag = ASSET_TAG_RETURN_TYPE, .value.u32 = ASSET_RETURN_ALL },
        { .tag = ASSET_TAG_AUTH_CHALLENGE, .value.blob = challenge },
        { .tag = ASSET_TAG_AUTH_TOKEN, .value.blob = funcName },
    };
    ASSERT_EQ(ASSET_INVALID_ARGUMENT, OH_Asset_Query(queryAttr, ARRAY_SIZE(queryAttr), &queryResultSet));

    Asset_Attr postQueryAttr[] = {
        { .tag = ASSET_TAG_AUTH_CHALLENGE, .value.blob = challenge },
    };
    ASSERT_EQ(ASSET_SUCCESS, OH_Asset_PostQuery(postQueryAttr, ARRAY_SIZE(postQueryAttr)));

    Asset_Blob blob = { .size = 0, .data = nullptr };
    OH_Asset_FreeBlob(&blob);
    OH_Asset_FreeBlob(nullptr);
    Asset_ResultSet resultSet = { .count = 0, .results = nullptr };
    OH_Asset_FreeResultSet(&queryResultSet);
    OH_Asset_FreeResultSet(&resultSet);
    OH_Asset_FreeResultSet(nullptr);
    ASSERT_EQ(ASSET_SUCCESS, RemoveByAliasNdk(__func__));
}

/**
 * @tc.name: AssetAuthQueryTest.AssetAuthQueryTest002
 * @tc.desc: Add auth asset, pre-query, query added auth asset without auth token, post-query,
 *     expect ASSET_INVALID_ARGUMENT.
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetAuthQueryTest, AssetAuthQueryTest002, TestSize.Level0)
{
    Asset_Blob funcName = { .size = strlen(__func__), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    Asset_Attr addAttr[] = {
        { .tag = ASSET_TAG_ALIAS, .value.blob = funcName },
        { .tag = ASSET_TAG_SECRET, .value.blob = funcName },
        { .tag = ASSET_TAG_ACCESSIBILITY, .value.u32 = ASSET_ACCESSIBILITY_DEVICE_POWERED_ON },
        { .tag = ASSET_TAG_AUTH_TYPE, .value.u32 = ASSET_AUTH_TYPE_ANY },
    };
    ASSERT_EQ(ASSET_SUCCESS, OH_Asset_Add(addAttr, ARRAY_SIZE(addAttr)));

    Asset_Attr preQueryAttr[] = {};
    Asset_Blob challenge = { 0 };
    ASSERT_EQ(ASSET_SUCCESS, OH_Asset_PreQuery(preQueryAttr, ARRAY_SIZE(preQueryAttr), &challenge));

    Asset_ResultSet queryResultSet = { 0 };
    Asset_Attr queryAttr[] = {
        { .tag = ASSET_TAG_ALIAS, .value.blob = funcName },
        { .tag = ASSET_TAG_RETURN_TYPE, .value.u32 = ASSET_RETURN_ALL },
        { .tag = ASSET_TAG_AUTH_CHALLENGE, .value.blob = challenge },
    };
    ASSERT_EQ(ASSET_INVALID_ARGUMENT, OH_Asset_Query(queryAttr, ARRAY_SIZE(queryAttr), &queryResultSet));

    Asset_Attr postQueryAttr[] = {
        { .tag = ASSET_TAG_AUTH_CHALLENGE, .value.blob = challenge },
    };
    ASSERT_EQ(ASSET_SUCCESS, OH_Asset_PostQuery(postQueryAttr, ARRAY_SIZE(postQueryAttr)));

    Asset_Blob blob = { .size = 0, .data = nullptr };
    OH_Asset_FreeBlob(&blob);
    OH_Asset_FreeBlob(nullptr);
    Asset_ResultSet resultSet = { .count = 0, .results = nullptr };
    OH_Asset_FreeResultSet(&queryResultSet);
    OH_Asset_FreeResultSet(&resultSet);
    OH_Asset_FreeResultSet(nullptr);
    ASSERT_EQ(ASSET_SUCCESS, RemoveByAliasNdk(__func__));
}

/**
 * @tc.name: AssetAuthQueryTest.AssetAuthQueryTest003
 * @tc.desc: Pre-query, expect ASSET_NOT_FOUND.
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetAuthQueryTest, AssetAuthQueryTest003, TestSize.Level0)
{
    Asset_Attr preQueryAttr[] = {};
    Asset_Blob challenge = { 0 };
    ASSERT_EQ(ASSET_NOT_FOUND, OH_Asset_PreQuery(preQueryAttr, ARRAY_SIZE(preQueryAttr), &challenge));

    Asset_Blob blob = { .size = 0, .data = nullptr };
    OH_Asset_FreeBlob(&blob);
    OH_Asset_FreeBlob(nullptr);
    Asset_ResultSet resultSet = { .count = 0, .results = nullptr };
    OH_Asset_FreeResultSet(&resultSet);
    OH_Asset_FreeResultSet(nullptr);
}

/**
 * @tc.name: AssetAuthQueryTest.AssetAuthQueryTest004
 * @tc.desc: Add auth asset, pre-query, post-query, expect ASSET_SUCCESS.
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetAuthQueryTest, AssetAuthQueryTest004, TestSize.Level0)
{
    Asset_Blob funcName = { .size = strlen(__func__), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    Asset_Attr addAttr[] = {
        { .tag = ASSET_TAG_ALIAS, .value.blob = funcName },
        { .tag = ASSET_TAG_SECRET, .value.blob = funcName },
        { .tag = ASSET_TAG_ACCESSIBILITY, .value.u32 = ASSET_ACCESSIBILITY_DEVICE_POWERED_ON },
        { .tag = ASSET_TAG_AUTH_TYPE, .value.u32 = ASSET_AUTH_TYPE_ANY },
    };
    ASSERT_EQ(ASSET_SUCCESS, OH_Asset_Add(addAttr, ARRAY_SIZE(addAttr)));

    Asset_Attr preQueryAttr[] = {};
    Asset_Blob challenge = { 0 };
    ASSERT_EQ(ASSET_SUCCESS, OH_Asset_PreQuery(preQueryAttr, ARRAY_SIZE(preQueryAttr), &challenge));

    Asset_Attr postQueryAttr[] = {
        { .tag = ASSET_TAG_AUTH_CHALLENGE, .value.blob = challenge },
    };
    ASSERT_EQ(ASSET_SUCCESS, OH_Asset_PostQuery(postQueryAttr, ARRAY_SIZE(postQueryAttr)));


    Asset_Blob blob = { .size = 0, .data = nullptr };
    OH_Asset_FreeBlob(&blob);
    OH_Asset_FreeBlob(nullptr);
    Asset_ResultSet resultSet = { .count = 0, .results = nullptr };
    OH_Asset_FreeResultSet(&resultSet);
    OH_Asset_FreeResultSet(nullptr);
    ASSERT_EQ(ASSET_SUCCESS, RemoveByAliasNdk(__func__));
}
}