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

#include "asset_system_api.h"
#include "asset_test_common.h"

using namespace testing::ext;
namespace UnitTest::AssetAddTest {
class AssetAddTest : public testing::Test {
public:
    static void SetUpTestCase(void);

    static void TearDownTestCase(void);

    void SetUp(void);

    void TearDown(void);
};

void AssetAddTest::SetUpTestCase(void)
{
}

void AssetAddTest::TearDownTestCase(void)
{
}

void AssetAddTest::SetUp(void)
{
}

void AssetAddTest::TearDown(void)
{
}

bool checkMatchAttrResult(const AssetAttr *attrs, uint32_t attrCnt, const AssetResult *result)
{
    for (uint32_t i = 0; i < attrCnt; i++) {
        if (attrs[i].tag == ASSET_SYSTEM_TAG_CONFLICT_RESOLUTION) {
            continue;
        }
        AssetAttr *res = AssetParseAttr(result, static_cast<AssetTag>(attrs[i].tag));
        if (res == nullptr) {
            return false;
        }
        switch (attrs[i].tag & ASSET_SYSTEM_TAG_TYPE_MASK) {
            case ASSET_SYSTEM_TYPE_BOOL:
                if (attrs[i].value.boolean != res->value.boolean) {
                    printf("tag is %x, %u vs %u", attrs[i].tag, attrs[i].value.boolean, res->value.boolean);
                    return false;
                }
                break;
            case ASSET_SYSTEM_TYPE_NUMBER:
                if (attrs[i].value.u32 != res->value.u32) {
                    printf("tag is %x, %u vs %u", attrs[i].tag, attrs[i].value.u32, res->value.u32);
                    return false;
                }
                break;
            case ASSET_SYSTEM_TYPE_BYTES:
                if (!CompareBlob(&attrs[i].value.blob, &res->value.blob)) {
                    printf("tag is %x, len %u vs len %u", attrs[i].tag, attrs[i].value.blob.size, res->value.blob.size);
                    return false;
                }
                break;
            default:
                return false;
        };
    }
    return true;
}

/**
 * @tc.name: AssetAddTest.AssetAddTest001
 * @tc.desc: Add asset with all attrs, then query, expect success and match
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetAddTest, AssetAddTest001, TestSize.Level0)
{
    AssetBlob funcName = { .size = strlen(__func__), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    AssetAttr attr[] = {
        { .tag = ASSET_SYSTEM_TAG_ALIAS, .value.blob = funcName },
        { .tag = ASSET_SYSTEM_TAG_SECRET, .value.blob = funcName },
        { .tag = ASSET_SYSTEM_TAG_ACCESSIBILITY, .value.u32 = ASSET_SYSTEM_ACCESSIBILITY_DEVICE_POWERED_ON },
        { .tag = ASSET_SYSTEM_TAG_REQUIRE_PASSWORD_SET, .value.boolean = false },
        { .tag = ASSET_SYSTEM_TAG_AUTH_TYPE, .value.u32 = ASSET_SYSTEM_AUTH_TYPE_NONE },
        { .tag = ASSET_SYSTEM_TAG_SYNC_TYPE, .value.u32 = ASSET_SYSTEM_SYNC_TYPE_NEVER },
        { .tag = ASSET_SYSTEM_TAG_CONFLICT_RESOLUTION, .value.u32 = ASSET_SYSTEM_CONFLICT_OVERWRITE },
        { .tag = ASSET_SYSTEM_TAG_DATA_LABEL_NORMAL_1, .value.blob = funcName },
        { .tag = ASSET_SYSTEM_TAG_DATA_LABEL_NORMAL_2, .value.blob = funcName },
        { .tag = ASSET_SYSTEM_TAG_DATA_LABEL_NORMAL_3, .value.blob = funcName },
        { .tag = ASSET_SYSTEM_TAG_DATA_LABEL_NORMAL_4, .value.blob = funcName },
        { .tag = ASSET_SYSTEM_TAG_DATA_LABEL_CRITICAL_1, .value.blob = funcName },
        { .tag = ASSET_SYSTEM_TAG_DATA_LABEL_CRITICAL_2, .value.blob = funcName },
        { .tag = ASSET_SYSTEM_TAG_DATA_LABEL_CRITICAL_3, .value.blob = funcName },
        { .tag = ASSET_SYSTEM_TAG_DATA_LABEL_CRITICAL_4, .value.blob = funcName }
    };
    ASSERT_EQ(ASSET_SYSTEM_SUCCESS, AssetAdd(attr, ARRAY_SIZE(attr)));

    AssetResultSet resultSet = { 0 };
    ASSERT_EQ(ASSET_SYSTEM_SUCCESS, QueryByAlias(__func__, &resultSet));
    ASSERT_EQ(1, resultSet.count);
    AssetResult result = resultSet.results[0];
    ASSERT_EQ(true, checkMatchAttrResult(attr, ARRAY_SIZE(attr), &result));

    AssetFreeResultSet(&resultSet);
    ASSERT_EQ(ASSET_SYSTEM_SUCCESS, RemoveByAlias(__func__));
}

/**
 * @tc.name: AssetAddTest.AssetAddTest002
 * @tc.desc: Add empty alias and secret, expect ASSET_SYSTEM_INVALID_ARGUMENT
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetAddTest, AssetAddTest002, TestSize.Level0)
{
    AssetBlob alias = { .size = strlen(__func__), .data = nullptr };
    AssetBlob secret = { .size = 0, .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    AssetAttr attr[] = {
        { .tag = ASSET_SYSTEM_TAG_ALIAS, .value.blob = alias },
        { .tag = ASSET_SYSTEM_TAG_SECRET, .value.blob = secret },
        { .tag = ASSET_SYSTEM_TAG_ACCESSIBILITY, .value.u32 = ASSET_SYSTEM_ACCESSIBILITY_DEVICE_POWERED_ON },
    };
    ASSERT_EQ(ASSET_SYSTEM_INVALID_ARGUMENT, AssetAdd(attr, ARRAY_SIZE(attr)));
}

/**
 * @tc.name: AssetAddTest.AssetAddTest003
 * @tc.desc: Add alias and secret with wrong blob-u32/blob-boolean data type, expect ASSET_SYSTEM_INVALID_ARGUMENT
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetAddTest, AssetAddTest003, TestSize.Level0)
{
    AssetAttr attr[] = {
        { .tag = ASSET_SYSTEM_TAG_ALIAS, .value.u32 = 1 },
        { .tag = ASSET_SYSTEM_TAG_SECRET, .value.boolean = true },
        { .tag = ASSET_SYSTEM_TAG_ACCESSIBILITY, .value.u32 = ASSET_SYSTEM_ACCESSIBILITY_DEVICE_POWERED_ON },
    };
    ASSERT_EQ(ASSET_SYSTEM_INVALID_ARGUMENT, AssetAdd(attr, ARRAY_SIZE(attr)));
}

/**
 * @tc.name: AssetAddTest.AssetAddTest004
 * @tc.desc: Add alias and secret with wrong u32-boolean data type, expect ASSET_SYSTEM_INVALID_ARGUMENT
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetAddTest, AssetAddTest004, TestSize.Level0)
{
    AssetBlob alias = { .size = strlen(__func__), .data = nullptr };
    AssetBlob secret = { .size = 0, .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    AssetAttr attr[] = {
        { .tag = ASSET_SYSTEM_TAG_ALIAS, .value.blob = alias },
        { .tag = ASSET_SYSTEM_TAG_SECRET, .value.blob = secret },
        { .tag = ASSET_SYSTEM_TAG_AUTH_TYPE, .value.boolean = false },
        { .tag = ASSET_SYSTEM_TAG_ACCESSIBILITY, .value.u32 = ASSET_SYSTEM_ACCESSIBILITY_DEVICE_POWERED_ON },
    };
    ASSERT_EQ(ASSET_SYSTEM_INVALID_ARGUMENT, AssetAdd(attr, ARRAY_SIZE(attr)));
}

/**
 * @tc.name: AssetAddTest.AssetAddTest005
 * @tc.desc: Add alias and secret with wrong bool-blob data type, expect ASSET_SYSTEM_INVALID_ARGUMENT
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetAddTest, AssetAddTest005, TestSize.Level0)
{
    AssetBlob alias = { .size = strlen(__func__), .data = nullptr };
    AssetBlob secret = { .size = 0, .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    AssetAttr attr[] = {
        { .tag = ASSET_SYSTEM_TAG_ALIAS, .value.blob = alias },
        { .tag = ASSET_SYSTEM_TAG_SECRET, .value.blob = secret },
        { .tag = ASSET_SYSTEM_TAG_REQUIRE_PASSWORD_SET, .value.blob = secret },
        { .tag = ASSET_SYSTEM_TAG_ACCESSIBILITY, .value.u32 = ASSET_SYSTEM_ACCESSIBILITY_DEVICE_POWERED_ON },
    };
    ASSERT_EQ(ASSET_SYSTEM_INVALID_ARGUMENT, AssetAdd(attr, ARRAY_SIZE(attr)));
}

/**
 * @tc.name: AssetAddTest.AssetAddTest006
 * @tc.desc: Add alias and secret, then add again, expect duplicate
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetAddTest, AssetAddTest006, TestSize.Level0)
{
    AssetBlob funcName = { .size = strlen(__func__), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    AssetAttr attr[] = {
        { .tag = ASSET_SYSTEM_TAG_ALIAS, .value.blob = funcName },
        { .tag = ASSET_SYSTEM_TAG_SECRET, .value.blob = funcName },
        { .tag = ASSET_SYSTEM_TAG_ACCESSIBILITY, .value.u32 = ASSET_SYSTEM_ACCESSIBILITY_DEVICE_POWERED_ON },
    };
    ASSERT_EQ(ASSET_SYSTEM_SUCCESS, AssetAdd(attr, ARRAY_SIZE(attr)));
    ASSERT_EQ(ASSET_SYSTEM_DUPLICATED, AssetAdd(attr, ARRAY_SIZE(attr)));

    ASSERT_EQ(ASSET_SYSTEM_SUCCESS, RemoveByAlias(__func__));
}

/**
 * @tc.name: AssetAddTest.AssetAddTest008
 * @tc.desc: Add without attr, expect ASSET_SYSTEM_INVALID_ARGUMENT
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetAddTest, AssetAddTest008, TestSize.Level0)
{
    ASSERT_EQ(ASSET_SYSTEM_INVALID_ARGUMENT, AssetAdd(nullptr, 0));
}

/**
 * @tc.name: AssetAddTest.AssetAddTest009
 * @tc.desc: Add without attr but count is wrong, expect ASSET_SYSTEM_INVALID_ARGUMENT
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetAddTest, AssetAddTest009, TestSize.Level0)
{
    ASSERT_EQ(ASSET_SYSTEM_INVALID_ARGUMENT, AssetAdd(nullptr, 1));
}

/**
 * @tc.name: AssetAddTest.AssetAddTest010
 * @tc.desc: Add with attr but count is wrong, expect ASSET_SYSTEM_INVALID_ARGUMENT
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetAddTest, AssetAddTest010, TestSize.Level0)
{
    AssetBlob funcName = { .size = strlen(__func__), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    AssetAttr attr[] = {
        { .tag = ASSET_SYSTEM_TAG_ALIAS, .value.blob = funcName },
        { .tag = ASSET_SYSTEM_TAG_SECRET, .value.blob = funcName }
    };
    ASSERT_EQ(ASSET_SYSTEM_INVALID_ARGUMENT, AssetAdd(attr, 0));
}
}