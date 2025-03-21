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

#include "asset_system_api_test.h"

#include <cstring>
#include <gtest/gtest.h>

#include "nativetoken_kit.h"
#include "token_setproc.h"

#include "asset_system_api.h"
#include "asset_system_type.h"
#include "asset_test_common.h"
#include "asset_mem.h"

using namespace testing::ext;
namespace UnitTest::AssetSystemApiWithoutPermissionTest {
class AssetSystemApiWithoutPermissionTest : public testing::Test {
public:
    static void SetUpTestCase(void);

    static void TearDownTestCase(void);

    void SetUp(void);

    void TearDown(void);
};

void AssetSystemApiWithoutPermissionTest::SetUpTestCase(void)
{
}

void AssetSystemApiWithoutPermissionTest::TearDownTestCase(void)
{
}

void AssetSystemApiWithoutPermissionTest::SetUp(void)
{
}

void AssetSystemApiWithoutPermissionTest::TearDown(void)
{
}

/**
 * @tc.name: AssetSystemApiWithoutPermissionTest.AssetSystemApiWithoutPermissionTest001
 * @tc.desc: Test asset func AssetParseAttr, expect nullptr
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetSystemApiWithoutPermissionTest, AssetSystemApiWithoutPermissionTest001, TestSize.Level0)
{
    uint32_t numAttrs = 2;

    // allocate memory for AssetResult
    AssetResult result;
    result.count = numAttrs;
    result.attrs = (AssetAttr *)AssetMalloc(numAttrs * sizeof(AssetAttr));

    if (result.attrs == nullptr) {
        return;
    }

    // initialize result
    result.attrs[0].tag = 1;
    result.attrs[0].value.u32 = 42;

    result.attrs[1].tag = 2;
    result.attrs[1].value.boolean = true;
    ASSERT_EQ(nullptr, AssetParseAttr(&result, SEC_ASSET_TAG_WRAP_TYPE));
    AssetFree(result.attrs);
}
}