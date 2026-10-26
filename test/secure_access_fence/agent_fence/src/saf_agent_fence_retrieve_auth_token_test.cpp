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

#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <unistd.h>

#include "saf_agent_fence.h"
#include "saf_result_code.h"
#include "saf_permission_change.h"
#include "token_setproc.h"

#include "accesstoken_kit.h"
#include "hap_token_info.h"
#include "access_token.h"
#include "access_token_basic_type.h"
#include "permission_state_full.h"
#include "nativetoken_kit.h"

using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::Security::SAF;
using namespace OHOS::Security::AccessToken;

// Holds the system HAP token (userID=100) allocated in GrantTestPermission.
// Used as the callerToken parameter of RetrieveAuthTokenByTaskId; the same
// token is also set as the calling identity via SetSelfTokenID so that
// GenerateTaskChallenge/GenerateTaskIdentity resolve os_account_id=100.
static int32_t g_testHapTokenId = 0;

class SafAgentFenceRetrieveAuthTokenTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void) {}
    void SetUp(void) {}
    void TearDown(void) {}
};

// Creates a system HAP token with userID=100 (osAccountId=100) and grants
// ohos.permission.QUERY_TOOL_PERMISSIONS, then switches the calling identity to
// it. The service's check_query_tool_permission requires both CheckPermission
// (QUERY_TOOL_PERMISSIONS) and CheckIsSystemHap(), and GetOsAccountIdFromTokenId
// returns the HAP token's userID. Native tokens return 0 for that call, so a HAP
// token is required for GenerateTaskChallenge/GenerateTaskIdentity to succeed.
static void GrantTestPermission()
{
    // Step 1: native token with SAF permissions (gives privilege to allocate HAP tokens
    // and restarts the access_token service).
    GrantSelfPermission();

    // Step 2: native token with MANAGE_HAP_TOKENID (system_core APL required).
    // InitHapToken verifies VerifyAccessToken(caller, MANAGE_HAP_TOKENID) unless the
    // caller is root in debug mode. The native token from GrantSelfPermission (system_basic)
    // lacks this permission, so a separate system_core native token is needed.
    const char *hapManagePerms[] = { "ohos.permission.MANAGE_HAP_TOKENID" };
    NativeTokenInfoParams manageInfo = {
        .dcapsNum = 0,
        .permsNum = 1,
        .aclsNum = 0,
        .dcaps = nullptr,
        .perms = hapManagePerms,
        .acls = nullptr,
        .processName = "saf_hap_token_allocator",
        .aplStr = "system_core",
    };
    uint64_t manageTokenId = GetAccessTokenId(&manageInfo);
    SetSelfTokenID(manageTokenId);

    // Step 3: allocate a system HAP token with userID=100.
    HapInfoParams info = {
        .userID = 100, // 100: osAccountId
        .bundleName = "saf_retrieve_auth_token_test",
        .instIndex = 0,
        .appIDDesc = "saf_retrieve_auth_token_test",
        .apiVersion = 11,
        .isSystemApp = true,
    };
    PermissionStateFull permState = {
        .permissionName = "ohos.permission.QUERY_TOOL_PERMISSIONS",
        .isGeneral = true,
        .resDeviceID = {"local"},
        .grantStatus = {PERMISSION_GRANTED},
        .grantFlags = {PERMISSION_DEFAULT_FLAG},
    };
    HapPolicyParams policy = {
        .apl = APL_SYSTEM_BASIC,
        .domain = "saf_retrieve_auth_token_test",
        .permStateList = {permState},
    };
    AccessTokenIDEx tokenIdEx = {0};
    ASSERT_EQ(RET_SUCCESS, AccessTokenKit::InitHapToken(info, policy, tokenIdEx));
    ASSERT_NE(INVALID_TOKENID, tokenIdEx.tokenIdExStruct.tokenID);
    g_testHapTokenId = static_cast<int32_t>(tokenIdEx.tokenIdExStruct.tokenID);
    SetSelfTokenID(tokenIdEx.tokenIDEx);
}

void SafAgentFenceRetrieveAuthTokenTest::SetUpTestCase(void)
{
    GrantTestPermission();
}

static TaskIdentityInfo MakeLowRiskInfo(const std::string &taskId, int32_t expireTime)
{
    TaskIdentityInfo info = {};
    info.taskId = taskId;
    info.taskExpireTime = expireTime;
    info.executeType = ExecuteType::INSTANT;
    info.allowScreenLockExecution = false;
    info.allowableDuration = 0;
    return info;
}

static TaskIdentityInfo MakeDelayedInfo(const std::string &taskId, int32_t expireTime)
{
    TaskIdentityInfo info = {};
    info.taskId = taskId;
    info.taskExpireTime = expireTime;
    info.executeType = ExecuteType::DELAYED;
    info.allowScreenLockExecution = false;
    info.allowableDuration = 0;
    return info;
}

// ==================== Parameter validation ====================

/**
 * @tc.name: RetrieveAuthTokenByTaskId_001
 * @tc.desc: Retrieve with non-existent taskId, should return TASK_IDENTITY_NOT_FOUND.
 * @tc.type: FUNC
 * @tc.result: SAF_ERR_TASK_IDENTITY_NOT_FOUND
 */
HWTEST_F(SafAgentFenceRetrieveAuthTokenTest, RetrieveAuthTokenByTaskId_001, TestSize.Level0)
{
    std::vector<int32_t> authToken;
    int32_t result = SafAgentFence::RetrieveAuthTokenByTaskId(g_testHapTokenId, "nonexistent_task", authToken);
    EXPECT_EQ(result, SAF_ERR_TASK_IDENTITY_NOT_FOUND);
}

/**
 * @tc.name: RetrieveAuthTokenByTaskId_002
 * @tc.desc: Retrieve with invalid callerToken (0), should return error.
 * @tc.type: FUNC
 * @tc.result: SAF_ERR_ARG_INVALID
 */
HWTEST_F(SafAgentFenceRetrieveAuthTokenTest, RetrieveAuthTokenByTaskId_002, TestSize.Level0)
{
    std::vector<int32_t> authToken;
    int32_t result = SafAgentFence::RetrieveAuthTokenByTaskId(0, "any_task", authToken);
    EXPECT_NE(result, SAF_SUCCESS);
}

/**
 * @tc.name: RetrieveAuthTokenByTaskId_003
 * @tc.desc: Retrieve with negative callerToken, should return error.
 * @tc.type: FUNC
 * @tc.result: SAF_ERR_ARG_INVALID
 */
HWTEST_F(SafAgentFenceRetrieveAuthTokenTest, RetrieveAuthTokenByTaskId_003, TestSize.Level0)
{
    std::vector<int32_t> authToken;
    int32_t result = SafAgentFence::RetrieveAuthTokenByTaskId(-1, "any_task", authToken);
    EXPECT_NE(result, SAF_SUCCESS);
}

/**
 * @tc.name: RetrieveAuthTokenByTaskId_004
 * @tc.desc: Retrieve with empty taskId, should return error.
 * @tc.type: FUNC
 * @tc.result: SAF_ERR_ARG_INVALID
 */
HWTEST_F(SafAgentFenceRetrieveAuthTokenTest, RetrieveAuthTokenByTaskId_004, TestSize.Level0)
{
    std::vector<int32_t> authToken;
    int32_t result = SafAgentFence::RetrieveAuthTokenByTaskId(g_testHapTokenId, "", authToken);
    EXPECT_NE(result, SAF_SUCCESS);
}

/**
 * @tc.name: RetrieveAuthTokenByTaskId_005
 * @tc.desc: Retrieve with path traversal taskId, should return error.
 * @tc.type: FUNC
 * @tc.result: SAF_ERR_ARG_INVALID
 */
HWTEST_F(SafAgentFenceRetrieveAuthTokenTest, RetrieveAuthTokenByTaskId_005, TestSize.Level0)
{
    std::vector<int32_t> authToken;
    int32_t result = SafAgentFence::RetrieveAuthTokenByTaskId(g_testHapTokenId, "../../../etc/passwd", authToken);
    EXPECT_NE(result, SAF_SUCCESS);
}

// ==================== Successful retrieval (low-risk) ====================

/**
 * @tc.name: RetrieveAuthTokenByTaskId_006
 * @tc.desc: Generate low-risk task identity, then retrieve authToken. Low-risk task returns empty authToken.
 * @tc.type: FUNC
 * @tc.result: SAF_SUCCESS
 */
HWTEST_F(SafAgentFenceRetrieveAuthTokenTest, RetrieveAuthTokenByTaskId_006, TestSize.Level0)
{
    TaskIdentityInfo info = MakeLowRiskInfo("retrieve_test_001", 3600);

    TaskIdentity identity;
    int32_t genResult = SafAgentFence::GenerateTaskIdentity(info, identity);
    EXPECT_EQ(genResult, SAF_SUCCESS);

    std::vector<int32_t> authToken;
    int32_t result = SafAgentFence::RetrieveAuthTokenByTaskId(g_testHapTokenId, "retrieve_test_001", authToken);
    EXPECT_EQ(result, SAF_SUCCESS);
    EXPECT_TRUE(authToken.empty());

    // Cleanup
    std::vector<std::string> taskIds = {"retrieve_test_001"};
    SafAgentFence::RevokeTaskIdentity(taskIds);
}

// ==================== Successful retrieval (delayed, never expires) ====================

/**
 * @tc.name: RetrieveAuthTokenByTaskId_007
 * @tc.desc: Generate DELAYED task with taskExpireTime=-1 (never expires), then retrieve.
 * @tc.type: FUNC
 * @tc.result: SAF_SUCCESS
 */
HWTEST_F(SafAgentFenceRetrieveAuthTokenTest, RetrieveAuthTokenByTaskId_007, TestSize.Level0)
{
    TaskIdentityInfo info = MakeDelayedInfo("delayed_never_expire", -1);

    TaskIdentity identity;
    int32_t genResult = SafAgentFence::GenerateTaskIdentity(info, identity);
    EXPECT_EQ(genResult, SAF_SUCCESS);

    std::vector<int32_t> authToken;
    int32_t result = SafAgentFence::RetrieveAuthTokenByTaskId(g_testHapTokenId, "delayed_never_expire", authToken);
    EXPECT_EQ(result, SAF_SUCCESS);

    // Cleanup
    std::vector<std::string> taskIds = {"delayed_never_expire"};
    SafAgentFence::RevokeTaskIdentity(taskIds);
}

// ==================== Revoked ====================

/**
 * @tc.name: RetrieveAuthTokenByTaskId_008
 * @tc.desc: Generate task, revoke it, then retrieve should return TASK_IDENTITY_NOT_FOUND.
 * @tc.type: FUNC
 * @tc.result: SAF_ERR_TASK_IDENTITY_NOT_FOUND
 */
HWTEST_F(SafAgentFenceRetrieveAuthTokenTest, RetrieveAuthTokenByTaskId_008, TestSize.Level0)
{
    TaskIdentityInfo info = MakeLowRiskInfo("revoke_test_001", 3600);

    TaskIdentity identity;
    EXPECT_EQ(SafAgentFence::GenerateTaskIdentity(info, identity), SAF_SUCCESS);

    std::vector<std::string> taskIds = {"revoke_test_001"};
    EXPECT_EQ(SafAgentFence::RevokeTaskIdentity(taskIds), SAF_SUCCESS);

    std::vector<int32_t> authToken;
    int32_t result = SafAgentFence::RetrieveAuthTokenByTaskId(g_testHapTokenId, "revoke_test_001", authToken);
    EXPECT_EQ(result, SAF_ERR_TASK_IDENTITY_NOT_FOUND);
}

// ==================== Revoked then regenerate ====================

/**
 * @tc.name: RetrieveAuthTokenByTaskId_009
 * @tc.desc: Generate task, revoke, then regenerate clears revoked status. Retrieve should succeed.
 * @tc.type: FUNC
 * @tc.result: SAF_SUCCESS
 */
HWTEST_F(SafAgentFenceRetrieveAuthTokenTest, RetrieveAuthTokenByTaskId_009, TestSize.Level0)
{
    TaskIdentityInfo info = MakeLowRiskInfo("regen_test_001", 3600);

    TaskIdentity identity;
    EXPECT_EQ(SafAgentFence::GenerateTaskIdentity(info, identity), SAF_SUCCESS);

    std::vector<std::string> taskIds = {"regen_test_001"};
    EXPECT_EQ(SafAgentFence::RevokeTaskIdentity(taskIds), SAF_SUCCESS);

    std::vector<int32_t> authToken;
    int32_t result = SafAgentFence::RetrieveAuthTokenByTaskId(g_testHapTokenId, "regen_test_001", authToken);
    EXPECT_EQ(result, SAF_ERR_TASK_IDENTITY_NOT_FOUND);

    // Regenerate
    EXPECT_EQ(SafAgentFence::GenerateTaskIdentity(info, identity), SAF_SUCCESS);
    result = SafAgentFence::RetrieveAuthTokenByTaskId(g_testHapTokenId, "regen_test_001", authToken);
    EXPECT_EQ(result, SAF_SUCCESS);

    // Cleanup
    SafAgentFence::RevokeTaskIdentity(taskIds);
}

// ==================== Expired ====================

/**
 * @tc.name: RetrieveAuthTokenByTaskId_010
 * @tc.desc: Generate task with 1-second expire time, wait 2 seconds, retrieve should return EXPIRED.
 * @tc.type: FUNC
 * @tc.result: SAF_ERR_TASK_IDENTITY_EXPIRED
 */
HWTEST_F(SafAgentFenceRetrieveAuthTokenTest, RetrieveAuthTokenByTaskId_010, TestSize.Level1)
{
    TaskIdentityInfo info = MakeLowRiskInfo("expire_test_001", 1);

    TaskIdentity identity;
    EXPECT_EQ(SafAgentFence::GenerateTaskIdentity(info, identity), SAF_SUCCESS);

    // Wait for expiration
    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::vector<int32_t> authToken;
    int32_t result = SafAgentFence::RetrieveAuthTokenByTaskId(g_testHapTokenId, "expire_test_001", authToken);
    EXPECT_EQ(result, SAF_ERR_TASK_IDENTITY_EXPIRED);

    // Cleanup
    std::vector<std::string> taskIds = {"expire_test_001"};
    SafAgentFence::RevokeTaskIdentity(taskIds);
}

// ==================== Cross caller isolation ====================

/**
 * @tc.name: RetrieveAuthTokenByTaskId_011
 * @tc.desc: Generate task with caller A, retrieve with caller B should return NOT_FOUND.
 * @tc.type: FUNC
 * @tc.result: SAF_ERR_TASK_IDENTITY_NOT_FOUND
 */
HWTEST_F(SafAgentFenceRetrieveAuthTokenTest, RetrieveAuthTokenByTaskId_011, TestSize.Level0)
{
    TaskIdentityInfo info = MakeLowRiskInfo("isolation_test_001", 3600);

    TaskIdentity identity;
    EXPECT_EQ(SafAgentFence::GenerateTaskIdentity(info, identity), SAF_SUCCESS);

    // Use a different caller token for retrieve
    int32_t wrongCallerToken = g_testHapTokenId + 1;
    std::vector<int32_t> authToken;
    int32_t result = SafAgentFence::RetrieveAuthTokenByTaskId(wrongCallerToken, "isolation_test_001", authToken);
    EXPECT_EQ(result, SAF_ERR_TASK_IDENTITY_NOT_FOUND);

    // Correct caller should succeed
    result = SafAgentFence::RetrieveAuthTokenByTaskId(g_testHapTokenId, "isolation_test_001", authToken);
    EXPECT_EQ(result, SAF_SUCCESS);

    // Cleanup
    std::vector<std::string> taskIds = {"isolation_test_001"};
    SafAgentFence::RevokeTaskIdentity(taskIds);
}

// ==================== Batch revoke and retrieve ====================

/**
 * @tc.name: RetrieveAuthTokenByTaskId_012
 * @tc.desc: Generate 3 tasks, batch revoke all, each retrieve returns NOT_FOUND.
 * @tc.type: FUNC
 * @tc.result: SAF_ERR_TASK_IDENTITY_NOT_FOUND
 */
HWTEST_F(SafAgentFenceRetrieveAuthTokenTest, RetrieveAuthTokenByTaskId_012, TestSize.Level0)
{
    TaskIdentity identity;

    EXPECT_EQ(SafAgentFence::GenerateTaskIdentity(MakeLowRiskInfo("batch_001", 3600), identity), SAF_SUCCESS);
    EXPECT_EQ(SafAgentFence::GenerateTaskIdentity(MakeLowRiskInfo("batch_002", 3600), identity), SAF_SUCCESS);
    EXPECT_EQ(SafAgentFence::GenerateTaskIdentity(MakeLowRiskInfo("batch_003", 3600), identity), SAF_SUCCESS);

    std::vector<std::string> taskIds = {"batch_001", "batch_002", "batch_003"};
    EXPECT_EQ(SafAgentFence::RevokeTaskIdentity(taskIds), SAF_SUCCESS);

    std::vector<int32_t> authToken;
    EXPECT_EQ(SafAgentFence::RetrieveAuthTokenByTaskId(g_testHapTokenId, "batch_001", authToken),
        SAF_ERR_TASK_IDENTITY_NOT_FOUND);
    EXPECT_EQ(SafAgentFence::RetrieveAuthTokenByTaskId(g_testHapTokenId, "batch_002", authToken),
        SAF_ERR_TASK_IDENTITY_NOT_FOUND);
    EXPECT_EQ(SafAgentFence::RetrieveAuthTokenByTaskId(g_testHapTokenId, "batch_003", authToken),
        SAF_ERR_TASK_IDENTITY_NOT_FOUND);
}

// ==================== GenerateTaskChallenge success ====================

/**
 * @tc.name: RetrieveAuthTokenByTaskId_013
 * @tc.desc: GenerateTaskChallenge returns 32-byte challenge.
 * @tc.type: FUNC
 * @tc.result: SAF_SUCCESS
 */
HWTEST_F(SafAgentFenceRetrieveAuthTokenTest, RetrieveAuthTokenByTaskId_013, TestSize.Level0)
{
    TaskIdentityInfo info = MakeLowRiskInfo("challenge_test_001", 3600);

    std::vector<int32_t> challenge;
    int32_t result = SafAgentFence::GenerateTaskChallenge(info, challenge);
    EXPECT_EQ(result, SAF_SUCCESS);
    EXPECT_EQ(challenge.size(), 32u);

    // Cleanup
    std::vector<std::string> taskIds = {"challenge_test_001"};
    SafAgentFence::RevokeTaskIdentity(taskIds);
}

// ==================== TaskIdentity output validation ====================

/**
 * @tc.name: RetrieveAuthTokenByTaskId_014
 * @tc.desc: GenerateTaskIdentity returns valid TaskIdentity with non-empty fields.
 * @tc.type: FUNC
 * @tc.result: SAF_SUCCESS
 */
HWTEST_F(SafAgentFenceRetrieveAuthTokenTest, RetrieveAuthTokenByTaskId_014, TestSize.Level0)
{
    TaskIdentityInfo info = MakeLowRiskInfo("identity_test_001", 3600);

    TaskIdentity identity;
    int32_t result = SafAgentFence::GenerateTaskIdentity(info, identity);
    EXPECT_EQ(result, SAF_SUCCESS);
    EXPECT_FALSE(identity.taskAttributes.empty());
    EXPECT_FALSE(identity.identity.empty());
    EXPECT_FALSE(identity.challenge.empty());

    // Cleanup
    std::vector<std::string> taskIds = {"identity_test_001"};
    SafAgentFence::RevokeTaskIdentity(taskIds);
}

// ==================== Idempotent revoke ====================

/**
 * @tc.name: RetrieveAuthTokenByTaskId_015
 * @tc.desc: Revoke a non-existent taskId should succeed (idempotent).
 * @tc.type: FUNC
 * @tc.result: SAF_SUCCESS
 */
HWTEST_F(SafAgentFenceRetrieveAuthTokenTest, RetrieveAuthTokenByTaskId_015, TestSize.Level0)
{
    std::vector<std::string> taskIds = {"nonexistent_for_revoke"};
    int32_t result = SafAgentFence::RevokeTaskIdentity(taskIds);
    EXPECT_EQ(result, SAF_SUCCESS);
}
