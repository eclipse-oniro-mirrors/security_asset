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

#include "asset_mem_wrapper_test.h"

#include <cstring>
#include <gtest/gtest.h>

#include "asset_mem.h"

using namespace testing::ext;
namespace UnitTest::AssetMemWrapperTest {
class AssetMemWrapperTest : public testing::Test {
public:
    static void SetUpTestCase(void);

    static void TearDownTestCase(void);

    void SetUp(void);

    void TearDown(void);
};

void AssetMemWrapperTest::SetUpTestCase(void)
{
}

void AssetMemWrapperTest::TearDownTestCase(void)
{
}

void AssetMemWrapperTest::SetUp(void)
{
}

void AssetMemWrapperTest::TearDown(void)
{
}

/**
 * @tc.name: AssetMemWrapperTest.AssetMemWrapperTest001
 * @tc.desc: Test asset func AssetFree, AssetMalloc, expect nullptr
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetMemWrapperTest, AssetMemWrapperTest001, TestSize.Level0)
{
    AssetFree(nullptr);
    ASSERT_EQ(nullptr, AssetMalloc(0));
}
}
