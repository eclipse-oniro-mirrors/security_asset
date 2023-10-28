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

#include "asset_add_test.h"

#include <gtest/gtest.h>

#include "asset_api.h"
#include "asset_test_common.h"

using namespace testing::ext;
namespace Unittest::AttestAddTest {
class AttestAddTest : public testing::Test {
public:
    static void SetUpTestCase(void);

    static void TearDownTestCase(void);

    void SetUp();

    void TearDown();
};

void AttestAddTest::SetUpTestCase(void)
{
}

void AttestAddTest::TearDownTestCase(void)
{
}

void AttestAddTest::SetUp()
{
}

void AttestAddTest::TearDown()
{
}

/**
 * @tc.name: AttestAddTest.AttestAddTest001
 * @tc.desc: Add alias and secret, expect success
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AttestAddTest, AttestAddTest001, TestSize.Level0)
{
    Asset_Blob alias = { .size = strlen(__func__), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    Asset_Blob secret = { .size = strlen(__func__), .data = reinterpret_cast<uint8_t*>(const_cast<char*>(__func__)) };
    Asset_Attr attr[] = {
        {
            .tag = ASSET_TAG_ALIAS,
            .value.blob = alias
        },
        {
            .tag = ASSET_TAG_SECRET,
            .value.blob = secret
        },
    };
    ASSERT_EQ(ASSET_SUCCESS, OH_Asset_Add(attr, sizeof(attr) / sizeof(attr[0])));
    ASSERT_EQ(ASSET_SUCCESS, RemoveByAlias(__func__));
}
}