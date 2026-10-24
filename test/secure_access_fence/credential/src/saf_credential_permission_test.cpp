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

// Whether the SA 66532 service is deployed on this device. When false, the IPC
// fails with SAF_ERR_SERVICE_UNAVAILABLE before any authorization check runs.
bool IsSaDeployed()
{
    SafCredential cred;
    std::vector<ModelCredential> list;
    int32_t ret = cred.GetAllModelCredentials(list);
    if (ret == SAF_ERR_SERVICE_UNAVAILABLE) {
        LOGI("SA 66532 not deployed (ret=%{public}d)", ret);
        return false;
    }
    LOGI("SA 66532 reachable (ret=%{public}d)", ret);
    return true;
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
 * @tc.desc: AddModelCredential has no ACCESS_SECURE_FENCE check on the server
 *           (only the system-app-202 gate described in d.ts, which is enforced
 *           by the permission application threshold, not by the service). A
 *           normal-app caller therefore reaches the DIA layer. When SA is not
 *           deployed, SERVICE_UNAVAILABLE is returned first.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialPermissionTest, AddModelCredential_NoPermission_001, TestSize.Level0)
{
    LOGI("AddModelCredential_NoPermission_001 start");
    SafCredential cred;
    ModelCredential mc;
    mc.provider = "x";
    mc.baseUrl = "y";
    mc.credentialValue = "z";
    std::string handle;
    int32_t ret = cred.AddModelCredential(mc, handle);
    LOGI("AddModelCredential ret=%{public}d (expect SUCCESS or SERVICE_UNAVAILABLE)", ret);
    if (IsSaDeployed()) {
        // Server does not gate AddModelCredential; it delegates straight to DIA.
        EXPECT_EQ(ret, SAF_SUCCESS);
    } else {
        EXPECT_EQ(ret, SAF_ERR_SERVICE_UNAVAILABLE);
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
        EXPECT_EQ(ret, SAF_ERR_SERVICE_UNAVAILABLE);
    }
    LOGI("DeleteModelCredential_NoPermission_001 end");
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
    ModelCredentialInfo info;
    int32_t ret = cred.GetActiveModelCredential("any-handle", info);
    LOGI("GetActiveModelCredential ret=%{public}d (expect PERMISSION_DENIED)", ret);
    if (IsSaDeployed()) {
        EXPECT_EQ(ret, SAF_ERR_PERMISSION_DENIED);
    } else {
        EXPECT_EQ(ret, SAF_ERR_SERVICE_UNAVAILABLE);
    }
    LOGI("GetActiveModelCredential_NoPermission_001 end");
}

/**
 * @tc.name: SafCredentialPermissionTest.AddToolCredential_NoPermission_001
 * @tc.desc: AddToolCredential has no ACCESS_SECURE_FENCE check on the server
 *           (only the system-app-202 gate described in d.ts, which is enforced
 *           by the permission application threshold, not by the service). A
 *           normal-app caller therefore reaches the DIA layer. When SA is not
 *           deployed, SERVICE_UNAVAILABLE is returned first.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialPermissionTest, AddToolCredential_NoPermission_001, TestSize.Level0)
{
    LOGI("AddToolCredential_NoPermission_001 start");
    SafCredential cred;
    ToolCredential tc;
    tc.toolInfo.agentTokenId = TEST_AGENT_TOKEN_ID;
    tc.toolInfo.subjectId = TEST_SUBJECT_ID;
    ToolCredentialInfo ci;
    ci.envName = "API_KEY";
    ci.credentialValue = "secret";
    tc.toolCredentialInfos = {ci};
    std::vector<ToolCredential> tools = {tc};
    CredentialBatchResult result;
    int32_t ret = cred.AddToolCredential(tools, result);
    LOGI("AddToolCredential ret=%{public}d (expect SUCCESS or SERVICE_UNAVAILABLE)", ret);
    if (IsSaDeployed()) {
        // Server does not gate AddToolCredential; it delegates straight to DIA.
        EXPECT_EQ(ret, SAF_SUCCESS);
    } else {
        EXPECT_EQ(ret, SAF_ERR_SERVICE_UNAVAILABLE);
    }
    LOGI("AddToolCredential_NoPermission_001 end");
}

}  // namespace OHOS::Security::SAF
