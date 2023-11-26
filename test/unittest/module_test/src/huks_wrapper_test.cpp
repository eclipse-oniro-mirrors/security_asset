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

#include "os_account_wrapper_test.h"

#include <cstring>
#include <gtest/gtest.h>

#include "huks_wrapper.h"

using namespace testing::ext;
namespace UnitTest::HuksWrapperTest {
class HuksWrapperTest : public testing::Test {
public:
    static void SetUpTestCase(void);

    static void TearDownTestCase(void);

    void SetUp(void);

    void TearDown(void);
};

void HuksWrapperTest::SetUpTestCase(void)
{
}

void HuksWrapperTest::TearDownTestCase(void)
{
}

void HuksWrapperTest::SetUp(void)
{
}

void HuksWrapperTest::TearDown(void)
{
}

/**
 * @tc.name: HuksWrapperTest.HuksWrapperTest001
 * @tc.desc: Test huks wrapper func, for secrect key generate/exists/delete
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(HuksWrapperTest, HuksWrapperTest001, TestSize.Level0)
{
    char tmpKeyAlias[] = "AESCipherKeyAlias001";
    struct HksBlob keyAlias = { (uint32_t)strlen(tmpKeyAlias), (uint8_t *)tmpKeyAlias };
    struct KeyId keyId = { 0, keyAlias };
    ASSERT_EQ(HKS_SUCCESS, GenerateKey(&keyId, true, false));
    ASSERT_EQ(HKS_SUCCESS, IsKeyExist(&keyAlias));
    ASSERT_EQ(HKS_SUCCESS, DeleteKey(&keyAlias));
}

/**
 * @tc.name: HuksWrapperTest.HuksWrapperTest002
 * @tc.desc: Test huks wrapper func, for secrect encrypt&decrypt
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(HuksWrapperTest, HuksWrapperTest002, TestSize.Level0)
{
    char tmpKeyAlias[] = "AESCipherKeyAlias002";
    struct HksBlob keyAlias = { (uint32_t)strlen(tmpKeyAlias), (uint8_t *)tmpKeyAlias };
    struct KeyId keyId = { 0, keyAlias };
    ASSERT_EQ(HKS_SUCCESS, GenerateKey(&keyId, false, false));

    uint8_t msg[6] = { 1, 2, 3, 4, 5, 6 };
    struct HksBlob inData = { 6, msg };
    uint8_t plain[6 + TAG_SIZE + NONCE_SIZE] = { 0 };
    struct HksBlob outData = { 6 + TAG_SIZE + NONCE_SIZE, plain };
    uint8_t aad[8] = { 0 };
    struct HksBlob aadData = { 8, aad };

    ASSERT_EQ(HKS_SUCCESS, EncryptData(&keyId, &aadData, &inData, &outData));
    ASSERT_EQ(HKS_SUCCESS, DecryptData(&keyId, &aadData, &outData, &inData));
    ASSERT_EQ(HKS_SUCCESS, DeleteKey(&keyAlias));
}

/**
 * @tc.name: HuksWrapperTest.HuksWrapperTest003
 * @tc.desc: Test huks wrapper func, for secrect InitKey&ExecCrypt&Drop
 * @tc.type: FUNC
 * @tc.result:0
 */
HWTEST_F(HuksWrapperTest, HuksWrapperTest003, TestSize.Level0)
{
    char tmpKeyAlias[] = "AESCipherKeyAlias003";
    struct HksBlob keyAlias = { (uint32_t)strlen(tmpKeyAlias), (uint8_t *)tmpKeyAlias };
    struct KeyId keyId = { 0, keyAlias };
    ASSERT_EQ(HKS_SUCCESS, GenerateKey(&keyId, true, false));

    uint8_t msg[6] = { 1, 2, 3, 4, 5, 6 };
    struct HksBlob inData = { 6, msg };
    uint8_t plain[6 + TAG_SIZE + NONCE_SIZE] = { 0 };
    struct HksBlob outData = { 6 + TAG_SIZE + NONCE_SIZE, plain };
    uint8_t aad[8] = { 0 };
    struct HksBlob aadData = { 8, aad };
    ASSERT_EQ(HKS_SUCCESS, EncryptData(&keyId, &aadData, &inData, &outData));

    uint8_t challenge[32] = { 0 };
    struct HksBlob challengeData = { 32, challenge };
    uint8_t handle[8] = { 0 };
    struct HksBlob handleData = { 8, handle };
    ASSERT_EQ(HKS_SUCCESS, InitKey(&keyId, 600, &challengeData, &handleData));

    uint8_t authtoken[148] = { 0 };
    struct HksBlob authtokenData = { 148, authtoken };
    /* auth token is not ok, result in update&finish fail */
    (void)ExecCrypt(&handleData, &aadData, &authtokenData, &outData, &inData);
    (void)Drop(&handleData);
    ASSERT_EQ(HKS_SUCCESS, DeleteKey(&keyAlias));
}
}