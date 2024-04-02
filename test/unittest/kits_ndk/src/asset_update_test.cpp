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

#include "asset_update_test.h"

#include <string>
#include <gtest/gtest.h>

#include "asset_system_api.h"
#include "asset_test_common.h"

using namespace testing::ext;
namespace UnitTest::AssetUpdateTest {
class AssetUpdateTest : public testing::Test {
public:
    static void SetUpTestCase(void);

    static void TearDownTestCase(void);

    void SetUp(void);

    void TearDown(void);
};

void AssetUpdateTest::SetUpTestCase(void)
{
}

void AssetUpdateTest::TearDownTestCase(void)
{
}

void AssetUpdateTest::SetUp(void)
{
}

void AssetUpdateTest::TearDown(void)
{
}

/**
 * @tc.name: AssetUpdateTest.AssetUpdateTest001
 * @tc.desc: Add asset, then update with new secret, expect success
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetUpdateTest, AssetUpdateTest001, TestSize.Level0)
{
    AssetBlob funcName = { .size = strlen(__func__), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    AssetAttr addAttr[] = {
        { .tag = ASSET_SYSTEM_TAG_ALIAS, .value.blob = funcName },
        { .tag = ASSET_SYSTEM_TAG_SECRET, .value.blob = funcName },
        { .tag = ASSET_SYSTEM_TAG_ACCESSIBILITY, .value.u32 = ASSET_SYSTEM_ACCESSIBILITY_DEVICE_POWERED_ON },
    };
    ASSERT_EQ(ASSET_SYSTEM_SUCCESS, AssetAdd(addAttr, ARRAY_SIZE(addAttr)));

    AssetAttr queryAttr[] = {
        { .tag = ASSET_SYSTEM_TAG_ALIAS, .value.blob = funcName }
    };
    const char *secretNew = "secret_new";
    AssetAttr updateAttr[] = {
        {
            .tag = ASSET_SYSTEM_TAG_SECRET,
            .value.blob = {
                .size = strlen(secretNew), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(secretNew))
            }
        }
    };
    ASSERT_EQ(ASSET_SYSTEM_SUCCESS, AssetUpdate(queryAttr, ARRAY_SIZE(queryAttr), updateAttr, ARRAY_SIZE(updateAttr)));

    ASSERT_EQ(ASSET_SYSTEM_SUCCESS, RemoveByAlias(__func__));
}

/**
 * @tc.name: AssetUpdateTest.AssetUpdateTest002
 * @tc.desc: Update with empty update attr, expect ASSET_SYSTEM_INVALID_ARGUMENT
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetUpdateTest, AssetUpdateTest002, TestSize.Level0)
{
    AssetBlob funcName = { .size = strlen(__func__), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    AssetAttr queryAttr[] = {
        { .tag = ASSET_SYSTEM_TAG_ALIAS, .value.blob = funcName }
    };
    ASSERT_EQ(ASSET_SYSTEM_INVALID_ARGUMENT, AssetUpdate(queryAttr, ARRAY_SIZE(queryAttr), nullptr, 0));
}

/**
 * @tc.name: AssetUpdateTest.AssetUpdateTest003
 * @tc.desc: Update with empty query attr, expect ASSET_SYSTEM_INVALID_ARGUMENT
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetUpdateTest, AssetUpdateTest003, TestSize.Level0)
{
    AssetBlob funcName = { .size = strlen(__func__), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    AssetAttr updateAttr[] = {
        { .tag = ASSET_SYSTEM_TAG_SECRET, .value.blob = funcName }
    };
    ASSERT_EQ(ASSET_SYSTEM_INVALID_ARGUMENT, AssetUpdate(nullptr, 0, updateAttr, ARRAY_SIZE(updateAttr)));
}

/**
 * @tc.name: AssetUpdateTest.AssetUpdateTest004
 * @tc.desc: Update non-exist asset, expect ASSET_SYSTEM_NOT_FOUND
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetUpdateTest, AssetUpdateTest004, TestSize.Level0)
{
    AssetBlob funcName = { .size = strlen(__func__), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    AssetAttr queryAttr[] = {
        { .tag = ASSET_SYSTEM_TAG_ALIAS, .value.blob = funcName }
    };
    const char *secretNew = "secret_new";
    AssetAttr updateAttr[] = {
        {
            .tag = ASSET_SYSTEM_TAG_SECRET,
            .value.blob = {
                .size = strlen(secretNew), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(secretNew))
            }
        }
    };
    ASSERT_EQ(ASSET_SYSTEM_NOT_FOUND, AssetUpdate(queryAttr, ARRAY_SIZE(queryAttr), updateAttr, ARRAY_SIZE(updateAttr)));
}
}