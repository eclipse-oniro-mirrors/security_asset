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

#include "permission_check_wrapper_test.h"

#include <cstring>
#include <gtest/gtest.h>

#include "sec_asset_type.h"
#include "permission_check_wrapper.h"

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
    ASSERT_EQ(false, CheckPersistentPermission());
}

/**
 * @tc.name: AssetPermissionCheckWrapperTest.AssetPermissionCheckWrapperTest002
 * @tc.desc: Test asset func CheckInteractPermission, expect ERROR
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetPermissionCheckWrapperTest, AssetPermissionCheckWrapperTest002, TestSize.Level0)
{
    ASSERT_EQ(false, CheckInteractPermission());
}

/**
 * @tc.name: AssetPermissionCheckWrapperTest.AssetPermissionCheckWrapperTest003
 * @tc.desc: Test asset func CheckSystemHapPermission, expect SUCCESS
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetPermissionCheckWrapperTest, AssetPermissionCheckWrapperTest003, TestSize.Level0)
{
    ASSERT_EQ(true, CheckSystemHapPermission());
}
}
