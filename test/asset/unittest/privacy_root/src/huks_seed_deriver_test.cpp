/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "huks_seed_deriver.h"

#include "hks_api.h"
#include "hks_param.h"
#include "privacy_error_code.h"
#include <algorithm>
#include <gtest/gtest.h>
#include <limits>
#include <string>
#include <vector>

namespace {
using namespace testing::ext;
using namespace OHOS::Security::PrivacyComputation;
using namespace OHOS::Security::PrivacyComputation::internal;
struct HuksCalls {
    int32_t existResult = HKS_SUCCESS;
    int32_t generateResult = HKS_SUCCESS;
    int32_t initResult = HKS_SUCCESS;
    int32_t updateResult = HKS_SUCCESS;
    int32_t finishResult = HKS_SUCCESS;
    uint32_t outputSize = kHeSeedSize;
    size_t exists = 0;
    size_t generates = 0;
    size_t inits = 0;
    size_t updates = 0;
    size_t finishes = 0;
    size_t aborts = 0;
    std::string alias;
    uint32_t storage = 0;
    int32_t userId = -1;
    uint32_t algorithm = 0;
    uint32_t purpose = 0;
    uint32_t keyBits = 0;
    uint32_t digest = 0;
    uint32_t deriveBytes = 0;
    uint32_t exportFlag = 0;
    std::vector<uint8_t> salt;
    std::vector<std::string> stages;
    std::vector<int32_t> accounts;
};
HuksCalls g_calls;
uint32_t ReadUint(const HksParamSet *params, uint32_t tag)
{
    HksParam *param = nullptr;
    EXPECT_EQ(HksGetParam(params, tag, &param), HKS_SUCCESS);
    return param == nullptr ? 0 : param->uint32Param;
}
void ReadAccount(const HksParamSet *params)
{
    g_calls.storage = ReadUint(params, HKS_TAG_AUTH_STORAGE_LEVEL);
    HksParam *user = nullptr;
    const auto ret = HksGetParam(params, HKS_TAG_SPECIFIC_USER_ID, &user);
    EXPECT_EQ(ret, HKS_SUCCESS);
    g_calls.userId = ret == HKS_SUCCESS && user != nullptr ? user->int32Param : -1;
    g_calls.accounts.push_back(g_calls.userId);
}
void ReadAlias(const HksBlob *alias)
{
    g_calls.alias.assign(reinterpret_cast<const char *>(alias->data), alias->size);
}
std::array<uint8_t, kIdentityHashSize> Salt()
{
    std::array<uint8_t, kIdentityHashSize> salt{};
    salt.fill(0x45);
    return salt;
}
class HuksSeedDeriverTest : public testing::Test {
public:
    void SetUp() override
    {
        g_calls = HuksCalls{};
    }
};
} // namespace

// Replace only service-facing calls. Parameter construction/parsing uses the real HUKS SDK.
extern "C" int32_t HksKeyExist(const HksBlob *alias, const HksParamSet *params)
{
    ++g_calls.exists;
    ReadAlias(alias);
    ReadAccount(params);
    return g_calls.existResult;
}
extern "C" int32_t HksGenerateKey(const HksBlob *alias, const HksParamSet *params, HksParamSet *output)
{
    ++g_calls.generates;
    ReadAlias(alias);
    ReadAccount(params);
    EXPECT_EQ(output, nullptr);
    g_calls.algorithm = ReadUint(params, HKS_TAG_ALGORITHM);
    g_calls.purpose = ReadUint(params, HKS_TAG_PURPOSE);
    g_calls.keyBits = ReadUint(params, HKS_TAG_KEY_SIZE);
    g_calls.digest = ReadUint(params, HKS_TAG_DIGEST);
    return g_calls.generateResult;
}
extern "C" int32_t HksInit(const HksBlob *alias, const HksParamSet *params, HksBlob *handle, HksBlob *)
{
    ++g_calls.inits;
    g_calls.stages.push_back("Init");
    ReadAlias(alias);
    ReadAccount(params);
    g_calls.algorithm = ReadUint(params, HKS_TAG_ALGORITHM);
    g_calls.purpose = ReadUint(params, HKS_TAG_PURPOSE);
    g_calls.digest = ReadUint(params, HKS_TAG_DIGEST);
    g_calls.deriveBytes = ReadUint(params, HKS_TAG_DERIVE_KEY_SIZE);
    g_calls.exportFlag = ReadUint(params, HKS_TAG_DERIVE_AGREE_KEY_STORAGE_FLAG);
    HksParam *salt = nullptr;
    EXPECT_EQ(HksGetParam(params, HKS_TAG_SALT, &salt), HKS_SUCCESS);
    if (salt != nullptr) {
        g_calls.salt.assign(salt->blob.data, salt->blob.data + salt->blob.size);
    }
    std::fill_n(handle->data, handle->size, 1);
    return g_calls.initResult;
}
extern "C" int32_t HksUpdate(const HksBlob *, const HksParamSet *params, const HksBlob *, HksBlob *output)
{
    ++g_calls.updates;
    g_calls.stages.push_back("Update");
    ReadAccount(params);
    EXPECT_EQ(g_calls.storage, HKS_AUTH_STORAGE_LEVEL_CE);
    output->size = 0;
    return g_calls.updateResult;
}
extern "C" int32_t HksFinish(const HksBlob *, const HksParamSet *params, const HksBlob *, HksBlob *output)
{
    ++g_calls.finishes;
    g_calls.stages.push_back("Finish");
    ReadAccount(params);
    EXPECT_EQ(ReadUint(params, HKS_TAG_DERIVE_AGREE_KEY_STORAGE_FLAG), HKS_STORAGE_ALLOW_KEY_EXPORTED);
    // A failed HUKS call may have touched its output. The caller must not commit it to seed.
    std::fill_n(output->data, output->size, 0xAB);
    output->size = g_calls.outputSize;
    return g_calls.finishResult;
}
extern "C" int32_t HksAbort(const HksBlob *, const HksParamSet *params)
{
    ++g_calls.aborts;
    g_calls.stages.push_back("Abort");
    ReadAccount(params);
    return HKS_SUCCESS;
}

namespace {
/**
 * @tc.name: PrepareRootKey_001
 * @tc.desc: Query the fixed CE alias in the explicit OS account without regenerating an existing key.
 * @tc.type: FUNC
 */
HWTEST_F(HuksSeedDeriverTest, PrepareRootKey_001, TestSize.Level1)
{
    EXPECT_EQ(HuksSeedDeriver().PrepareRootKey(100, RootKeyPolicy::CREATE_IF_MISSING), PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_EQ(g_calls.storage, HKS_AUTH_STORAGE_LEVEL_CE);
    EXPECT_EQ(g_calls.userId, 100);
    EXPECT_EQ(g_calls.alias, "asset.privacy.derive.root.v1");
    EXPECT_EQ(g_calls.generates, 0U);
}
/**
 * @tc.name: PrepareRootKey_002
 * @tc.desc: Generate a missing 256-bit CE AES root with derivation purpose and SHA256.
 * @tc.type: FUNC
 */
HWTEST_F(HuksSeedDeriverTest, PrepareRootKey_002, TestSize.Level1)
{
    g_calls.existResult = HKS_ERROR_NOT_EXIST;
    ASSERT_EQ(HuksSeedDeriver().PrepareRootKey(100, RootKeyPolicy::CREATE_IF_MISSING), PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_EQ(g_calls.generates, 1U);
    EXPECT_EQ(g_calls.accounts, (std::vector<int32_t>{100, 100}));
    EXPECT_EQ(g_calls.algorithm, HKS_ALG_AES);
    EXPECT_EQ(g_calls.purpose, HKS_KEY_PURPOSE_DERIVE);
    EXPECT_EQ(g_calls.keyBits, 256U);
    EXPECT_EQ(g_calls.digest, HKS_DIGEST_SHA256);
    EXPECT_EQ(g_calls.storage, HKS_AUTH_STORAGE_LEVEL_CE);
    EXPECT_EQ(g_calls.userId, 100);
    EXPECT_EQ(g_calls.alias, "asset.privacy.derive.root.v1");
}
/**
 * @tc.name: PrepareRootKey_003
 * @tc.desc: Do not create a root for decryption or after a CE/permission query error.
 * @tc.type: FUNC
 */
HWTEST_F(HuksSeedDeriverTest, PrepareRootKey_003, TestSize.Level2)
{
    g_calls.existResult = HKS_ERROR_NOT_EXIST;
    EXPECT_EQ(HuksSeedDeriver().PrepareRootKey(100, RootKeyPolicy::REQUIRE_EXISTING), PRIVACY_PROTOCOL_CRYPTO_ERROR);
    g_calls.existResult = HKS_ERROR_NO_PERMISSION;
    EXPECT_EQ(HuksSeedDeriver().PrepareRootKey(100, RootKeyPolicy::CREATE_IF_MISSING), PRIVACY_PROTOCOL_CRYPTO_ERROR);
    EXPECT_EQ(g_calls.generates, 0U);
}
/**
 * @tc.name: PrepareRootKey_004
 * @tc.desc: Propagate root generation failure without trying a local fallback.
 * @tc.type: FUNC
 */
HWTEST_F(HuksSeedDeriverTest, PrepareRootKey_004, TestSize.Level2)
{
    g_calls.existResult = HKS_ERROR_NOT_EXIST;
    g_calls.generateResult = HKS_ERROR_CRYPTO_ENGINE_ERROR;
    EXPECT_EQ(HuksSeedDeriver().PrepareRootKey(100, RootKeyPolicy::CREATE_IF_MISSING), PRIVACY_PROTOCOL_CRYPTO_ERROR);
    EXPECT_EQ(g_calls.generates, 1U);
    EXPECT_EQ(g_calls.inits, 0U);
}
/**
 * @tc.name: Derive_001
 * @tc.desc: Export only a temporary 32-byte HKDF seed using the identity salt and CE root alias.
 * @tc.type: FUNC
 */
HWTEST_F(HuksSeedDeriverTest, Derive_001, TestSize.Level1)
{
    HeSeed seed;
    const auto salt = Salt();
    ASSERT_EQ(HuksSeedDeriver().Derive({20010001, 100}, salt, seed), PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_EQ(g_calls.algorithm, HKS_ALG_HKDF);
    EXPECT_EQ(g_calls.purpose, HKS_KEY_PURPOSE_DERIVE);
    EXPECT_EQ(g_calls.digest, HKS_DIGEST_SHA256);
    EXPECT_EQ(g_calls.deriveBytes, 32U);
    EXPECT_EQ(g_calls.exportFlag, HKS_STORAGE_ALLOW_KEY_EXPORTED);
    EXPECT_EQ(g_calls.userId, 100);
    EXPECT_EQ(g_calls.storage, HKS_AUTH_STORAGE_LEVEL_CE);
    EXPECT_EQ(g_calls.salt, std::vector<uint8_t>(salt.begin(), salt.end()));
    EXPECT_TRUE(std::all_of(seed.bytes.begin(), seed.bytes.end(), [](uint8_t value) { return value == 0xAB; }));
    EXPECT_EQ(g_calls.updates, 1U);
    EXPECT_EQ(g_calls.finishes, 1U);
    EXPECT_EQ(g_calls.aborts, 0U);
    EXPECT_EQ(g_calls.stages, (std::vector<std::string>{"Init", "Update", "Finish"}));
    EXPECT_EQ(g_calls.accounts, (std::vector<int32_t>{100, 100, 100}));
}
/**
 * @tc.name: Derive_002
 * @tc.desc: Preserve caller output and avoid update/finish when HUKS initialization fails.
 * @tc.type: FUNC
 */
HWTEST_F(HuksSeedDeriverTest, Derive_002, TestSize.Level2)
{
    g_calls.initResult = HKS_ERROR_NOT_EXIST;
    HeSeed seed;
    seed.bytes.fill(0x99);
    EXPECT_EQ(HuksSeedDeriver().Derive({20010001, 100}, Salt(), seed), PRIVACY_PROTOCOL_CRYPTO_ERROR);
    EXPECT_EQ(seed.bytes.front(), 0x99);
    EXPECT_EQ(g_calls.updates, 0U);
    EXPECT_EQ(g_calls.aborts, 0U);
    EXPECT_EQ(g_calls.generates, 0U);
    EXPECT_EQ(g_calls.stages, (std::vector<std::string>{"Init"}));
}
/**
 * @tc.name: Derive_003
 * @tc.desc: Abort the HUKS session on update failure without committing any seed.
 * @tc.type: FUNC
 */
HWTEST_F(HuksSeedDeriverTest, Derive_003, TestSize.Level2)
{
    g_calls.updateResult = HKS_ERROR_CRYPTO_ENGINE_ERROR;
    HeSeed seed;
    seed.bytes.fill(0x99);
    EXPECT_EQ(HuksSeedDeriver().Derive({20010001, 100}, Salt(), seed), PRIVACY_PROTOCOL_CRYPTO_ERROR);
    EXPECT_EQ(seed.bytes.front(), 0x99);
    EXPECT_EQ(g_calls.finishes, 0U);
    EXPECT_EQ(g_calls.aborts, 1U);
    EXPECT_EQ(g_calls.stages, (std::vector<std::string>{"Init", "Update", "Abort"}));
}
/**
 * @tc.name: Derive_004
 * @tc.desc: Abort on finish failure and reject a successful response with invalid seed length.
 * @tc.type: FUNC
 */
HWTEST_F(HuksSeedDeriverTest, Derive_004, TestSize.Level2)
{
    g_calls.finishResult = HKS_ERROR_CRYPTO_ENGINE_ERROR;
    HeSeed seed;
    seed.bytes.fill(0x99);
    EXPECT_EQ(HuksSeedDeriver().Derive({20010001, 100}, Salt(), seed), PRIVACY_PROTOCOL_CRYPTO_ERROR);
    EXPECT_EQ(seed.bytes.front(), 0x99);
    EXPECT_EQ(g_calls.aborts, 1U);
    EXPECT_EQ(g_calls.stages, (std::vector<std::string>{"Init", "Update", "Finish", "Abort"}));
    g_calls.finishResult = HKS_SUCCESS;
    g_calls.outputSize = 31;
    EXPECT_EQ(HuksSeedDeriver().Derive({20010001, 100}, Salt(), seed), PRIVACY_PROTOCOL_CRYPTO_ERROR);
    EXPECT_EQ(seed.bytes.front(), 0x99);
    EXPECT_EQ(g_calls.finishes, 2U);
    EXPECT_EQ(g_calls.aborts, 1U);
}
/**
 * @tc.name: Derive_005
 * @tc.desc: Use the same root alias in the corresponding CE account and retain each caller supplied salt.
 * @tc.type: FUNC
 */
HWTEST_F(HuksSeedDeriverTest, Derive_005, TestSize.Level2)
{
    const HuksSeedDeriver deriver;
    HeSeed seed;
    auto salt = Salt();
    ASSERT_EQ(deriver.Derive({20010001, 100}, salt, seed), PRIVACY_PROTOCOL_SUCCESS);
    const auto alias = g_calls.alias;
    EXPECT_EQ(alias, "asset.privacy.derive.root.v1");
    EXPECT_EQ(g_calls.userId, 100);
    salt.front() ^= 1;
    ASSERT_EQ(deriver.Derive({40010001, 200}, salt, seed), PRIVACY_PROTOCOL_SUCCESS);
    EXPECT_EQ(g_calls.alias, alias);
    EXPECT_EQ(g_calls.userId, 200);
    EXPECT_EQ(g_calls.salt, std::vector<uint8_t>(salt.begin(), salt.end()));
    EXPECT_EQ(g_calls.inits, 2U);
    EXPECT_EQ(g_calls.accounts, (std::vector<int32_t>{100, 100, 100, 200, 200, 200}));
    EXPECT_EQ(g_calls.generates, 0U);
}
/*
 * @tc.name: InvalidCeUserId
 * @tc.desc: Reject user IDs outside the signed HUKS tag range before any key access or output modification.
 * @tc.type: FUNC
 */
HWTEST_F(HuksSeedDeriverTest, InvalidCeUserId, TestSize.Level2)
{
    const auto userId = std::numeric_limits<uint32_t>::max();
    const HuksSeedDeriver deriver;
    EXPECT_EQ(deriver.PrepareRootKey(userId, RootKeyPolicy::CREATE_IF_MISSING), PRIVACY_PROTOCOL_INVALID_ARGUMENT);
    HeSeed seed;
    seed.bytes.fill(0x99);
    EXPECT_EQ(deriver.Derive({1000, userId}, Salt(), seed), PRIVACY_PROTOCOL_INVALID_ARGUMENT);
    EXPECT_EQ(seed.bytes.front(), 0x99);
    EXPECT_EQ(g_calls.exists, 0U);
    EXPECT_EQ(g_calls.generates, 0U);
    EXPECT_EQ(g_calls.inits, 0U);
}
} // namespace
