/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "asset_query_sync_result_test.h"

#include <string>
#include <gtest/gtest.h>

#include "asset_api.h"
#include "asset_test_common.h"

using namespace testing::ext;
namespace UnitTest::AssetQuerySyncResultTest {
class AssetQuerySyncResultTest : public testing::Test {
public:
    static void SetUpTestCase(void);

    static void TearDownTestCase(void);

    void SetUp(void);

    void TearDown(void);
};

void AssetQuerySyncResultTest::SetUpTestCase(void)
{
}

void AssetQuerySyncResultTest::TearDownTestCase(void)
{
}

void AssetQuerySyncResultTest::SetUp(void)
{
}

void AssetQuerySyncResultTest::TearDown(void)
{
}

/**
 * @tc.name: AssetQuerySyncResultTest.AssetQuerySyncResultTest001
 * @tc.desc: query sync result, expect ASSET_SUCCESS.
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetQuerySyncResultTest, AssetQuerySyncResultTest001, TestSize.Level0)
{
    Asset_Attr attr[] = {};
    Asset_SyncResult result = { 0 };
    ASSERT_EQ(ASSET_SUCCESS, OH_Asset_QuerySyncResult(attr, ARRAY_SIZE(attr), &result));
}

/**
 * @tc.name: AssetQuerySyncResultTest.AssetQuerySyncResultTest002
 * @tc.desc: query sync result of attr, expect ASSET_SUCCESS.
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetQuerySyncResultTest, AssetQuerySyncResultTest002, TestSize.Level0)
{
    Asset_Attr attr[] = {
        { .tag = ASSET_TAG_REQUIRE_ATTR_ENCRYPTED, .value.boolean = true },
    };
    Asset_SyncResult result = { 0 };
    ASSERT_EQ(ASSET_SUCCESS, OH_Asset_QuerySyncResult(attr, ARRAY_SIZE(attr), &result));
}

/**
 * @tc.name: AssetQuerySyncResultTest.AssetQuerySyncResultTest003
 * @tc.desc: query sync result of group, expect ASSET_UNSUPPORTED.
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetQuerySyncResultTest, AssetQuerySyncResultTest003, TestSize.Level0)
{
    Asset_Blob funcName = { .size = strlen(__func__), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    Asset_Attr attr[] = {
        { .tag = ASSET_TAG_GROUP_ID, .value.blob = funcName },
    };
    Asset_SyncResult result = { 0 };
    ASSERT_EQ(ASSET_UNSUPPORTED, OH_Asset_QuerySyncResult(attr, ARRAY_SIZE(attr), &result));
}
}