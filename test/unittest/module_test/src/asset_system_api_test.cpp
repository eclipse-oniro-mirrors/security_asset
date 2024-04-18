/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "asset_system_api_test.h"

#include <cstring>
#include <gtest/gtest.h>

#include "nativetoken_kit.h"
#include "token_setproc.h"

#include "asset_system_api.h"
#include "asset_system_type.h"
#include "asset_test_common.h"

using namespace testing::ext;
namespace UnitTest::AssetSystemApiTest {
int GrantSelfPermission()
{
    const char *permissions[] = {
        "ohos.permission.INTERACT_ACROSS_LOCAL_ACCOUNTS"
    };
    NativeTokenInfoParams info = {
        .dcapsNum = 0,
        .permsNum = 1,
        .aclsNum = 0,
        .dcaps = nullptr,
        .perms = permissions,
        .acls = nullptr,
        .processName = "asset_bin_test",
        .aplStr = "system_basic",
    };
    uint64_t tokenId = GetAccessTokenId(&info);
    return SetSelfTokenID(tokenId);
}

class AssetSystemApiTest : public testing::Test {
public:
    static void SetUpTestCase(void);

    static void TearDownTestCase(void);

    void SetUp(void);

    void TearDown(void);
};

void AssetSystemApiTest::SetUpTestCase(void)
{
    ASSERT_EQ(0, GrantSelfPermission());
}

void AssetSystemApiTest::TearDownTestCase(void)
{
}

void AssetSystemApiTest::SetUp(void)
{
}

void AssetSystemApiTest::TearDown(void)
{
}

/**
 * @tc.name: AssetSystemApiTest.AssetSystemApiTest001
 * @tc.desc: Test asset func AssetAdd specific user id, expect SUCCESS
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetSystemApiTest, AssetSystemApiTest001, TestSize.Level0)
{
    AssetBlob funcName = { .size = strlen(__func__), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    AssetAttr attr[] = {
        { .tag = SEC_ASSET_TAG_ALIAS, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_SECRET, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_ACCESSIBILITY, .value.u32 = SEC_ASSET_ACCESSIBILITY_DEVICE_UNLOCKED },
        { .tag = SEC_ASSET_TAG_USER_ID, .value.u32 = SPECIFIC_USER_ID },
        { .tag = SEC_ASSET_TAG_REQUIRE_PASSWORD_SET, .value.boolean = false },
        { .tag = SEC_ASSET_TAG_AUTH_TYPE, .value.u32 = SEC_ASSET_AUTH_TYPE_NONE },
        { .tag = SEC_ASSET_TAG_SYNC_TYPE, .value.u32 = SEC_ASSET_SYNC_TYPE_NEVER },
        { .tag = SEC_ASSET_TAG_CONFLICT_RESOLUTION, .value.u32 = SEC_ASSET_CONFLICT_OVERWRITE },
        { .tag = SEC_ASSET_TAG_DATA_LABEL_NORMAL_1, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_DATA_LABEL_NORMAL_2, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_DATA_LABEL_NORMAL_3, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_DATA_LABEL_NORMAL_4, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_DATA_LABEL_CRITICAL_1, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_DATA_LABEL_CRITICAL_2, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_DATA_LABEL_CRITICAL_3, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_DATA_LABEL_CRITICAL_4, .value.blob = funcName }
    };
    ASSERT_EQ(SEC_ASSET_SUCCESS, AssetAdd(attr, ARRAY_SIZE(attr)));
    AssetResultSet resultSet = { 0 };
    ASSERT_EQ(SEC_ASSET_SUCCESS, QueryByAliasSdk(__func__, &resultSet));
    ASSERT_EQ(1, resultSet.count);
    AssetResult result = resultSet.results[0];
    AssetAttr attr2[] = {
        { .tag = SEC_ASSET_TAG_ALIAS, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_SECRET, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_ACCESSIBILITY, .value.u32 = SEC_ASSET_ACCESSIBILITY_DEVICE_UNLOCKED },
        { .tag = SEC_ASSET_TAG_REQUIRE_PASSWORD_SET, .value.boolean = false },
        { .tag = SEC_ASSET_TAG_AUTH_TYPE, .value.u32 = SEC_ASSET_AUTH_TYPE_NONE },
        { .tag = SEC_ASSET_TAG_SYNC_TYPE, .value.u32 = SEC_ASSET_SYNC_TYPE_NEVER },
        { .tag = SEC_ASSET_TAG_CONFLICT_RESOLUTION, .value.u32 = SEC_ASSET_CONFLICT_OVERWRITE },
        { .tag = SEC_ASSET_TAG_DATA_LABEL_NORMAL_1, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_DATA_LABEL_NORMAL_2, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_DATA_LABEL_NORMAL_3, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_DATA_LABEL_NORMAL_4, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_DATA_LABEL_CRITICAL_1, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_DATA_LABEL_CRITICAL_2, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_DATA_LABEL_CRITICAL_3, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_DATA_LABEL_CRITICAL_4, .value.blob = funcName }
    };
    ASSERT_TRUE(CheckMatchAttrResultSdk(attr2, ARRAY_SIZE(attr2), &result));
    AssetFreeResultSet(&resultSet);
    ASSERT_EQ(SEC_ASSET_SUCCESS, RemoveByAliasSdk(__func__));
}

/**
 * @tc.name: AssetSystemApiTest.AssetSystemApiTest002
 * @tc.desc: Test asset func AssetQuery specific user id, expect SUCCESS
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetSystemApiTest, AssetSystemApiTest002, TestSize.Level0)
{
    AssetBlob funcName = { .size = strlen(__func__), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    AssetAttr addAttr[] = {
        { .tag = SEC_ASSET_TAG_ALIAS, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_SECRET, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_USER_ID, .value.u32 = SPECIFIC_USER_ID },
        { .tag = SEC_ASSET_TAG_ACCESSIBILITY, .value.u32 = SEC_ASSET_ACCESSIBILITY_DEVICE_UNLOCKED },
    };
    ASSERT_EQ(SEC_ASSET_SUCCESS, AssetAdd(addAttr, ARRAY_SIZE(addAttr)));

    AssetResultSet resultSet = { 0 };
    AssetAttr queryAttr[] = {
        { .tag = SEC_ASSET_TAG_ALIAS, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_USER_ID, .value.u32 = SPECIFIC_USER_ID },
        { .tag = SEC_ASSET_TAG_RETURN_TYPE, .value.u32 = SEC_ASSET_RETURN_ALL }
    };
    ASSERT_EQ(SEC_ASSET_SUCCESS, AssetQuery(queryAttr, ARRAY_SIZE(queryAttr), &resultSet));
    ASSERT_EQ(1, resultSet.count);

    AssetFreeResultSet(&resultSet);
    ASSERT_EQ(SEC_ASSET_SUCCESS, RemoveByAliasSdk(__func__));
}

/**
 * @tc.name: AssetSystemApiTest.AssetSystemApiTest003
 * @tc.desc: Test asset func AssetRemove, expect SUCCESS
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetSystemApiTest, AssetSystemApiTest003, TestSize.Level0)
{
    AssetBlob funcName = { .size = strlen(__func__), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    AssetAttr attr[] = {
        { .tag = SEC_ASSET_TAG_ALIAS, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_SECRET, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_ACCESSIBILITY, .value.u32 = SEC_ASSET_ACCESSIBILITY_DEVICE_UNLOCKED },
        { .tag = SEC_ASSET_TAG_REQUIRE_PASSWORD_SET, .value.boolean = false },
        { .tag = SEC_ASSET_TAG_USER_ID, .value.u32 = SPECIFIC_USER_ID },
        { .tag = SEC_ASSET_TAG_AUTH_TYPE, .value.u32 = SEC_ASSET_AUTH_TYPE_NONE },
        { .tag = SEC_ASSET_TAG_SYNC_TYPE, .value.u32 = SEC_ASSET_SYNC_TYPE_NEVER },
        { .tag = SEC_ASSET_TAG_DATA_LABEL_NORMAL_1, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_DATA_LABEL_NORMAL_2, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_DATA_LABEL_NORMAL_3, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_DATA_LABEL_NORMAL_4, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_DATA_LABEL_CRITICAL_1, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_DATA_LABEL_CRITICAL_2, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_DATA_LABEL_CRITICAL_3, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_DATA_LABEL_CRITICAL_4, .value.blob = funcName }
    };
    ASSERT_EQ(SEC_ASSET_SUCCESS, AssetAdd(attr, ARRAY_SIZE(attr)));
    AssetAttr attr2[] = {
        { .tag = SEC_ASSET_TAG_ALIAS, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_ACCESSIBILITY, .value.u32 = SEC_ASSET_ACCESSIBILITY_DEVICE_UNLOCKED },
        { .tag = SEC_ASSET_TAG_REQUIRE_PASSWORD_SET, .value.boolean = false },
        { .tag = SEC_ASSET_TAG_USER_ID, .value.u32 = SPECIFIC_USER_ID },
        { .tag = SEC_ASSET_TAG_AUTH_TYPE, .value.u32 = SEC_ASSET_AUTH_TYPE_NONE },
        { .tag = SEC_ASSET_TAG_SYNC_TYPE, .value.u32 = SEC_ASSET_SYNC_TYPE_NEVER },
        { .tag = SEC_ASSET_TAG_IS_PERSISTENT, .value.boolean = false },
        { .tag = SEC_ASSET_TAG_DATA_LABEL_NORMAL_1, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_DATA_LABEL_NORMAL_2, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_DATA_LABEL_NORMAL_3, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_DATA_LABEL_NORMAL_4, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_DATA_LABEL_CRITICAL_1, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_DATA_LABEL_CRITICAL_2, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_DATA_LABEL_CRITICAL_3, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_DATA_LABEL_CRITICAL_4, .value.blob = funcName }
    };
    ASSERT_EQ(SEC_ASSET_SUCCESS, AssetRemove(attr2, ARRAY_SIZE(attr2)));
}

/**
 * @tc.name: AssetSystemApiTest.AssetSystemApiTest004
 * @tc.desc: Test asset func AssetPreQuery, AssetPostQuery expect SUCCESS
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetSystemApiTest, AssetSystemApiTest004, TestSize.Level0)
{
    AssetBlob funcName = { .size = strlen(__func__), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    AssetAttr attr[] = {
        { .tag = SEC_ASSET_TAG_ALIAS, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_SECRET, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_USER_ID, .value.u32 = SPECIFIC_USER_ID },
        { .tag = SEC_ASSET_TAG_ACCESSIBILITY, .value.u32 = SEC_ASSET_ACCESSIBILITY_DEVICE_UNLOCKED },
        { .tag = SEC_ASSET_TAG_AUTH_TYPE, .value.u32 = SEC_ASSET_AUTH_TYPE_ANY }
    };
    ASSERT_EQ(SEC_ASSET_SUCCESS, AssetAdd(attr, ARRAY_SIZE(attr)));

    AssetBlob challenge = { 0 };
    AssetAttr attr2[] = {
        { .tag = SEC_ASSET_TAG_ALIAS, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_USER_ID, .value.u32 = SPECIFIC_USER_ID }
    };
    ASSERT_EQ(SEC_ASSET_SUCCESS, AssetPreQuery(attr2, ARRAY_SIZE(attr2), &challenge));

    AssetAttr attr3[] = {
        { .tag = SEC_ASSET_TAG_AUTH_CHALLENGE, .value.blob = challenge },
        { .tag = SEC_ASSET_TAG_USER_ID, .value.u32 = SPECIFIC_USER_ID }
    };
    ASSERT_EQ(SEC_ASSET_SUCCESS, AssetPostQuery(attr3, ARRAY_SIZE(attr3)));

    AssetAttr attr4[] = {
        { .tag = SEC_ASSET_TAG_ALIAS, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_USER_ID, .value.u32 = SPECIFIC_USER_ID }
    };
    ASSERT_EQ(SEC_ASSET_SUCCESS, AssetRemove(attr4, ARRAY_SIZE(attr4)));
}

/**
 * @tc.name: AssetSystemApiTest.AssetSystemApiTest005
 * @tc.desc: Test asset func AssetUpdate expect SUCCESS
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetSystemApiTest, AssetSystemApiTest005, TestSize.Level0)
{
    AssetBlob funcName = { .size = strlen(__func__), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    AssetAttr attr[] = {
        { .tag = SEC_ASSET_TAG_ALIAS, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_SECRET, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_USER_ID, .value.u32 = SPECIFIC_USER_ID },
        { .tag = SEC_ASSET_TAG_ACCESSIBILITY, .value.u32 = SEC_ASSET_ACCESSIBILITY_DEVICE_UNLOCKED },
        { .tag = SEC_ASSET_TAG_AUTH_TYPE, .value.u32 = SEC_ASSET_AUTH_TYPE_ANY }
    };
    ASSERT_EQ(SEC_ASSET_SUCCESS, AssetAdd(attr, ARRAY_SIZE(attr)));

    AssetAttr queryAttr[] = {
        { .tag = SEC_ASSET_TAG_ALIAS, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_USER_ID, .value.u32 = SPECIFIC_USER_ID },
    };
    const char *secretNew = "secret_new";
    AssetAttr updateAttr[] = {
        { .tag = SEC_ASSET_TAG_SECRET, .value.blob =
            { .size = strlen(secretNew), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(secretNew)) } }
    };
    ASSERT_EQ(SEC_ASSET_SUCCESS, AssetUpdate(queryAttr, ARRAY_SIZE(queryAttr), updateAttr, ARRAY_SIZE(updateAttr)));

    ASSERT_EQ(SEC_ASSET_SUCCESS, RemoveByAliasSdk(__func__));
}
}