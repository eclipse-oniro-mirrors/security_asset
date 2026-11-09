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

#include <gtest/gtest.h>

#include "nativetoken_kit.h"
#include "token_setproc.h"

#include "saf_credential.h"
#include "saf_log.h"
#include "saf_result_code.h"

using namespace testing::ext;
namespace OHOS::Security::SAF {
namespace {

// Whether the credential APIs of SA 66532 are usable on this device.
//
// Two environments yield "not usable":
//   - No SA 66532 at all: the IPC layer reports SAF_ERR_SERVICE_UNAVAILABLE.
//   - An SA 66532 built before the credential plugin existed: the service is
//     reachable but rejects the credential ipccode, so the proxy reports
//     SAF_ERR_IPC_PROXY_FAIL.
bool IsSaDeployed()
{
    SafCredential cred;
    std::vector<ModelCredential> list;
    int32_t ret = cred.PreGetAllModelCredentials(list);
    if (ret == SAF_ERR_SERVICE_UNAVAILABLE || ret == SAF_ERR_IPC_PROXY_FAIL) {
        LOGI("credential API of SA 66532 not available (ret=%{public}d)", ret);
        return false;
    }
    LOGI("credential API of SA 66532 reachable (ret=%{public}d)", ret);
    return true;
}

// The code every credential call degrades to when the API is not usable.
int32_t DegradedCode()
{
    SafCredential cred;
    std::vector<ModelCredential> list;
    int32_t ret = cred.PreGetAllModelCredentials(list);
    return (ret == SAF_ERR_SERVICE_UNAVAILABLE || ret == SAF_ERR_IPC_PROXY_FAIL) ?
        ret : SAF_ERR_SERVICE_UNAVAILABLE;
}

constexpr uint64_t TEST_AGENT_TOKEN_ID = 123456789ULL;
constexpr char TEST_SUBJECT_ID[] = "test-subject-perm";

}  // namespace

// Fixture that sets an APL_NORMAL token with no permissions, simulating a
// non-system application caller. The service's authorization check
// (CheckSignaturePermission) returns SAF_ERR_PERMISSION_DENIED (201) because
// the caller lacks ohos.permission.ACCESS_SECURE_FENCE.
//
// When SA 66532 is not deployed, the inner API returns
// SAF_ERR_SERVICE_UNAVAILABLE before any authorization check runs, so the
// assertion accepts either the authorization code or SERVICE_UNAVAILABLE.
class SafCredentialPermissionTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
};

void SafCredentialPermissionTest::SetUpTestCase(void)
{
    LOGI("SafCredentialPermissionTest suite start (APL_NORMAL, no perms)");
    // Set an APL_NORMAL token with zero permissions — simulates a normal app.
    NativeTokenInfoParams info = {
        .dcapsNum = 0,
        .permsNum = 0,
        .aclsNum = 0,
        .dcaps = nullptr,
        .perms = nullptr,
        .acls = nullptr,
        .processName = "saf_credential_perm_test",
        .aplStr = "normal",
    };
    uint64_t tokenId = GetAccessTokenId(&info);
    (void)SetSelfTokenID(tokenId);
    LOGI("SetSelfTokenID tokenId=%{public}llu", (unsigned long long)tokenId);
}

void SafCredentialPermissionTest::TearDownTestCase(void)
{
    LOGI("SafCredentialPermissionTest suite end");
}
void SafCredentialPermissionTest::SetUp(void)
{
    LOGI("case enter");
}
void SafCredentialPermissionTest::TearDown(void)
{
    LOGI("case leave");
}

/**
 * @tc.name: SafCredentialPermissionTest.AddModelCredential_NoPermission_001
 * @tc.desc: AddModelCredential requires ACCESS_SECURE_FENCE (201) on the
 *           server. A normal-app caller is rejected with PERMISSION_DENIED.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialPermissionTest, AddModelCredential_NoPermission_001, TestSize.Level0)
{
    LOGI("AddModelCredential_NoPermission_001 start");
    SafCredential cred;
    ModelCredentialContent content;
    content.baseUrl = "https://api.example.com";
    content.credentialValue = "secret";
    ModelCredentialInfo info;
    info.provider = "x";
    info.modelList = {"m"};
    info.handleAlias = "alias";
    std::string handle;
    std::string credentialId;
    int32_t ret = cred.AddModelCredential(content, info, handle, credentialId);
    LOGI("AddModelCredential ret=%{public}d (expect PERMISSION_DENIED)", ret);
    if (IsSaDeployed()) {
        EXPECT_EQ(ret, SAF_ERR_PERMISSION_DENIED);
    } else {
        EXPECT_EQ(ret, DegradedCode());
    }
    LOGI("AddModelCredential_NoPermission_001 end");
}

/**
 * @tc.name: SafCredentialPermissionTest.DeleteModelCredential_NoPermission_001
 * @tc.desc: DeleteModelCredential requires ACCESS_SECURE_FENCE (201). A
 *           normal-app caller is rejected with PERMISSION_DENIED.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialPermissionTest, DeleteModelCredential_NoPermission_001, TestSize.Level0)
{
    LOGI("DeleteModelCredential_NoPermission_001 start");
    SafCredential cred;
    std::vector<ModelCredential> list;
    CredentialBatchResult result;
    int32_t ret = cred.DeleteModelCredential(list, result);
    LOGI("DeleteModelCredential ret=%{public}d (expect PERMISSION_DENIED)", ret);
    if (IsSaDeployed()) {
        EXPECT_EQ(ret, SAF_ERR_PERMISSION_DENIED);
    } else {
        EXPECT_EQ(ret, DegradedCode());
    }
    LOGI("DeleteModelCredential_NoPermission_001 end");
}

/**
 * @tc.name: SafCredentialPermissionTest.UpdateModelCredential_NoPermission_001
 * @tc.desc: UpdateModelCredential requires ACCESS_SECURE_FENCE (201). A
 *           normal-app caller is rejected with PERMISSION_DENIED.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialPermissionTest, UpdateModelCredential_NoPermission_001, TestSize.Level0)
{
    LOGI("UpdateModelCredential_NoPermission_001 start");
    SafCredential cred;
    std::vector<ModelCredential> src;
    std::vector<ModelCredential> dst;
    CredentialBatchResult result;
    int32_t ret = cred.UpdateModelCredential(src, dst, result);
    LOGI("UpdateModelCredential ret=%{public}d (expect PERMISSION_DENIED)", ret);
    if (IsSaDeployed()) {
        EXPECT_EQ(ret, SAF_ERR_PERMISSION_DENIED);
    } else {
        EXPECT_EQ(ret, DegradedCode());
    }
    LOGI("UpdateModelCredential_NoPermission_001 end");
}

/**
 * @tc.name: SafCredentialPermissionTest.PreGetAllModelCredentials_NoPermission_001
 * @tc.desc: PreGetAllModelCredentials requires ACCESS_SECURE_FENCE (201).
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialPermissionTest, PreGetAllModelCredentials_NoPermission_001, TestSize.Level0)
{
    LOGI("PreGetAllModelCredentials_NoPermission_001 start");
    SafCredential cred;
    std::vector<ModelCredential> list;
    int32_t ret = cred.PreGetAllModelCredentials(list);
    LOGI("PreGetAllModelCredentials ret=%{public}d (expect PERMISSION_DENIED)", ret);
    if (IsSaDeployed()) {
        EXPECT_EQ(ret, SAF_ERR_PERMISSION_DENIED);
    } else {
        EXPECT_EQ(ret, DegradedCode());
    }
    LOGI("PreGetAllModelCredentials_NoPermission_001 end");
}

/**
 * @tc.name: SafCredentialPermissionTest.GetActiveModelCredential_NoPermission_001
 * @tc.desc: GetActiveModelCredential requires ACCESS_SECURE_FENCE (201).
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialPermissionTest, GetActiveModelCredential_NoPermission_001, TestSize.Level0)
{
    LOGI("GetActiveModelCredential_NoPermission_001 start");
    SafCredential cred;
    ModelCredentialContent content;
    int32_t ret = cred.GetActiveModelCredential("mc_any_handle", "m", content);
    LOGI("GetActiveModelCredential ret=%{public}d (expect PERMISSION_DENIED)", ret);
    if (IsSaDeployed()) {
        EXPECT_EQ(ret, SAF_ERR_PERMISSION_DENIED);
    } else {
        EXPECT_EQ(ret, DegradedCode());
    }
    LOGI("GetActiveModelCredential_NoPermission_001 end");
}

/**
 * @tc.name: SafCredentialPermissionTest.GetModelCredentialValue_NoPermission_001
 * @tc.desc: GetModelCredentialValue requires ACCESS_SECURE_FENCE (201).
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialPermissionTest, GetModelCredentialValue_NoPermission_001, TestSize.Level0)
{
    LOGI("GetModelCredentialValue_NoPermission_001 start");
    SafCredential cred;
    ModelCredentialIndex index;
    index.credentialHandle = "mc_any_handle";
    index.credentialId = "any-id";
    std::string credentialValue;
    int32_t ret = cred.GetModelCredentialValue(index, credentialValue);
    LOGI("GetModelCredentialValue ret=%{public}d (expect PERMISSION_DENIED)", ret);
    if (IsSaDeployed()) {
        EXPECT_EQ(ret, SAF_ERR_PERMISSION_DENIED);
    } else {
        EXPECT_EQ(ret, DegradedCode());
    }
    LOGI("GetModelCredentialValue_NoPermission_001 end");
}

/**
 * @tc.name: SafCredentialPermissionTest.AddToolCredential_NoPermission_001
 * @tc.desc: AddToolCredential requires ACCESS_SECURE_FENCE (201) on the
 *           server. A normal-app caller is rejected with PERMISSION_DENIED.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialPermissionTest, AddToolCredential_NoPermission_001, TestSize.Level0)
{
    LOGI("AddToolCredential_NoPermission_001 start");
    SafCredential cred;
    ToolCredential tc;
    tc.subjectId = TEST_SUBJECT_ID;
    ToolCredentialContent ci;
    ci.envName = "API_KEY";
    ci.credentialValue = "secret";
    tc.toolCredentialContents = {ci};
    std::vector<ToolCredential> tools = {tc};
    CredentialBatchResult result;
    int32_t ret = cred.AddToolCredential(tools, result);
    LOGI("AddToolCredential ret=%{public}d (expect PERMISSION_DENIED)", ret);
    if (IsSaDeployed()) {
        EXPECT_EQ(ret, SAF_ERR_PERMISSION_DENIED);
    } else {
        EXPECT_EQ(ret, DegradedCode());
    }
    LOGI("AddToolCredential_NoPermission_001 end");
}

/**
 * @tc.name: SafCredentialPermissionTest.UpdateToolCredential_NoPermission_001
 * @tc.desc: UpdateToolCredential requires ACCESS_SECURE_FENCE (201).
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialPermissionTest, UpdateToolCredential_NoPermission_001, TestSize.Level0)
{
    LOGI("UpdateToolCredential_NoPermission_001 start");
    SafCredential cred;
    ToolCredential tc;
    tc.subjectId = TEST_SUBJECT_ID;
    ToolCredentialContent ci;
    ci.envName = "API_KEY";
    ci.credentialValue = "secret";
    tc.toolCredentialContents = {ci};
    std::vector<ToolCredential> tools = {tc};
    CredentialBatchResult result;
    int32_t ret = cred.UpdateToolCredential(tools, result);
    LOGI("UpdateToolCredential ret=%{public}d (expect PERMISSION_DENIED)", ret);
    if (IsSaDeployed()) {
        EXPECT_EQ(ret, SAF_ERR_PERMISSION_DENIED);
    } else {
        EXPECT_EQ(ret, DegradedCode());
    }
    LOGI("UpdateToolCredential_NoPermission_001 end");
}

/**
 * @tc.name: SafCredentialPermissionTest.DeleteToolCredential_NoPermission_001
 * @tc.desc: DeleteToolCredential requires ACCESS_SECURE_FENCE (201).
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialPermissionTest, DeleteToolCredential_NoPermission_001, TestSize.Level0)
{
    LOGI("DeleteToolCredential_NoPermission_001 start");
    SafCredential cred;
    ToolCredential tc;
    tc.subjectId = TEST_SUBJECT_ID;
    ToolCredentialContent ci;
    ci.envName = "API_KEY";
    ci.credentialValue = "secret";
    tc.toolCredentialContents = {ci};
    std::vector<ToolCredential> tools = {tc};
    CredentialBatchResult result;
    int32_t ret = cred.DeleteToolCredential(tools, result);
    LOGI("DeleteToolCredential ret=%{public}d (expect PERMISSION_DENIED)", ret);
    if (IsSaDeployed()) {
        EXPECT_EQ(ret, SAF_ERR_PERMISSION_DENIED);
    } else {
        EXPECT_EQ(ret, DegradedCode());
    }
    LOGI("DeleteToolCredential_NoPermission_001 end");
}

/**
 * @tc.name: SafCredentialPermissionTest.GetToolCredential_NoPermission_001
 * @tc.desc: GetToolCredential requires ACCESS_SECURE_FENCE (201).
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialPermissionTest, GetToolCredential_NoPermission_001, TestSize.Level0)
{
    LOGI("GetToolCredential_NoPermission_001 start");
    SafCredential cred;
    ToolInfo info;
    info.agentTokenId = TEST_AGENT_TOKEN_ID;
    info.subjectId = TEST_SUBJECT_ID;
    std::vector<ToolCredentialInfo> out;
    int32_t ret = cred.GetToolCredential(info, out);
    LOGI("GetToolCredential ret=%{public}d (expect PERMISSION_DENIED)", ret);
    if (IsSaDeployed()) {
        EXPECT_EQ(ret, SAF_ERR_PERMISSION_DENIED);
    } else {
        EXPECT_EQ(ret, DegradedCode());
    }
    LOGI("GetToolCredential_NoPermission_001 end");
}

}  // namespace OHOS::Security::SAF
