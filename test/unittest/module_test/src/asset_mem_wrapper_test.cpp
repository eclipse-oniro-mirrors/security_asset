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
 * @tc.desc: Test asset func AssetMalloc, expect nullptr
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetMemWrapperTest, AssetMemWrapperTest001, TestSize.Level0)
{
    ASSERT_EQ(nullptr, AssetMalloc(0));
}

/**
 * @tc.name: AssetMemWrapperTest.AssetMemWrapperTest002
 * @tc.desc: Test asset func AssetFree
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetMemWrapperTest, AssetMemWrapperTest002, TestSize.Level0)
{
    AssetFree(nullptr);
}

/**
 * @tc.name: AssetMemWrapperTest.AssetMemWrapperTest003
 * @tc.desc: Test asset func AssetMemCmp, expect 0
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AssetMemWrapperTest, AssetMemWrapperTest003, TestSize.Level0)
{
    const uint32_t size = 5;
    uint8_t array1[size] = {1, 2, 3, 4, 5};
    uint8_t array2[size] = {1, 2, 3, 4, 5};
    ASSERT_EQ(0, AssetMemCmp(array1, array2, size));
}
}
