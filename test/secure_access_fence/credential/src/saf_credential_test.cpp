/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "saf_credential_test.h"

#include <gtest/gtest.h>

#include "saf_credential.h"
#include "saf_log.h"
#include "saf_result_code.h"

using namespace testing::ext;
namespace OHOS::Security::SAF {
namespace {

// ===== Test fixtures =====

// Whether the SA 66532 service is deployed on this device. When false, every
// credential method degrades to SAF_ERR_SERVICE_UNAVAILABLE.
bool IsSaDeployed()
{
    SafCredential cred;
    std::vector<ModelCredential> list;
    int32_t ret = cred.PreGetAllModelCredentials(list);
    if (ret == SAF_ERR_SERVICE_UNAVAILABLE) {
        LOGI("SA 66532 not deployed (ret=%{public}d)", ret);
        return false;
    }
    LOGI("SA 66532 reachable (ret=%{public}d)", ret);
    return true;
}

// Shared test constants
constexpr uint64_t TEST_AGENT_TOKEN_ID = 123456789ULL;
constexpr char TEST_SUBJECT_ID[] = "test-subject-001";
constexpr char TEST_PROVIDER[] = "openai";
constexpr char TEST_BASE_URL[] = "https://api.openai.com/v1";
constexpr char TEST_API_KEY[] = "sk-test-key-001";
constexpr char TEST_HANDLE_ALIAS[] = "test-handle-alias";
constexpr char TEST_CREDENTIAL_ALIAS[] = "test-credential-alias";

ModelCredentialContent MakeValidModelCredentialContent()
{
    ModelCredentialContent content;
    content.baseUrl = TEST_BASE_URL;
    content.credentialValue = TEST_API_KEY;
    return content;
}

ModelCredentialInfo MakeValidModelCredentialInfo()
{
    ModelCredentialInfo info;
    info.provider = TEST_PROVIDER;
    info.modelList = {"gpt-4", "gpt-3.5-turbo"};
    info.handleAlias = TEST_HANDLE_ALIAS;
    info.credentialAlias = TEST_CREDENTIAL_ALIAS;
    info.state = true;
    return info;
}

ModelCredentialIndex MakeValidModelCredentialIndex()
{
    ModelCredentialIndex index;
    index.credentialHandle = "mc_test_handle";
    index.credentialId = "test-credential-id";
    return index;
}

ModelCredential MakeValidModelCredential()
{
    ModelCredential mc;
    mc.modelCredentialContent = MakeValidModelCredentialContent();
    mc.modelCredentialInfo = MakeValidModelCredentialInfo();
    mc.modelCredentialIndex = MakeValidModelCredentialIndex();
    return mc;
}

ToolCredential MakeValidToolCredential()
{
    ToolCredential tc;
    tc.subjectId = TEST_SUBJECT_ID;
    ToolCredentialContent ci1;
    ci1.envName = "API_KEY";
    ci1.credentialValue = "tool-secret-001";
    ToolCredentialContent ci2;
    ci2.envName = "ENDPOINT";
    ci2.credentialValue = "https://tool.example.com";
    tc.toolCredentialContents = {ci1, ci2};
    return tc;
}

}  // namespace

// Fixture for functional cases: grants ACCESS_SECURE_FENCE + system_basic APL.
class SafCredentialTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
};

void SafCredentialTest::SetUpTestCase(void)
{
    LOGI("SafCredentialTest suite start");
    // Grant permission once for the whole suite. If the SA is not deployed,
    // the permission grant is harmless (no authorization check runs).
    int32_t grantRet = UnitTest::SafCredentialTest::GrantCredentialPermission();
    LOGI("GrantCredentialPermission ret=%{public}d", grantRet);
    ASSERT_EQ(0, grantRet);
}

void SafCredentialTest::TearDownTestCase(void)
{
    LOGI("SafCredentialTest suite end");
}
void SafCredentialTest::SetUp(void)
{
    LOGI("case enter");
}
void SafCredentialTest::TearDown(void)
{
    LOGI("case leave");
}

// ===== Model Credential — AddModelCredential =====

/**
 * @tc.name: SafCredentialTest.AddModelCredential_001
 * @tc.desc: Add a model credential with all valid fields.
 * @tc.type: FUNC
 * @tc.result: 0 (SA deployed) or SAF_ERR_SERVICE_UNAVAILABLE
 */
HWTEST_F(SafCredentialTest, AddModelCredential_001, TestSize.Level0)
{
    LOGI("AddModelCredential_001 start");
    SafCredential cred;
    ModelCredentialContent content = MakeValidModelCredentialContent();
    ModelCredentialInfo info = MakeValidModelCredentialInfo();
    std::string handle;
    std::string credentialId;
    LOGI("call AddModelCredential provider=%{public}s", info.provider.c_str());
    int32_t ret = cred.AddModelCredential(content, info, handle, credentialId);
    LOGI("AddModelCredential ret=%{public}d handleLen=%{public}zu idLen=%{public}zu",
        ret, handle.size(), credentialId.size());
    if (IsSaDeployed()) {
        EXPECT_EQ(ret, SAF_SUCCESS);
        EXPECT_FALSE(handle.empty());
        EXPECT_FALSE(credentialId.empty());
    } else {
        EXPECT_EQ(ret, SAF_ERR_SERVICE_UNAVAILABLE);
    }
    LOGI("AddModelCredential_001 end");
}

/**
 * @tc.name: SafCredentialTest.AddModelCredential_002
 * @tc.desc: Add a model credential with empty provider. The inner API does not
 *           validate fields — the service may return ARG_INVALID or accept it.
 *           When SA is not deployed, SERVICE_UNAVAILABLE is returned first.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialTest, AddModelCredential_002, TestSize.Level0)
{
    LOGI("AddModelCredential_002 start (empty provider)");
    SafCredential cred;
    ModelCredentialContent content = MakeValidModelCredentialContent();
    ModelCredentialInfo info = MakeValidModelCredentialInfo();
    info.provider = "";
    std::string handle;
    std::string credentialId;
    int32_t ret = cred.AddModelCredential(content, info, handle, credentialId);
    LOGI("AddModelCredential ret=%{public}d", ret);
    if (IsSaDeployed()) {
        // Service may accept or reject; only assert no crash + sensible code.
        EXPECT_TRUE(ret == SAF_SUCCESS || ret == SAF_ERR_ARG_INVALID);
    } else {
        EXPECT_EQ(ret, SAF_ERR_SERVICE_UNAVAILABLE);
    }
    LOGI("AddModelCredential_002 end");
}

// ===== Model Credential — DeleteModelCredential =====

/**
 * @tc.name: SafCredentialTest.DeleteModelCredential_001
 * @tc.desc: Batch delete with a single valid credential.
 * @tc.type: FUNC
 * @tc.result: 0 (SA deployed) or SAF_ERR_SERVICE_UNAVAILABLE
 */
HWTEST_F(SafCredentialTest, DeleteModelCredential_001, TestSize.Level0)
{
    LOGI("DeleteModelCredential_001 start");
    SafCredential cred;
    std::vector<ModelCredential> list = {MakeValidModelCredential()};
    CredentialBatchResult result;
    int32_t ret = cred.DeleteModelCredential(list, result);
    LOGI("DeleteModelCredential ret=%{public}d", ret);
    if (IsSaDeployed()) {
        EXPECT_EQ(ret, SAF_SUCCESS);
    } else {
        EXPECT_EQ(ret, SAF_ERR_SERVICE_UNAVAILABLE);
    }
    LOGI("DeleteModelCredential_001 end");
}

/**
 * @tc.name: SafCredentialTest.DeleteModelCredential_002
 * @tc.desc: Batch delete with an empty array. Inner API does not validate; the
 *           service may return SAF_SUCCESS or ARG_INVALID.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialTest, DeleteModelCredential_002, TestSize.Level0)
{
    LOGI("DeleteModelCredential_002 start (empty list)");
    SafCredential cred;
    std::vector<ModelCredential> list;
    CredentialBatchResult result;
    int32_t ret = cred.DeleteModelCredential(list, result);
    LOGI("DeleteModelCredential ret=%{public}d", ret);
    if (IsSaDeployed()) {
        EXPECT_TRUE(ret == SAF_SUCCESS || ret == SAF_ERR_ARG_INVALID);
    } else {
        EXPECT_EQ(ret, SAF_ERR_SERVICE_UNAVAILABLE);
    }
    LOGI("DeleteModelCredential_002 end");
}

// ===== Model Credential — UpdateModelCredential =====

/**
 * @tc.name: SafCredentialTest.UpdateModelCredential_001
 * @tc.desc: Update with src/dst of equal length. Service returns SUCCESS.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialTest, UpdateModelCredential_001, TestSize.Level0)
{
    LOGI("UpdateModelCredential_001 start");
    SafCredential cred;
    std::vector<ModelCredential> src = {MakeValidModelCredential()};
    std::vector<ModelCredential> dst = {MakeValidModelCredential()};
    dst[0].modelCredentialContent.credentialValue = "sk-updated-key";
    CredentialBatchResult result;
    int32_t ret = cred.UpdateModelCredential(src, dst, result);
    LOGI("UpdateModelCredential ret=%{public}d", ret);
    if (IsSaDeployed()) {
        EXPECT_EQ(ret, SAF_SUCCESS);
    } else {
        EXPECT_EQ(ret, SAF_ERR_SERVICE_UNAVAILABLE);
    }
    LOGI("UpdateModelCredential_001 end");
}

/**
 * @tc.name: SafCredentialTest.UpdateModelCredential_002
 * @tc.desc: Update with src/dst length mismatch. The inner API checks this
 *           BEFORE IPC, so ARG_INVALID is returned in both environments.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialTest, UpdateModelCredential_002, TestSize.Level0)
{
    LOGI("UpdateModelCredential_002 start (length mismatch — inner API checks before IPC)");
    SafCredential cred;
    std::vector<ModelCredential> src = {MakeValidModelCredential()};
    std::vector<ModelCredential> dst;  // empty -> length mismatch
    CredentialBatchResult result;
    int32_t ret = cred.UpdateModelCredential(src, dst, result);
    LOGI("UpdateModelCredential ret=%{public}d (expect ARG_INVALID)", ret);
    EXPECT_EQ(ret, SAF_ERR_ARG_INVALID);
    LOGI("UpdateModelCredential_002 end");
}

/**
 * @tc.name: SafCredentialTest.UpdateModelCredential_003
 * @tc.desc: Update with both src and dst empty. Equal length (0 == 0), so the
 *           inner API does not reject; SERVICE_UNAVAILABLE when SA not deployed,
 *           service may return SUCCESS or ARG_INVALID when deployed.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialTest, UpdateModelCredential_003, TestSize.Level0)
{
    LOGI("UpdateModelCredential_003 start (both empty, 0==0)");
    SafCredential cred;
    std::vector<ModelCredential> src;
    std::vector<ModelCredential> dst;
    CredentialBatchResult result;
    int32_t ret = cred.UpdateModelCredential(src, dst, result);
    LOGI("UpdateModelCredential ret=%{public}d", ret);
    if (IsSaDeployed()) {
        EXPECT_TRUE(ret == SAF_SUCCESS || ret == SAF_ERR_ARG_INVALID);
    } else {
        EXPECT_EQ(ret, SAF_ERR_SERVICE_UNAVAILABLE);
    }
    LOGI("UpdateModelCredential_003 end");
}

// ===== Model Credential — GetActiveModelCredential =====

/**
 * @tc.name: SafCredentialTest.GetActiveModelCredential_001
 * @tc.desc: Query with a non-empty handle and a model filter.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialTest, GetActiveModelCredential_001, TestSize.Level0)
{
    LOGI("GetActiveModelCredential_001 start");
    SafCredential cred;
    ModelCredentialContent content;
    int32_t ret = cred.GetActiveModelCredential("mc_some_handle", "gpt-4", content);
    LOGI("GetActiveModelCredential ret=%{public}d", ret);
    if (IsSaDeployed()) {
        EXPECT_TRUE(ret == SAF_SUCCESS || ret == SAF_ERR_ARG_INVALID);
    } else {
        EXPECT_EQ(ret, SAF_ERR_SERVICE_UNAVAILABLE);
    }
    LOGI("GetActiveModelCredential_001 end");
}

/**
 * @tc.name: SafCredentialTest.GetActiveModelCredential_002
 * @tc.desc: Query with an empty handle string. Inner API does not validate.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialTest, GetActiveModelCredential_002, TestSize.Level0)
{
    LOGI("GetActiveModelCredential_002 start (empty handle)");
    SafCredential cred;
    ModelCredentialContent content;
    int32_t ret = cred.GetActiveModelCredential("", "", content);
    LOGI("GetActiveModelCredential ret=%{public}d", ret);
    if (IsSaDeployed()) {
        EXPECT_TRUE(ret == SAF_SUCCESS || ret == SAF_ERR_ARG_INVALID);
    } else {
        EXPECT_EQ(ret, SAF_ERR_SERVICE_UNAVAILABLE);
    }
    LOGI("GetActiveModelCredential_002 end");
}

// ===== Model Credential — PreGetAllModelCredentials =====

/**
 * @tc.name: SafCredentialTest.PreGetAllModelCredentials_001
 * @tc.desc: Query all model credentials (metadata only, credentialValue empty).
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialTest, PreGetAllModelCredentials_001, TestSize.Level0)
{
    LOGI("PreGetAllModelCredentials_001 start");
    SafCredential cred;
    std::vector<ModelCredential> out;
    int32_t ret = cred.PreGetAllModelCredentials(out);
    LOGI("PreGetAllModelCredentials ret=%{public}d outSize=%{public}zu", ret, out.size());
    if (IsSaDeployed()) {
        EXPECT_EQ(ret, SAF_SUCCESS);
        for (const auto &mc : out) {
            EXPECT_TRUE(mc.modelCredentialContent.credentialValue.empty());
        }
    } else {
        EXPECT_EQ(ret, SAF_ERR_SERVICE_UNAVAILABLE);
    }
    LOGI("PreGetAllModelCredentials_001 end");
}

// ===== Model Credential — GetModelCredentialValue =====

/**
 * @tc.name: SafCredentialTest.GetModelCredentialValue_001
 * @tc.desc: Query one credential value by index.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialTest, GetModelCredentialValue_001, TestSize.Level0)
{
    LOGI("GetModelCredentialValue_001 start");
    SafCredential cred;
    ModelCredentialIndex index = MakeValidModelCredentialIndex();
    std::string credentialValue;
    int32_t ret = cred.GetModelCredentialValue(index, credentialValue);
    LOGI("GetModelCredentialValue ret=%{public}d valueLen=%{public}zu", ret, credentialValue.size());
    if (IsSaDeployed()) {
        EXPECT_TRUE(ret == SAF_SUCCESS || ret == SAF_ERR_ARG_INVALID);
    } else {
        EXPECT_EQ(ret, SAF_ERR_SERVICE_UNAVAILABLE);
    }
    LOGI("GetModelCredentialValue_001 end");
}

/**
 * @tc.name: SafCredentialTest.GetModelCredentialValue_002
 * @tc.desc: Query with an empty index. Inner API does not validate.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialTest, GetModelCredentialValue_002, TestSize.Level0)
{
    LOGI("GetModelCredentialValue_002 start (empty index)");
    SafCredential cred;
    ModelCredentialIndex index;
    std::string credentialValue;
    int32_t ret = cred.GetModelCredentialValue(index, credentialValue);
    LOGI("GetModelCredentialValue ret=%{public}d", ret);
    if (IsSaDeployed()) {
        EXPECT_TRUE(ret == SAF_SUCCESS || ret == SAF_ERR_ARG_INVALID);
    } else {
        EXPECT_EQ(ret, SAF_ERR_SERVICE_UNAVAILABLE);
    }
    LOGI("GetModelCredentialValue_002 end");
}

// ===== Tool Credential — AddToolCredential =====

/**
 * @tc.name: SafCredentialTest.AddToolCredential_001
 * @tc.desc: Add a tool credential with all valid fields.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialTest, AddToolCredential_001, TestSize.Level0)
{
    LOGI("AddToolCredential_001 start");
    SafCredential cred;
    std::vector<ToolCredential> tools = {MakeValidToolCredential()};
    CredentialBatchResult result;
    int32_t ret = cred.AddToolCredential(tools, result);
    LOGI("AddToolCredential ret=%{public}d", ret);
    if (IsSaDeployed()) {
        EXPECT_EQ(ret, SAF_SUCCESS);
    } else {
        EXPECT_EQ(ret, SAF_ERR_SERVICE_UNAVAILABLE);
    }
    LOGI("AddToolCredential_001 end");
}

/**
 * @tc.name: SafCredentialTest.AddToolCredential_002
 * @tc.desc: Add a tool credential with empty toolCredentialContents. Inner API
 *           does not validate; service may return ARG_INVALID.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialTest, AddToolCredential_002, TestSize.Level0)
{
    LOGI("AddToolCredential_002 start (empty toolCredentialContents)");
    SafCredential cred;
    ToolCredential tc = MakeValidToolCredential();
    tc.toolCredentialContents.clear();
    std::vector<ToolCredential> tools = {tc};
    CredentialBatchResult result;
    int32_t ret = cred.AddToolCredential(tools, result);
    LOGI("AddToolCredential ret=%{public}d", ret);
    if (IsSaDeployed()) {
        EXPECT_TRUE(ret == SAF_SUCCESS || ret == SAF_ERR_ARG_INVALID);
    } else {
        EXPECT_EQ(ret, SAF_ERR_SERVICE_UNAVAILABLE);
    }
    LOGI("AddToolCredential_002 end");
}

// ===== Tool Credential — UpdateToolCredential =====

/**
 * @tc.name: SafCredentialTest.UpdateToolCredential_001
 * @tc.desc: Update the secret value of an existing env name.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialTest, UpdateToolCredential_001, TestSize.Level0)
{
    LOGI("UpdateToolCredential_001 start");
    SafCredential cred;
    std::vector<ToolCredential> tools = {MakeValidToolCredential()};
    tools[0].toolCredentialContents[0].credentialValue = "updated-tool-secret";
    CredentialBatchResult result;
    int32_t ret = cred.UpdateToolCredential(tools, result);
    LOGI("UpdateToolCredential ret=%{public}d", ret);
    if (IsSaDeployed()) {
        EXPECT_TRUE(ret == SAF_SUCCESS || ret == SAF_ERR_ARG_INVALID);
    } else {
        EXPECT_EQ(ret, SAF_ERR_SERVICE_UNAVAILABLE);
    }
    LOGI("UpdateToolCredential_001 end");
}

/**
 * @tc.name: SafCredentialTest.UpdateToolCredential_002
 * @tc.desc: Update with an empty array.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialTest, UpdateToolCredential_002, TestSize.Level0)
{
    LOGI("UpdateToolCredential_002 start (empty array)");
    SafCredential cred;
    std::vector<ToolCredential> tools;
    CredentialBatchResult result;
    int32_t ret = cred.UpdateToolCredential(tools, result);
    LOGI("UpdateToolCredential ret=%{public}d", ret);
    if (IsSaDeployed()) {
        EXPECT_TRUE(ret == SAF_SUCCESS || ret == SAF_ERR_ARG_INVALID);
    } else {
        EXPECT_EQ(ret, SAF_ERR_SERVICE_UNAVAILABLE);
    }
    LOGI("UpdateToolCredential_002 end");
}

// ===== Tool Credential — DeleteToolCredential =====

/**
 * @tc.name: SafCredentialTest.DeleteToolCredential_001
 * @tc.desc: Batch delete with a valid ToolCredential.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialTest, DeleteToolCredential_001, TestSize.Level0)
{
    LOGI("DeleteToolCredential_001 start");
    SafCredential cred;
    std::vector<ToolCredential> tools = {MakeValidToolCredential()};
    CredentialBatchResult result;
    int32_t ret = cred.DeleteToolCredential(tools, result);
    LOGI("DeleteToolCredential ret=%{public}d", ret);
    if (IsSaDeployed()) {
        EXPECT_TRUE(ret == SAF_SUCCESS || ret == SAF_ERR_ARG_INVALID);
    } else {
        EXPECT_EQ(ret, SAF_ERR_SERVICE_UNAVAILABLE);
    }
    LOGI("DeleteToolCredential_001 end");
}

/**
 * @tc.name: SafCredentialTest.DeleteToolCredential_002
 * @tc.desc: Batch delete with an empty array. Inner API does not validate.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialTest, DeleteToolCredential_002, TestSize.Level0)
{
    LOGI("DeleteToolCredential_002 start (empty array)");
    SafCredential cred;
    std::vector<ToolCredential> tools;
    CredentialBatchResult result;
    int32_t ret = cred.DeleteToolCredential(tools, result);
    LOGI("DeleteToolCredential ret=%{public}d", ret);
    if (IsSaDeployed()) {
        EXPECT_TRUE(ret == SAF_SUCCESS || ret == SAF_ERR_ARG_INVALID);
    } else {
        EXPECT_EQ(ret, SAF_ERR_SERVICE_UNAVAILABLE);
    }
    LOGI("DeleteToolCredential_002 end");
}

// ===== Tool Credential — GetToolCredential =====

/**
 * @tc.name: SafCredentialTest.GetToolCredential_001
 * @tc.desc: Query with a valid ToolInfo (agentTokenId + subjectId).
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialTest, GetToolCredential_001, TestSize.Level0)
{
    LOGI("GetToolCredential_001 start");
    SafCredential cred;
    ToolInfo info;
    info.agentTokenId = TEST_AGENT_TOKEN_ID;
    info.subjectId = TEST_SUBJECT_ID;
    std::vector<ToolCredentialInfo> out;
    int32_t ret = cred.GetToolCredential(info, out);
    LOGI("GetToolCredential ret=%{public}d outSize=%{public}zu", ret, out.size());
    for (size_t i = 0; i < out.size(); ++i) {
        LOGI("GetToolCredential out[%{public}zu]: envName=%{public}s credentialValue=%{public}s",
            i, out[i].envName.c_str(), out[i].credentialValue.c_str());
    }
    if (IsSaDeployed()) {
        EXPECT_EQ(ret, SAF_SUCCESS);
    } else {
        EXPECT_EQ(ret, SAF_ERR_SERVICE_UNAVAILABLE);
    }
    LOGI("GetToolCredential_001 end");
}

/**
 * @tc.name: SafCredentialTest.GetToolCredential_002
 * @tc.desc: Query with empty subjectId. Inner API does not validate.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialTest, GetToolCredential_002, TestSize.Level0)
{
    LOGI("GetToolCredential_002 start (empty subjectId)");
    SafCredential cred;
    ToolInfo info;
    info.agentTokenId = TEST_AGENT_TOKEN_ID;
    info.subjectId = "";
    std::vector<ToolCredentialInfo> out;
    int32_t ret = cred.GetToolCredential(info, out);
    LOGI("GetToolCredential ret=%{public}d outSize=%{public}zu", ret, out.size());
    if (IsSaDeployed()) {
        EXPECT_TRUE(ret == SAF_SUCCESS || ret == SAF_ERR_ARG_INVALID);
    } else {
        EXPECT_EQ(ret, SAF_ERR_SERVICE_UNAVAILABLE);
    }
    LOGI("GetToolCredential_002 end");
}

}  // namespace OHOS::Security::SAF
