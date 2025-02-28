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
 * @tc.desc: Test asset func AssetAdd specific user id, expect SUCCESS
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetSystemApiWithoutPermissionTest, AssetSystemApiWithoutPermissionTest001, TestSize.Level0)
{
    uint32_t numAttrs = 2;

    // 分配内存给 AssetResult
    AssetResult result;
    result.count = numAttrs;
    result.attrs = (AssetAttr *)AssetMalloc(numAttrs * sizeof(AssetAttr));

    // 检查内存分配是否成功
    if (result.attrs == nullptr) {
        return;
    }

    // 初始化第一个 AssetAttr
    result.attrs[0].tag = 1; // 假设标签为 1
    result.attrs[0].value.u32 = 42; // 假设值为 42，类型为 uint32_t

    // 初始化第二个 AssetAttr
    result.attrs[1].tag = 2; // 假设标签为 2
    result.attrs[1].value.boolean = true; // 假设值为 true，类型为 bool
    ASSERT_EQ(nullptr, AssetParseAttr(&result, SEC_ASSET_TAG_WRAP_TYPE));
    AssetFree(result.attrs);
}
}