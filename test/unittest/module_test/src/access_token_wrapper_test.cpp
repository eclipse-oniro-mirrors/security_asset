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

#include "access_token_wrapper_test.h"

#include <cstring>
#include <gtest/gtest.h>

#include "asset_system_type.h"
#include "access_token_wrapper.h"

using namespace testing::ext;
namespace UnitTest::AssetPermissionCheckWrapperTest {
class AssetPermissionCheckWrapperTest : public testing::Test {
public:
    static void SetUpTestCase(void);

    static void TearDownTestCase(void);

    void SetUp(void);

    void TearDown(void);
};

void AssetPermissionCheckWrapperTest::SetUpTestCase(void)
{
}

void AssetPermissionCheckWrapperTest::TearDownTestCase(void)
{
}

void AssetPermissionCheckWrapperTest::SetUp(void)
{
}

void AssetPermissionCheckWrapperTest::TearDown(void)
{
}

/**
 * @tc.name: AssetPermissionCheckWrapperTest.AssetPermissionCheckWrapperTest001
 * @tc.desc: Test asset func CheckPersistentPermission, expect ERROR
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetPermissionCheckWrapperTest, AssetPermissionCheckWrapperTest001, TestSize.Level0)
{
    const char *permission = "ohos.permission.STORE_PERSISTENT_DATA"
    ASSERT_EQ(false, CheckPermission(permission));
}

/**
 * @tc.name: AssetPermissionCheckWrapperTest.AssetPermissionCheckWrapperTest002
 * @tc.desc: Test asset func CheckSystemHapPermission, expect SUCCESS
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetPermissionCheckWrapperTest, AssetPermissionCheckWrapperTest002, TestSize.Level0)
{
    ASSERT_EQ(true, CheckSystemHapPermission());
}
}
