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

#include <gtest/gtest.h>
#include <string.h>

#include "asset_api.h"
#include "asset_test_common.h"

using namespace testing::ext;
namespace Unittest::AssetUpdateTest {
class AssetUpdateTest : public testing::Test {
public:
    static void SetUpTestCase(void);

    static void TearDownTestCase(void);

    void SetUp();

    void TearDown();
};

void AssetUpdateTest::SetUpTestCase(void)
{
}

void AssetUpdateTest::TearDownTestCase(void)
{
}

void AssetUpdateTest::SetUp()
{
}

void AssetUpdateTest::TearDown()
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
    Asset_Blob funcName = { .size = strlen(__func__), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    Asset_Attr add_attr[] = {
        {
            .tag = ASSET_TAG_ALIAS,
            .value.blob = funcName
        }, {
            .tag = ASSET_TAG_SECRET,
            .value.blob = funcName
        }
    };
    ASSERT_EQ(ASSET_SUCCESS, OH_Asset_Add(add_attr, sizeof(add_attr) / sizeof(add_attr[0])));

    Asset_Attr query_attr[] = {
        {
            .tag = ASSET_TAG_ALIAS,
            .value.blob = funcName
        }
    };
    const char *secret_new = "secret_new";
    Asset_Attr update_attr[] = {
        {
            .tag = ASSET_TAG_SECRET,
            .value.blob = {
                .size = strlen(secret_new), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(secret_new))
            }
        }
    };
    ASSERT_EQ(ASSET_SUCCESS, OH_Asset_Update(query_attr, sizeof(query_attr) / sizeof(query_attr[0]),
        update_attr, sizeof(update_attr) / sizeof(update_attr[0])));

    ASSERT_EQ(ASSET_SUCCESS, RemoveByAlias(__func__));
}

/**
 * @tc.name: AssetUpdateTest.AssetUpdateTest002
 * @tc.desc: Update with empty update attr, expect ASSET_INVALID_ARGUMENT
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetUpdateTest, AssetUpdateTest002, TestSize.Level0)
{
    Asset_Blob funcName = { .size = strlen(__func__), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    Asset_Attr query_attr[] = {
        {
            .tag = ASSET_TAG_ALIAS,
            .value.blob = funcName
        }
    };
    ASSERT_EQ(ASSET_INVALID_ARGUMENT, OH_Asset_Update(query_attr, sizeof(query_attr) / sizeof(query_attr[0]),
        nullptr, 0));
}

/**
 * @tc.name: AssetUpdateTest.AssetUpdateTest003
 * @tc.desc: Update with empty query attr, expect ASSET_INVALID_ARGUMENT
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetUpdateTest, AssetUpdateTest003, TestSize.Level0)
{
    Asset_Blob funcName = { .size = strlen(__func__), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    Asset_Attr update_attr[] = {
        {
            .tag = ASSET_TAG_SECRET,
            .value.blob = funcName
        }
    };
    ASSERT_EQ(ASSET_INVALID_ARGUMENT, OH_Asset_Update(nullptr, 0,
        update_attr, sizeof(update_attr) / sizeof(update_attr[0])));
}

/**
 * @tc.name: AssetUpdateTest.AssetUpdateTest004
 * @tc.desc: Update non-exist asset, expect ASSET_NOT_FOUND
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetUpdateTest, AssetUpdateTest004, TestSize.Level0)
{
    Asset_Blob funcName = { .size = strlen(__func__), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    Asset_Attr query_attr[] = {
        {
            .tag = ASSET_TAG_ALIAS,
            .value.blob = funcName
        }
    };
    const char *secret_new = "secret_new";
    Asset_Attr update_attr[] = {
        {
            .tag = ASSET_TAG_SECRET,
            .value.blob = {
                .size = strlen(secret_new), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(secret_new))
            }
        }
    };
    ASSERT_EQ(ASSET_NOT_FOUND, OH_Asset_Update(query_attr, sizeof(query_attr) / sizeof(query_attr[0]),
        update_attr, sizeof(update_attr) / sizeof(update_attr[0])));
}
}