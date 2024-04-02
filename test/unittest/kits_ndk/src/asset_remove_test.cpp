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

#include "asset_remove_test.h"

#include <gtest/gtest.h>

#include "asset_system_api.h"
#include "asset_test_common.h"

using namespace testing::ext;
namespace UnitTest::AssetRemoveTest {
class AssetRemoveTest : public testing::Test {
public:
    static void SetUpTestCase(void);

    static void TearDownTestCase(void);

    void SetUp(void);

    void TearDown(void);
};

void AssetRemoveTest::SetUpTestCase(void)
{
}

void AssetRemoveTest::TearDownTestCase(void)
{
}

void AssetRemoveTest::SetUp(void)
{
}

void AssetRemoveTest::TearDown(void)
{
}

/**
 * @tc.name: AssetRemoveTest.AssetRemoveTest001
 * @tc.desc: Add alias and secret, then remove it, expect success
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetRemoveTest, AssetRemoveTest001, TestSize.Level0)
{
    AssetBlob funcName = { .size = strlen(__func__), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    AssetAttr attr[] = {
        { .tag = ASSET_SYSTEM_TAG_ALIAS, .value.blob = funcName },
        { .tag = ASSET_SYSTEM_TAG_SECRET, .value.blob = funcName },
        { .tag = ASSET_SYSTEM_TAG_ACCESSIBILITY, .value.u32 = ASSET_SYSTEM_ACCESSIBILITY_DEVICE_POWERED_ON },
        { .tag = ASSET_SYSTEM_TAG_REQUIRE_PASSWORD_SET, .value.boolean = false },
        { .tag = ASSET_SYSTEM_TAG_AUTH_TYPE, .value.u32 = ASSET_SYSTEM_AUTH_TYPE_NONE },
        { .tag = ASSET_SYSTEM_TAG_SYNC_TYPE, .value.u32 = ASSET_SYSTEM_SYNC_TYPE_NEVER },
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
    AssetAttr attr2[] = {
        { .tag = ASSET_SYSTEM_TAG_ALIAS, .value.blob = funcName },
        { .tag = ASSET_SYSTEM_TAG_ACCESSIBILITY, .value.u32 = ASSET_SYSTEM_ACCESSIBILITY_DEVICE_POWERED_ON },
        { .tag = ASSET_SYSTEM_TAG_REQUIRE_PASSWORD_SET, .value.boolean = false },
        { .tag = ASSET_SYSTEM_TAG_AUTH_TYPE, .value.u32 = ASSET_SYSTEM_AUTH_TYPE_NONE },
        { .tag = ASSET_SYSTEM_TAG_SYNC_TYPE, .value.u32 = ASSET_SYSTEM_SYNC_TYPE_NEVER },
        { .tag = ASSET_SYSTEM_TAG_IS_PERSISTENT, .value.boolean = false },
        { .tag = ASSET_SYSTEM_TAG_DATA_LABEL_NORMAL_1, .value.blob = funcName },
        { .tag = ASSET_SYSTEM_TAG_DATA_LABEL_NORMAL_2, .value.blob = funcName },
        { .tag = ASSET_SYSTEM_TAG_DATA_LABEL_NORMAL_3, .value.blob = funcName },
        { .tag = ASSET_SYSTEM_TAG_DATA_LABEL_NORMAL_4, .value.blob = funcName },
        { .tag = ASSET_SYSTEM_TAG_DATA_LABEL_CRITICAL_1, .value.blob = funcName },
        { .tag = ASSET_SYSTEM_TAG_DATA_LABEL_CRITICAL_2, .value.blob = funcName },
        { .tag = ASSET_SYSTEM_TAG_DATA_LABEL_CRITICAL_3, .value.blob = funcName },
        { .tag = ASSET_SYSTEM_TAG_DATA_LABEL_CRITICAL_4, .value.blob = funcName }
    };
    ASSERT_EQ(ASSET_SYSTEM_SUCCESS, AssetRemove(attr2, ARRAY_SIZE(attr2)));
}

/**
 * @tc.name: AssetRemoveTest.AssetRemoveTest002
 * @tc.desc: remove wrong blob type, expect fail
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetRemoveTest, AssetRemoveTest002, TestSize.Level0)
{
    int arr[] = {
        ASSET_SYSTEM_TAG_ALIAS,
        ASSET_SYSTEM_TAG_DATA_LABEL_NORMAL_1, ASSET_SYSTEM_TAG_DATA_LABEL_NORMAL_2,
        ASSET_SYSTEM_TAG_DATA_LABEL_NORMAL_3, ASSET_SYSTEM_TAG_DATA_LABEL_NORMAL_4,
        ASSET_SYSTEM_TAG_DATA_LABEL_CRITICAL_1, ASSET_SYSTEM_TAG_DATA_LABEL_CRITICAL_2,
        ASSET_SYSTEM_TAG_DATA_LABEL_CRITICAL_3, ASSET_SYSTEM_TAG_DATA_LABEL_CRITICAL_4
    };
    for (int i = 0; i < ARRAY_SIZE(arr); i++) {
        AssetBlob tmpBlob = { .size = strlen(__func__), .data = nullptr };
        AssetAttr attr[] = {
            { .tag = arr[i], .value.blob = tmpBlob }
        };
        ASSERT_EQ(ASSET_SYSTEM_INVALID_ARGUMENT, AssetRemove(attr, ARRAY_SIZE(attr)));

        attr[0].value.u32 = 0;
        ASSERT_EQ(ASSET_SYSTEM_INVALID_ARGUMENT, AssetRemove(attr, ARRAY_SIZE(attr)));

        attr[0].value.boolean = true;
        ASSERT_EQ(ASSET_SYSTEM_INVALID_ARGUMENT, AssetRemove(attr, ARRAY_SIZE(attr)));
    }
}

/**
 * @tc.name: AssetRemoveTest.AssetRemoveTest003
 * @tc.desc: remove wrong u32 type, expect fail
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetRemoveTest, AssetRemoveTest003, TestSize.Level0)
{
    int arr[] = { ASSET_SYSTEM_TAG_ACCESSIBILITY, ASSET_SYSTEM_TAG_AUTH_TYPE, ASSET_SYSTEM_TAG_SYNC_TYPE };
    for (int i = 0; i < ARRAY_SIZE(arr); i++) {
        AssetBlob tmpBlob = { .size = strlen(__func__), .data = nullptr };
        AssetAttr attr[] = {
            { .tag = arr[i], .value.blob = tmpBlob }
        };
        ASSERT_EQ(ASSET_SYSTEM_INVALID_ARGUMENT, AssetRemove(attr, ARRAY_SIZE(attr)));

        tmpBlob = { .size = strlen(__func__), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
        attr[0].value.blob = tmpBlob;
        ASSERT_EQ(ASSET_SYSTEM_INVALID_ARGUMENT, AssetRemove(attr, ARRAY_SIZE(attr)));
    }
}
}