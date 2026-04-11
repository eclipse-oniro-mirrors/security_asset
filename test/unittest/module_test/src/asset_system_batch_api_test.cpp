/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include "asset_permission_change.h"

using namespace testing::ext;
namespace UnitTest::AssetSystemBatchApiTest {

class AssetSystemBatchApiTest : public testing::Test {
public:
    static void SetUpTestCase(void);

    static void TearDownTestCase(void);

    void SetUp(void);

    void TearDown(void);
};

void AssetSystemBatchApiTest::SetUpTestCase(void)
{
    ASSERT_EQ(0, GrantSelfPermission());
}

void AssetSystemBatchApiTest::TearDownTestCase(void)
{
}

void AssetSystemBatchApiTest::SetUp(void)
{
}

void AssetSystemBatchApiTest::TearDown(void)
{
}

/**
 * @tc.name: AssetSystemBatchApiTest.AssetSystemBatchApiTest001
 * @tc.desc: Test asset func AssetBatchAdd expect SUCCESS
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetSystemBatchApiTest, AssetSystemBatchApiTest001, TestSize.Level0)
{
    AssetBlob funcName = { .size = strlen(__func__), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    std::vector<AssetAttr> attr = {
        { .tag = SEC_ASSET_TAG_ALIAS, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_SECRET, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_USER_ID, .value.u32 = SPECIFIC_USER_ID },
        { .tag = SEC_ASSET_TAG_ACCESSIBILITY, .value.u32 = SEC_ASSET_ACCESSIBILITY_DEVICE_UNLOCKED },
        { .tag = SEC_ASSET_TAG_AUTH_TYPE, .value.u32 = SEC_ASSET_AUTH_TYPE_ANY }
    };
    std::vector<std::vector<AssetAttr>> param;
    param.push_back(attr);
    std::vector<std::pair<uint32_t, uint32_t>> errInfoArray;
    ASSERT_EQ(SEC_ASSET_SUCCESS, AssetBatchAdd(param, errInfoArray));
}

/**
 * @tc.name: AssetSystemBatchApiTest.AssetSystemBatchApiTest002
 * @tc.desc: Test asset func AssetBatchRemove expect SUCCESS
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetSystemBatchApiTest, AssetSystemBatchApiTest002, TestSize.Level0)
{
    AssetBlob funcName = { .size = strlen(__func__), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    std::vector<AssetAttr> attr = {
        { .tag = SEC_ASSET_TAG_ALIAS, .value.blob = funcName },
    };
    std::vector<std::vector<AssetAttr>> param;
    param.push_back(attr);
    ASSERT_EQ(SEC_ASSET_SUCCESS, AssetBatchRemove(param));
}

/**
 * @tc.name: AssetSystemBatchApiTest.AssetSystemBatchApiTest003
 * @tc.desc: Test asset func AssetBatchRemove expect FAIL
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetSystemBatchApiTest, AssetSystemBatchApiTest003, TestSize.Level0)
{
    std::vector<std::vector<AssetAttr>> param;
    ASSERT_EQ(ASSET_INVALID_ARGUMENT, AssetBatchRemove(param));
}

/**
 * @tc.name: AssetSystemBatchApiTest.AssetSystemBatchApiTest004
 * @tc.desc: Test asset func AssetBatchAdd expect FAIL
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetSystemBatchApiTest, AssetSystemBatchApiTest004, TestSize.Level0)
{
    std::vector<std::vector<AssetAttr>> param;
    std::vector<std::pair<uint32_t, uint32_t>> errInfoArray;
    ASSERT_EQ(ASSET_INVALID_ARGUMENT, AssetBatchAdd(param, errInfoArray));
}

/**
 * @tc.name: AssetSystemBatchApiTest.AssetSystemBatchApiTest005
 * @tc.desc: Test asset func AssetBatchAdd, AssetBatchRemove expect SUCCESS
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetSystemBatchApiTest, AssetSystemBatchApiTest005, TestSize.Level0)
{
    AssetBlob funcName = { .size = strlen(__func__), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    std::vector<AssetAttr> attr = {
        { .tag = SEC_ASSET_TAG_ALIAS, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_SECRET, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_USER_ID, .value.u32 = SPECIFIC_USER_ID },
        { .tag = SEC_ASSET_TAG_ACCESSIBILITY, .value.u32 = SEC_ASSET_ACCESSIBILITY_DEVICE_UNLOCKED },
        { .tag = SEC_ASSET_TAG_AUTH_TYPE, .value.u32 = SEC_ASSET_AUTH_TYPE_ANY }
    };
    std::vector<std::vector<AssetAttr>> param;
    param.push_back(attr);
    std::vector<std::pair<uint32_t, uint32_t>> errInfoArray;
    ASSERT_EQ(SEC_ASSET_SUCCESS, AssetBatchAdd(param, errInfoArray));

    std::vector<AssetAttr> attrToRemove = {
        { .tag = SEC_ASSET_TAG_ALIAS, .value.blob = funcName },
    };
    std::vector<std::vector<AssetAttr>> paramToRemove;
    paramToRemove.push_back(attrToRemove);
    ASSERT_EQ(SEC_ASSET_SUCCESS, AssetBatchRemove(paramToRemove));
}

/**
 * @tc.name: AssetSystemBatchApiTest.AssetSystemBatchApiTest006
 * @tc.desc: Test asset func AssetBatchUpdate expect SUCCESS
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetSystemBatchApiTest, AssetSystemBatchApiTest006, TestSize.Level0)
{
    AssetBlob funcName = { .size = strlen(__func__), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    AssetAttr attr[] = {
        { .tag = SEC_ASSET_TAG_ALIAS, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_SECRET, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_USER_ID, .value.u32 = SPECIFIC_USER_ID },
        { .tag = SEC_ASSET_TAG_ACCESSIBILITY, .value.u32 = SEC_ASSET_ACCESSIBILITY_DEVICE_UNLOCKED },
        { .tag = SEC_ASSET_TAG_AUTH_TYPE, .value.u32 = SEC_ASSET_AUTH_TYPE_ANY },
        { .tag = SEC_ASSET_TAG_CONFLICT_RESOLUTION, .value.u32 = SEC_ASSET_CONFLICT_OVERWRITE },
    };
    ASSERT_EQ(SEC_ASSET_SUCCESS, AssetAdd(attr, ARRAY_SIZE(attr)));

    std::vector<AssetAttr> queryAttr = {
        { .tag = SEC_ASSET_TAG_ALIAS, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_USER_ID, .value.u32 = SPECIFIC_USER_ID },
    };
    const char *secretNew = "secret_new";
    std::vector<AssetAttr> updateAttr = {
        { .tag = SEC_ASSET_TAG_SECRET, .value.blob =
            { .size = strlen(secretNew), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(secretNew)) } }
    };
    std::vector<std::vector<AssetAttr>> paramQuery;
    paramQuery.push_back(queryAttr);

    std::vector<std::vector<AssetAttr>> paramUpdate;
    paramUpdate.push_back(updateAttr);

    std::vector<std::pair<uint32_t, uint32_t>> errInfoArray;
    ASSERT_EQ(SEC_ASSET_SUCCESS, AssetBatchUpdate(paramQuery, paramUpdate, errInfoArray));
}

/**
 * @tc.name: AssetSystemBatchApiTest.AssetSystemBatchApiTest007
 * @tc.desc: Test asset func AssetBatchUpdate expect FAIL
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetSystemBatchApiTest, AssetSystemBatchApiTest007, TestSize.Level0)
{
    AssetBlob funcName = { .size = strlen(__func__), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    AssetAttr attr[] = {
        { .tag = SEC_ASSET_TAG_ALIAS, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_SECRET, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_USER_ID, .value.u32 = SPECIFIC_USER_ID },
        { .tag = SEC_ASSET_TAG_ACCESSIBILITY, .value.u32 = SEC_ASSET_ACCESSIBILITY_DEVICE_UNLOCKED },
        { .tag = SEC_ASSET_TAG_AUTH_TYPE, .value.u32 = SEC_ASSET_AUTH_TYPE_ANY },
        { .tag = SEC_ASSET_TAG_CONFLICT_RESOLUTION, .value.u32 = SEC_ASSET_CONFLICT_OVERWRITE },
    };
    ASSERT_EQ(SEC_ASSET_SUCCESS, AssetAdd(attr, ARRAY_SIZE(attr)));

    std::vector<AssetAttr> queryAttr = {
        { .tag = SEC_ASSET_TAG_ALIAS, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_USER_ID, .value.u32 = SPECIFIC_USER_ID },
    };
    const char *secretNew = "secret_new";
    std::vector<AssetAttr> updateAttr = {
        { .tag = SEC_ASSET_TAG_SECRET, .value.blob =
            { .size = strlen(secretNew), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(secretNew)) } }
    };
    std::vector<std::vector<AssetAttr>> paramQuery;

    std::vector<std::vector<AssetAttr>> paramUpdate;
    paramUpdate.push_back(updateAttr);

    std::vector<std::pair<uint32_t, uint32_t>> errInfoArray;
    ASSERT_EQ(ASSET_INVALID_ARGUMENT, AssetBatchUpdate(paramQuery, paramUpdate, errInfoArray));
}

/**
 * @tc.name: AssetSystemBatchApiTest.AssetSystemBatchApiTest008
 * @tc.desc: Test asset func AssetBatchUpdate expect FAIL
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetSystemBatchApiTest, AssetSystemBatchApiTest008, TestSize.Level0)
{
    AssetBlob funcName = { .size = strlen(__func__), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    AssetAttr attr[] = {
        { .tag = SEC_ASSET_TAG_ALIAS, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_SECRET, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_USER_ID, .value.u32 = SPECIFIC_USER_ID },
        { .tag = SEC_ASSET_TAG_ACCESSIBILITY, .value.u32 = SEC_ASSET_ACCESSIBILITY_DEVICE_UNLOCKED },
        { .tag = SEC_ASSET_TAG_AUTH_TYPE, .value.u32 = SEC_ASSET_AUTH_TYPE_ANY },
        { .tag = SEC_ASSET_TAG_CONFLICT_RESOLUTION, .value.u32 = SEC_ASSET_CONFLICT_OVERWRITE },
    };
    ASSERT_EQ(SEC_ASSET_SUCCESS, AssetAdd(attr, ARRAY_SIZE(attr)));

    std::vector<AssetAttr> queryAttr = {
        { .tag = SEC_ASSET_TAG_ALIAS, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_USER_ID, .value.u32 = SPECIFIC_USER_ID },
    };
    const char *secretNew = "secret_new";
    std::vector<AssetAttr> updateAttr = {
        { .tag = SEC_ASSET_TAG_SECRET, .value.blob =
            { .size = strlen(secretNew), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(secretNew)) } }
    };
    std::vector<std::vector<AssetAttr>> paramQuery;
    paramQuery.push_back(queryAttr);

    std::vector<std::vector<AssetAttr>> paramUpdate;

    std::vector<std::pair<uint32_t, uint32_t>> errInfoArray;
    ASSERT_EQ(ASSET_INVALID_ARGUMENT, AssetBatchUpdate(paramQuery, paramUpdate, errInfoArray));
}

/**
 * @tc.name: AssetSystemBatchApiTest.AssetSystemBatchApiTest009
 * @tc.desc: Test asset func AssetBatchUpdate expect FAIL
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetSystemBatchApiTest, AssetSystemBatchApiTest009, TestSize.Level0)
{
    AssetBlob funcName = { .size = strlen(__func__), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    AssetAttr attr[] = {
        { .tag = SEC_ASSET_TAG_ALIAS, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_SECRET, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_USER_ID, .value.u32 = SPECIFIC_USER_ID },
        { .tag = SEC_ASSET_TAG_ACCESSIBILITY, .value.u32 = SEC_ASSET_ACCESSIBILITY_DEVICE_UNLOCKED },
        { .tag = SEC_ASSET_TAG_AUTH_TYPE, .value.u32 = SEC_ASSET_AUTH_TYPE_ANY },
        { .tag = SEC_ASSET_TAG_CONFLICT_RESOLUTION, .value.u32 = SEC_ASSET_CONFLICT_OVERWRITE },
    };
    ASSERT_EQ(SEC_ASSET_SUCCESS, AssetAdd(attr, ARRAY_SIZE(attr)));

    std::vector<AssetAttr> queryAttr = {
        { .tag = SEC_ASSET_TAG_ALIAS, .value.blob = funcName },
        { .tag = SEC_ASSET_TAG_USER_ID, .value.u32 = SPECIFIC_USER_ID },
    };
    const char *secretNew = "secret_new";
    std::vector<AssetAttr> updateAttr = {
        { .tag = SEC_ASSET_TAG_SECRET, .value.blob =
            { .size = strlen(secretNew), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(secretNew)) } }
    };
    std::vector<std::vector<AssetAttr>> paramQuery;

    std::vector<std::vector<AssetAttr>> paramUpdate;

    std::vector<std::pair<uint32_t, uint32_t>> errInfoArray;
    ASSERT_EQ(ASSET_INVALID_ARGUMENT, AssetBatchUpdate(paramQuery, paramUpdate, errInfoArray));
}
}