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

#include "native_asset_api.h"

#include <gtest/gtest.h>

using namespace testing::ext;
namespace Unittest::AttestCApiTest {
class AttestCApiTest : public testing::Test {
public:
    static void SetUpTestCase(void);

    static void TearDownTestCase(void);

    void SetUp();

    void TearDown();
};

void AttestCApiTest::SetUpTestCase(void)
{
}

void AttestCApiTest::TearDownTestCase(void)
{
}

void AttestCApiTest::SetUp()
{
}

void AttestCApiTest::TearDown()
{
}

/**
 * @tc.name: AttestCApiTest.AttestCApiTest001
 * @tc.desc: AttestCApiTest001
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(AttestCApiTest, AttestCApiTest001, TestSize.Level0)
{
    int32_t ret = OH_Asset_Test(1);
    ASSERT_EQ(1, ret) << "res is" << ret;
}
}