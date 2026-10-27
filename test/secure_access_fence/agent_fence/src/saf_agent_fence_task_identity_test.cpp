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
#include "ipc_skeleton.h"

using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::Security::SAF;
using namespace OHOS::Security::AccessToken;

// Holds the system HAP token (userID=100) allocated in GrantTestPermission.
// Used as the callerToken parameter of RetrieveAuthTokenByTaskId; the same
// token is also set as the calling identity via SetSelfTokenID so that
// GenerateTaskChallenge/GenerateTaskIdentity resolve os_account_id=100.
static int32_t g_testHapTokenId = 0;
// Full 64-bit HAP token ID for SetSelfTokenID (high bits carry token type info).
static uint64_t g_testHapTokenIdFull = 0;
// Native token (TOKEN_NATIVE) for RetrieveAuthTokenByTaskId calls, which requires
// CheckIsSystemSa(). The HAP token fails that check, so we switch to this native
// token before RetrieveAuthTokenByTaskId and switch back to HAP for Generate/Revoke.
static uint64_t g_testNativeTokenIdFull = 0;

static void SwitchToRetrieveCaller()
{
    SetSelfTokenID(g_testNativeTokenIdFull);
}

static void SwitchToGenerateCaller()
{
    SetSelfTokenID(g_testHapTokenIdFull);
}

class SafAgentFenceTaskIdentityTest : public testing::Test {
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
    // and restarts the access_token service). Save this native token — it has
    // QUERY_TOOL_PERMISSIONS + INTERACT_ACROSS_LOCAL_ACCOUNTS and is TOKEN_NATIVE,
    // which RetrieveAuthTokenByTaskId's CheckIsSystemSa() requires.
    GrantSelfPermission();
    g_testNativeTokenIdFull = OHOS::IPCSkeleton::GetCallingTokenID();

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
        .bundleName = "saf_task_identity_test",
        .instIndex = 0,
        .appIDDesc = "saf_task_identity_test",
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
        .domain = "saf_task_identity_test",
        .permStateList = {permState},
    };
    AccessTokenIDEx tokenIdEx = {0};
    ASSERT_EQ(RET_SUCCESS, AccessTokenKit::InitHapToken(info, policy, tokenIdEx));
    ASSERT_NE(INVALID_TOKENID, tokenIdEx.tokenIdExStruct.tokenID);
    g_testHapTokenId = static_cast<int32_t>(tokenIdEx.tokenIdExStruct.tokenID);
    g_testHapTokenIdFull = tokenIdEx.tokenIDEx;
    SetSelfTokenID(tokenIdEx.tokenIDEx);
}

void SafAgentFenceTaskIdentityTest::SetUpTestCase(void)
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

// ==================== GenerateTaskChallenge ====================

/**
 * @tc.name: GenerateTaskChallenge_001
 * @tc.desc: Generate challenge for INSTANT low-risk task, should return 32-byte challenge.
 * @tc.type: FUNC
 * @tc.result: SAF_SUCCESS
 */
HWTEST_F(SafAgentFenceTaskIdentityTest, GenerateTaskChallenge_001, TestSize.Level0)
{
    TaskIdentityInfo info = MakeLowRiskInfo("task_challenge_001", 3600);

    std::vector<int32_t> challenge;
    int32_t result = SafAgentFence::GenerateTaskChallenge(info, challenge);
    EXPECT_EQ(result, SAF_SUCCESS);
    EXPECT_EQ(challenge.size(), 32u);

    // Cleanup
    std::vector<std::string> taskIds = {"task_challenge_001"};
    SafAgentFence::RevokeTaskIdentity(taskIds);
}

/**
 * @tc.name: GenerateTaskChallenge_002
 * @tc.desc: Generate challenge for DELAYED never-expire task, should return 32-byte challenge.
 * @tc.type: FUNC
 * @tc.result: SAF_SUCCESS
 */
HWTEST_F(SafAgentFenceTaskIdentityTest, GenerateTaskChallenge_002, TestSize.Level0)
{
    TaskIdentityInfo info = MakeDelayedInfo("task_challenge_002", -1);

    std::vector<int32_t> challenge;
    int32_t result = SafAgentFence::GenerateTaskChallenge(info, challenge);
    EXPECT_EQ(result, SAF_SUCCESS);
    EXPECT_EQ(challenge.size(), 32u);

    // Cleanup
    std::vector<std::string> taskIds = {"task_challenge_002"};
    SafAgentFence::RevokeTaskIdentity(taskIds);
}

/**
 * @tc.name: GenerateTaskChallenge_003
 * @tc.desc: Generate challenge twice for the same taskId, both should succeed with 32-byte challenge.
 * @tc.type: FUNC
 * @tc.result: SAF_SUCCESS
 */
HWTEST_F(SafAgentFenceTaskIdentityTest, GenerateTaskChallenge_003, TestSize.Level0)
{
    TaskIdentityInfo info = MakeLowRiskInfo("task_challenge_003", 3600);

    std::vector<int32_t> challenge1;
    int32_t result1 = SafAgentFence::GenerateTaskChallenge(info, challenge1);
    EXPECT_EQ(result1, SAF_SUCCESS);
    EXPECT_EQ(challenge1.size(), 32u);

    std::vector<int32_t> challenge2;
    int32_t result2 = SafAgentFence::GenerateTaskChallenge(info, challenge2);
    EXPECT_EQ(result2, SAF_SUCCESS);
    EXPECT_EQ(challenge2.size(), 32u);

    // Cleanup
    std::vector<std::string> taskIds = {"task_challenge_003"};
    SafAgentFence::RevokeTaskIdentity(taskIds);
}

/**
 * @tc.name: GenerateTaskChallenge_004
 * @tc.desc: Generate challenge for never-expire DELAYED task (taskExpireTime=-1), should succeed.
 * @tc.type: FUNC
 * @tc.result: SAF_SUCCESS
 */
HWTEST_F(SafAgentFenceTaskIdentityTest, GenerateTaskChallenge_004, TestSize.Level0)
{
    TaskIdentityInfo info = MakeDelayedInfo("task_challenge_004", -1);

    std::vector<int32_t> challenge;
    int32_t result = SafAgentFence::GenerateTaskChallenge(info, challenge);
    EXPECT_EQ(result, SAF_SUCCESS);
    EXPECT_EQ(challenge.size(), 32u);

    // Cleanup
    std::vector<std::string> taskIds = {"task_challenge_004"};
    SafAgentFence::RevokeTaskIdentity(taskIds);
}

// ==================== GenerateTaskIdentity ====================

/**
 * @tc.name: GenerateTaskIdentity_001
 * @tc.desc: Generate identity for INSTANT low-risk task, TaskIdentity fields should be non-empty.
 * @tc.type: FUNC
 * @tc.result: SAF_SUCCESS
 */
HWTEST_F(SafAgentFenceTaskIdentityTest, GenerateTaskIdentity_001, TestSize.Level0)
{
    TaskIdentityInfo info = MakeLowRiskInfo("task_identity_001", 3600);

    TaskIdentity identity;
    int32_t result = SafAgentFence::GenerateTaskIdentity(info, identity);
    EXPECT_EQ(result, SAF_SUCCESS);
    EXPECT_FALSE(identity.taskAttributes.empty());
    EXPECT_FALSE(identity.identity.empty());
    EXPECT_FALSE(identity.challenge.empty());

    // Cleanup
    std::vector<std::string> taskIds = {"task_identity_001"};
    SafAgentFence::RevokeTaskIdentity(taskIds);
}

/**
 * @tc.name: GenerateTaskIdentity_002
 * @tc.desc: Generate identity for DELAYED never-expire task, TaskIdentity fields should be non-empty.
 * @tc.type: FUNC
 * @tc.result: SAF_SUCCESS
 */
HWTEST_F(SafAgentFenceTaskIdentityTest, GenerateTaskIdentity_002, TestSize.Level0)
{
    TaskIdentityInfo info = MakeDelayedInfo("task_identity_002", -1);

    TaskIdentity identity;
    int32_t result = SafAgentFence::GenerateTaskIdentity(info, identity);
    EXPECT_EQ(result, SAF_SUCCESS);
    EXPECT_FALSE(identity.taskAttributes.empty());
    EXPECT_FALSE(identity.identity.empty());
    EXPECT_FALSE(identity.challenge.empty());

    // Cleanup
    std::vector<std::string> taskIds = {"task_identity_002"};
    SafAgentFence::RevokeTaskIdentity(taskIds);
}

/**
 * @tc.name: GenerateTaskIdentity_003
 * @tc.desc: Generate identity twice for the same taskId (regenerate), both should succeed.
 * @tc.type: FUNC
 * @tc.result: SAF_SUCCESS
 */
HWTEST_F(SafAgentFenceTaskIdentityTest, GenerateTaskIdentity_003, TestSize.Level0)
{
    TaskIdentityInfo info = MakeLowRiskInfo("task_identity_003", 3600);

    TaskIdentity identity1;
    int32_t result1 = SafAgentFence::GenerateTaskIdentity(info, identity1);
    EXPECT_EQ(result1, SAF_SUCCESS);
    EXPECT_FALSE(identity1.identity.empty());

    TaskIdentity identity2;
    int32_t result2 = SafAgentFence::GenerateTaskIdentity(info, identity2);
    EXPECT_EQ(result2, SAF_SUCCESS);
    EXPECT_FALSE(identity2.identity.empty());

    // Cleanup
    std::vector<std::string> taskIds = {"task_identity_003"};
    SafAgentFence::RevokeTaskIdentity(taskIds);
}

/**
 * @tc.name: GenerateTaskIdentity_004
 * @tc.desc: Generate identity for never-expire DELAYED task (taskExpireTime=-1), should succeed.
 * @tc.type: FUNC
 * @tc.result: SAF_SUCCESS
 */
HWTEST_F(SafAgentFenceTaskIdentityTest, GenerateTaskIdentity_004, TestSize.Level0)
{
    TaskIdentityInfo info = MakeDelayedInfo("task_identity_004", -1);

    TaskIdentity identity;
    int32_t result = SafAgentFence::GenerateTaskIdentity(info, identity);
    EXPECT_EQ(result, SAF_SUCCESS);
    EXPECT_FALSE(identity.identity.empty());

    // Cleanup
    std::vector<std::string> taskIds = {"task_identity_004"};
    SafAgentFence::RevokeTaskIdentity(taskIds);
}

// ==================== RevokeTaskIdentity ====================

/**
 * @tc.name: RevokeTaskIdentity_001
 * @tc.desc: Revoke a single existing taskId should succeed, and retrieve returns NOT_FOUND.
 * @tc.type: FUNC
 * @tc.result: SAF_SUCCESS
 */
HWTEST_F(SafAgentFenceTaskIdentityTest, RevokeTaskIdentity_001, TestSize.Level0)
{
    TaskIdentityInfo info = MakeLowRiskInfo("task_revoke_001", 3600);

    TaskIdentity identity;
    EXPECT_EQ(SafAgentFence::GenerateTaskIdentity(info, identity), SAF_SUCCESS);

    std::vector<std::string> taskIds = {"task_revoke_001"};
    int32_t result = SafAgentFence::RevokeTaskIdentity(taskIds);
    EXPECT_EQ(result, SAF_SUCCESS);

    std::vector<int32_t> authToken;
    SwitchToRetrieveCaller();
    EXPECT_EQ(SafAgentFence::RetrieveAuthTokenByTaskId(g_testHapTokenId, "task_revoke_001", authToken),
        SAF_ERR_TASK_IDENTITY_NOT_FOUND);
    SwitchToGenerateCaller();
}

/**
 * @tc.name: RevokeTaskIdentity_002
 * @tc.desc: Batch revoke multiple existing taskIds should succeed, all retrieve return NOT_FOUND.
 * @tc.type: FUNC
 * @tc.result: SAF_SUCCESS
 */
HWTEST_F(SafAgentFenceTaskIdentityTest, RevokeTaskIdentity_002, TestSize.Level0)
{
    TaskIdentity identity;
    EXPECT_EQ(SafAgentFence::GenerateTaskIdentity(MakeLowRiskInfo("task_revoke_002a", 3600), identity), SAF_SUCCESS);
    EXPECT_EQ(SafAgentFence::GenerateTaskIdentity(MakeLowRiskInfo("task_revoke_002b", 3600), identity), SAF_SUCCESS);
    EXPECT_EQ(SafAgentFence::GenerateTaskIdentity(MakeLowRiskInfo("task_revoke_002c", 3600), identity), SAF_SUCCESS);

    std::vector<std::string> taskIds = {"task_revoke_002a", "task_revoke_002b", "task_revoke_002c"};
    int32_t result = SafAgentFence::RevokeTaskIdentity(taskIds);
    EXPECT_EQ(result, SAF_SUCCESS);

    std::vector<int32_t> authToken;
    SwitchToRetrieveCaller();
    EXPECT_EQ(SafAgentFence::RetrieveAuthTokenByTaskId(g_testHapTokenId, "task_revoke_002a", authToken),
        SAF_ERR_TASK_IDENTITY_NOT_FOUND);
    EXPECT_EQ(SafAgentFence::RetrieveAuthTokenByTaskId(g_testHapTokenId, "task_revoke_002b", authToken),
        SAF_ERR_TASK_IDENTITY_NOT_FOUND);
    EXPECT_EQ(SafAgentFence::RetrieveAuthTokenByTaskId(g_testHapTokenId, "task_revoke_002c", authToken),
        SAF_ERR_TASK_IDENTITY_NOT_FOUND);
    SwitchToGenerateCaller();
}

/**
 * @tc.name: RevokeTaskIdentity_003
 * @tc.desc: Revoke a non-existent taskId should succeed (idempotent).
 * @tc.type: FUNC
 * @tc.result: SAF_SUCCESS
 */
HWTEST_F(SafAgentFenceTaskIdentityTest, RevokeTaskIdentity_003, TestSize.Level0)
{
    std::vector<std::string> taskIds = {"task_revoke_nonexistent_003"};
    int32_t result = SafAgentFence::RevokeTaskIdentity(taskIds);
    EXPECT_EQ(result, SAF_SUCCESS);
}

/**
 * @tc.name: RevokeTaskIdentity_004
 * @tc.desc: Revoke an already-revoked taskId should still succeed (idempotent).
 * @tc.type: FUNC
 * @tc.result: SAF_SUCCESS
 */
HWTEST_F(SafAgentFenceTaskIdentityTest, RevokeTaskIdentity_004, TestSize.Level0)
{
    TaskIdentityInfo info = MakeLowRiskInfo("task_revoke_twice_004", 3600);

    TaskIdentity identity;
    EXPECT_EQ(SafAgentFence::GenerateTaskIdentity(info, identity), SAF_SUCCESS);

    std::vector<std::string> taskIds = {"task_revoke_twice_004"};
    EXPECT_EQ(SafAgentFence::RevokeTaskIdentity(taskIds), SAF_SUCCESS);
    // Revoke the same taskId again; should still succeed (idempotent)
    EXPECT_EQ(SafAgentFence::RevokeTaskIdentity(taskIds), SAF_SUCCESS);
}

/**
 * @tc.name: RevokeTaskIdentity_005
 * @tc.desc: Revoke a mix of existing and non-existing taskIds should succeed.
 * @tc.type: FUNC
 * @tc.result: SAF_SUCCESS
 */
HWTEST_F(SafAgentFenceTaskIdentityTest, RevokeTaskIdentity_005, TestSize.Level0)
{
    TaskIdentityInfo info = MakeLowRiskInfo("task_revoke_mixed_005", 3600);

    TaskIdentity identity;
    EXPECT_EQ(SafAgentFence::GenerateTaskIdentity(info, identity), SAF_SUCCESS);

    // Mixed vector: one existing task + one non-existing task
    std::vector<std::string> taskIds = {"task_revoke_mixed_005", "task_revoke_mixed_nonexistent_005"};
    int32_t result = SafAgentFence::RevokeTaskIdentity(taskIds);
    EXPECT_EQ(result, SAF_SUCCESS);

    // Verify the existing one is gone
    std::vector<int32_t> authToken;
    SwitchToRetrieveCaller();
    EXPECT_EQ(SafAgentFence::RetrieveAuthTokenByTaskId(g_testHapTokenId, "task_revoke_mixed_005", authToken),
        SAF_ERR_TASK_IDENTITY_NOT_FOUND);
    SwitchToGenerateCaller();
}

// ==================== RetrieveAuthTokenByTaskId (lifecycle) ====================

/**
 * @tc.name: RetrieveAuthTokenByTaskId_001
 * @tc.desc: Generate low-risk task then retrieve; low-risk task returns empty authToken.
 * @tc.type: FUNC
 * @tc.result: SAF_SUCCESS
 */
HWTEST_F(SafAgentFenceTaskIdentityTest, RetrieveAuthTokenByTaskId_001, TestSize.Level0)
{
    TaskIdentityInfo info = MakeLowRiskInfo("task_retrieve_001", 3600);

    TaskIdentity identity;
    EXPECT_EQ(SafAgentFence::GenerateTaskIdentity(info, identity), SAF_SUCCESS);

    std::vector<int32_t> authToken;
    SwitchToRetrieveCaller();
    int32_t result = SafAgentFence::RetrieveAuthTokenByTaskId(g_testHapTokenId, "task_retrieve_001", authToken);
    EXPECT_EQ(result, SAF_SUCCESS);
    EXPECT_TRUE(authToken.empty());
    SwitchToGenerateCaller();

    // Cleanup
    std::vector<std::string> taskIds = {"task_retrieve_001"};
    SafAgentFence::RevokeTaskIdentity(taskIds);
}

/**
 * @tc.name: RetrieveAuthTokenByTaskId_002
 * @tc.desc: Generate DELAYED never-expire task then retrieve; should succeed.
 * @tc.type: FUNC
 * @tc.result: SAF_SUCCESS
 */
HWTEST_F(SafAgentFenceTaskIdentityTest, RetrieveAuthTokenByTaskId_002, TestSize.Level0)
{
    TaskIdentityInfo info = MakeDelayedInfo("task_retrieve_002", -1);

    TaskIdentity identity;
    EXPECT_EQ(SafAgentFence::GenerateTaskIdentity(info, identity), SAF_SUCCESS);

    std::vector<int32_t> authToken;
    SwitchToRetrieveCaller();
    int32_t result = SafAgentFence::RetrieveAuthTokenByTaskId(g_testHapTokenId, "task_retrieve_002", authToken);
    EXPECT_EQ(result, SAF_SUCCESS);
    SwitchToGenerateCaller();

    // Cleanup
    std::vector<std::string> taskIds = {"task_retrieve_002"};
    SafAgentFence::RevokeTaskIdentity(taskIds);
}

/**
 * @tc.name: RetrieveAuthTokenByTaskId_003
 * @tc.desc: Generate task, revoke it, then retrieve should return NOT_FOUND.
 * @tc.type: FUNC
 * @tc.result: SAF_ERR_TASK_IDENTITY_NOT_FOUND
 */
HWTEST_F(SafAgentFenceTaskIdentityTest, RetrieveAuthTokenByTaskId_003, TestSize.Level0)
{
    TaskIdentityInfo info = MakeLowRiskInfo("task_retrieve_003", 3600);

    TaskIdentity identity;
    EXPECT_EQ(SafAgentFence::GenerateTaskIdentity(info, identity), SAF_SUCCESS);

    std::vector<std::string> taskIds = {"task_retrieve_003"};
    EXPECT_EQ(SafAgentFence::RevokeTaskIdentity(taskIds), SAF_SUCCESS);

    std::vector<int32_t> authToken;
    SwitchToRetrieveCaller();
    int32_t result = SafAgentFence::RetrieveAuthTokenByTaskId(g_testHapTokenId, "task_retrieve_003", authToken);
    EXPECT_EQ(result, SAF_ERR_TASK_IDENTITY_NOT_FOUND);
    SwitchToGenerateCaller();
}

/**
 * @tc.name: RetrieveAuthTokenByTaskId_004
 * @tc.desc: Generate task with caller A, retrieve with caller B should return NOT_FOUND, A succeeds.
 * @tc.type: FUNC
 * @tc.result: SAF_ERR_TASK_IDENTITY_NOT_FOUND
 */
HWTEST_F(SafAgentFenceTaskIdentityTest, RetrieveAuthTokenByTaskId_004, TestSize.Level0)
{
    TaskIdentityInfo info = MakeLowRiskInfo("task_retrieve_004", 3600);

    TaskIdentity identity;
    EXPECT_EQ(SafAgentFence::GenerateTaskIdentity(info, identity), SAF_SUCCESS);

    // Use a different caller token for retrieve
    int32_t wrongCallerToken = g_testHapTokenId + 1;
    std::vector<int32_t> authToken;
    SwitchToRetrieveCaller();
    int32_t result = SafAgentFence::RetrieveAuthTokenByTaskId(wrongCallerToken, "task_retrieve_004", authToken);
    EXPECT_EQ(result, SAF_ERR_TASK_IDENTITY_NOT_FOUND);

    // Correct caller should succeed
    result = SafAgentFence::RetrieveAuthTokenByTaskId(g_testHapTokenId, "task_retrieve_004", authToken);
    EXPECT_EQ(result, SAF_SUCCESS);
    SwitchToGenerateCaller();

    // Cleanup
    std::vector<std::string> taskIds = {"task_retrieve_004"};
    SafAgentFence::RevokeTaskIdentity(taskIds);
}

/**
 * @tc.name: RetrieveAuthTokenByTaskId_005
 * @tc.desc: Generate task with 1-second expire time, wait 2 seconds, retrieve should return EXPIRED.
 * @tc.type: FUNC
 * @tc.result: SAF_ERR_TASK_IDENTITY_EXPIRED
 */
HWTEST_F(SafAgentFenceTaskIdentityTest, RetrieveAuthTokenByTaskId_005, TestSize.Level1)
{
    TaskIdentityInfo info = MakeLowRiskInfo("task_retrieve_005", 1);

    TaskIdentity identity;
    EXPECT_EQ(SafAgentFence::GenerateTaskIdentity(info, identity), SAF_SUCCESS);

    // Wait for expiration
    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::vector<int32_t> authToken;
    SwitchToRetrieveCaller();
    int32_t result = SafAgentFence::RetrieveAuthTokenByTaskId(g_testHapTokenId, "task_retrieve_005", authToken);
    EXPECT_EQ(result, SAF_ERR_TASK_IDENTITY_EXPIRED);
    SwitchToGenerateCaller();

    // Cleanup
    std::vector<std::string> taskIds = {"task_retrieve_005"};
    SafAgentFence::RevokeTaskIdentity(taskIds);
}
