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

// Full 64-bit HAP token ID for SetSelfTokenID (high bits carry token type info).
static uint64_t g_testHapTokenIdFull = 0;
// The task owner's uid recorded by the SA at generate time (IPCSkeleton::GetCallingUid
// of this test process). RetrieveAuthTokenByTaskId takes this uid directly as input.
static uint32_t g_testUid = 0;
// Native token (TOKEN_NATIVE) for RetrieveAuthTokenByTaskId calls, which requires
// CheckIsSystemSa(). The HAP token fails that check, so we switch to this native
// token before RetrieveAuthTokenByTaskId and switch back to HAP for Generate/Revoke.
static uint64_t g_testNativeTokenIdFull = 0;
// Synthetic caller uid inside os account 100 (100 * 200000 + 101). The xdevice
// test binary runs as root (uid 0), but the SA rejects uid==0 on retrieve and
// keys records by the binder calling uid, so the process drops to this uid in
// GrantTestPermission after the root-dependent token setup is done.
static constexpr uint32_t TEST_CALLER_UID = 20000101;
// Set when GrantTestPermission has dropped the root identity. Token switches
// and the final XML report need the root identity restored temporarily.
static bool g_uidDropped = false;

// SetSelfTokenID (write to /proc/self/tokenid) is rejected once the process
// has dropped root, so restore the saved root identity for the switch and
// drop back to the test caller uid afterwards. The kernel tokenid survives
// uid changes, so SAF IPC keeps running under the test caller uid.
static void SetCallerToken(uint64_t token)
{
    if (g_uidDropped) {
        setresuid(0, 0, 0);
    }
    SetSelfTokenID(token);
    if (g_uidDropped) {
        setresuid(TEST_CALLER_UID, TEST_CALLER_UID, 0);
    }
}

static void SwitchToRetrieveCaller()
{
    SetCallerToken(g_testNativeTokenIdFull);
}

static void SwitchToGenerateCaller()
{
    SetCallerToken(g_testHapTokenIdFull);
}

class SafAgentFenceRetrieveAuthTokenTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void)
    {
        // Restore root so gtest can write its XML report at the end of the
        // run (the xdevice output path is root-owned and a dropped uid makes
        // OpenFileForWriting abort).
        if (g_uidDropped) {
            setresuid(0, 0, 0);
            g_uidDropped = false;
        }
    }
    void SetUp(void) {}
    void TearDown(void) {}
};

// Creates a system HAP token with userID=100 (osAccountId=100) and grants
// ohos.permission.QUERY_TOOL_PERMISSIONS, then switches the calling identity to
// it. The service's check_query_tool_permission requires both CheckPermission
// (QUERY_TOOL_PERMISSIONS) and CheckIsSystemHap(). Native tokens fail
// CheckIsSystemHap, so a HAP token is required for GenerateTaskChallenge /
// GenerateTaskIdentity to succeed. The os_account_id used by the SA is derived
// from the calling uid, not from this token.
static void GrantTestPermission()
{
    // Step 1: native token with SAF permissions (gives privilege to allocate HAP tokens
    // and restarts the access_token service). Save this native token �?it has
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
    g_testHapTokenIdFull = tokenIdEx.tokenIDEx;
    SetSelfTokenID(tokenIdEx.tokenIDEx);

    // Drop the root identity AFTER all token allocation is done: the SA keys
    // records by the binder calling uid and rejects uid==0 (root), so a root
    // test process can never generate/retrieve successfully. Keep saved-uid 0
    // so setresuid(0, 0, 0) can restore the root identity if ever needed.
    if (geteuid() == 0 && setresuid(TEST_CALLER_UID, TEST_CALLER_UID, 0) == 0) {
        g_uidDropped = true;
    } else {
        GTEST_LOG_(WARNING) << "setresuid to test caller uid failed";
    }
    // The SA records the IPC caller's uid (this process) at generate time;
    // retrieve must pass the same uid.
    g_testUid = static_cast<uint32_t>(getuid());
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

static TaskIdentityInfo MakeHighRiskInfo(const std::string &taskId, int32_t expireTime,
    const std::vector<int32_t> &authToken)
{
    TaskIdentityInfo info = {};
    info.taskId = taskId;
    info.taskExpireTime = expireTime;
    info.executeType = ExecuteType::INSTANT;
    info.allowScreenLockExecution = false;
    info.allowableDuration = 0;
    info.authToken = authToken;
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
    std::vector<uint8_t> authToken;
    SwitchToRetrieveCaller();
    int32_t result = SafAgentFence::RetrieveAuthTokenByTaskId(g_testUid, "nonexistent_task", authToken);
    EXPECT_EQ(result, SAF_ERR_TASK_IDENTITY_NOT_FOUND);
    SwitchToGenerateCaller();
}

/**
 * @tc.name: RetrieveAuthTokenByTaskId_002
 * @tc.desc: Retrieve with invalid uid (0), should return error.
 * @tc.type: FUNC
 * @tc.result: SAF_ERR_ARG_INVALID
 */
HWTEST_F(SafAgentFenceRetrieveAuthTokenTest, RetrieveAuthTokenByTaskId_002, TestSize.Level0)
{
    std::vector<uint8_t> authToken;
    int32_t result = SafAgentFence::RetrieveAuthTokenByTaskId(0, "any_task", authToken);
    EXPECT_NE(result, SAF_SUCCESS);
}

/**
 * @tc.name: RetrieveAuthTokenByTaskId_003
 * @tc.desc: Retrieve with a uid that owns no tasks, should return error.
 * @tc.type: FUNC
 * @tc.result: SAF_ERR_TASK_IDENTITY_NOT_FOUND
 */
HWTEST_F(SafAgentFenceRetrieveAuthTokenTest, RetrieveAuthTokenByTaskId_003, TestSize.Level0)
{
    std::vector<uint8_t> authToken;
    int32_t result = SafAgentFence::RetrieveAuthTokenByTaskId(g_testUid + 999999, "any_task", authToken);
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
    std::vector<uint8_t> authToken;
    int32_t result = SafAgentFence::RetrieveAuthTokenByTaskId(g_testUid, "", authToken);
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
    std::vector<uint8_t> authToken;
    int32_t result = SafAgentFence::RetrieveAuthTokenByTaskId(g_testUid, "../../../etc/passwd", authToken);
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

    std::vector<uint8_t> authToken;
    SwitchToRetrieveCaller();
    int32_t result = SafAgentFence::RetrieveAuthTokenByTaskId(g_testUid, "retrieve_test_001", authToken);
    EXPECT_EQ(result, SAF_SUCCESS);
    EXPECT_TRUE(authToken.empty());
    SwitchToGenerateCaller();

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

    std::vector<uint8_t> authToken;
    SwitchToRetrieveCaller();
    int32_t result = SafAgentFence::RetrieveAuthTokenByTaskId(g_testUid, "delayed_never_expire", authToken);
    EXPECT_EQ(result, SAF_SUCCESS);
    SwitchToGenerateCaller();

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

    std::vector<uint8_t> authToken;
    SwitchToRetrieveCaller();
    int32_t result = SafAgentFence::RetrieveAuthTokenByTaskId(g_testUid, "revoke_test_001", authToken);
    EXPECT_EQ(result, SAF_ERR_TASK_IDENTITY_NOT_FOUND);
    SwitchToGenerateCaller();
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

    std::vector<uint8_t> authToken;
    SwitchToRetrieveCaller();
    int32_t result = SafAgentFence::RetrieveAuthTokenByTaskId(g_testUid, "regen_test_001", authToken);
    EXPECT_EQ(result, SAF_ERR_TASK_IDENTITY_NOT_FOUND);
    SwitchToGenerateCaller();

    // Regenerate
    EXPECT_EQ(SafAgentFence::GenerateTaskIdentity(info, identity), SAF_SUCCESS);
    SwitchToRetrieveCaller();
    result = SafAgentFence::RetrieveAuthTokenByTaskId(g_testUid, "regen_test_001", authToken);
    EXPECT_EQ(result, SAF_SUCCESS);
    SwitchToGenerateCaller();

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

    std::vector<uint8_t> authToken;
    SwitchToRetrieveCaller();
    int32_t result = SafAgentFence::RetrieveAuthTokenByTaskId(g_testUid, "expire_test_001", authToken);
    EXPECT_EQ(result, SAF_ERR_TASK_IDENTITY_EXPIRED);
    SwitchToGenerateCaller();

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

    // Use a different uid for retrieve (isolation: records are keyed by uid)
    uint32_t wrongUid = g_testUid + 1;
    std::vector<uint8_t> authToken;
    SwitchToRetrieveCaller();
    int32_t result = SafAgentFence::RetrieveAuthTokenByTaskId(wrongUid, "isolation_test_001", authToken);
    EXPECT_EQ(result, SAF_ERR_TASK_IDENTITY_NOT_FOUND);

    // Correct caller should succeed
    result = SafAgentFence::RetrieveAuthTokenByTaskId(g_testUid, "isolation_test_001", authToken);
    EXPECT_EQ(result, SAF_SUCCESS);
    SwitchToGenerateCaller();

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

    std::vector<uint8_t> authToken;
    SwitchToRetrieveCaller();
    EXPECT_EQ(SafAgentFence::RetrieveAuthTokenByTaskId(g_testUid, "batch_001", authToken),
        SAF_ERR_TASK_IDENTITY_NOT_FOUND);
    EXPECT_EQ(SafAgentFence::RetrieveAuthTokenByTaskId(g_testUid, "batch_002", authToken),
        SAF_ERR_TASK_IDENTITY_NOT_FOUND);
    EXPECT_EQ(SafAgentFence::RetrieveAuthTokenByTaskId(g_testUid, "batch_003", authToken),
        SAF_ERR_TASK_IDENTITY_NOT_FOUND);
    SwitchToGenerateCaller();
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

// ==================== Scheduled auth token redemption ====================

/**
 * @tc.name: RetrieveAuthTokenByTaskId_016
 * @tc.desc: INSTANT high-risk task with authToken requires generateTaskChallenge first.
 *           The fake authToken is rejected by the real IAM service, so generation
 *           cannot succeed on a real device. Without a pending challenge (and with
 *           a valid token), generateTaskIdentity would return TASK_IDENTITY_NOT_FOUND.
 *           (DELAYED tasks no longer consult the stored pending challenge — see 023/025.)
 * @tc.type: FUNC
 * @tc.result: not SAF_SUCCESS
 */
HWTEST_F(SafAgentFenceRetrieveAuthTokenTest, RetrieveAuthTokenByTaskId_016, TestSize.Level0)
{
    std::vector<int32_t> authToken = {0x48, 0x65, 0x6C, 0x6C, 0x6F};
    TaskIdentityInfo info = MakeHighRiskInfo("high_risk_no_challenge", 3600, authToken);

    TaskIdentity identity;
    int32_t genResult = SafAgentFence::GenerateTaskIdentity(info, identity);
    // A fake auth token is rejected by the real IAM service; generation must
    // not succeed in the test environment.
    EXPECT_NE(genResult, SAF_SUCCESS);

    // Cleanup
    std::vector<std::string> taskIds = {"high_risk_no_challenge"};
    SafAgentFence::RevokeTaskIdentity(taskIds);
}

/**
 * @tc.name: RetrieveAuthTokenByTaskId_017
 * @tc.desc: High-risk task with challenge + authToken: generateTaskChallenge succeeds.
 *           Generation verifies the authToken via the real IAM service; the fake
 *           test token is rejected there, so the result is tolerated (never
 *           TASK_IDENTITY_NOT_FOUND — see 023/025 for the tolerance pattern).
 * @tc.type: FUNC
 * @tc.result: SAF_SUCCESS or IAM error (not SAF_ERR_TASK_IDENTITY_NOT_FOUND)
 */
HWTEST_F(SafAgentFenceRetrieveAuthTokenTest, RetrieveAuthTokenByTaskId_017, TestSize.Level0)
{
    std::vector<int32_t> authToken = {0x48, 0x65, 0x6C, 0x6C, 0x6F};
    TaskIdentityInfo info = MakeHighRiskInfo("high_risk_challenge", 3600, authToken);

    std::vector<int32_t> challenge;
    int32_t chResult = SafAgentFence::GenerateTaskChallenge(info, challenge);
    EXPECT_EQ(chResult, SAF_SUCCESS);
    EXPECT_EQ(challenge.size(), 32u);

    TaskIdentity identity;
    int32_t genResult = SafAgentFence::GenerateTaskIdentity(info, identity);
    // Redeeming/verifying a test token requires real IAM cooperation; tolerate
    // an IAM error but never a missing pending challenge (same pattern as 023).
    if (genResult == SAF_SUCCESS) {
        EXPECT_FALSE(identity.taskAttributes.empty());
        EXPECT_FALSE(identity.identity.empty());
        EXPECT_FALSE(identity.challenge.empty());
    } else {
        EXPECT_NE(genResult, SAF_ERR_TASK_IDENTITY_NOT_FOUND);
    }

    // Cleanup
    std::vector<std::string> taskIds = {"high_risk_challenge"};
    SafAgentFence::RevokeTaskIdentity(taskIds);
}

// ==================== allowableDuration boundary (ms) ====================

/**
 * @tc.name: RetrieveAuthTokenByTaskId_018
 * @tc.desc: allowableDuration = 0 (not provided) uses default value (86400000ms = 24h).
 * @tc.type: FUNC
 * @tc.result: SAF_SUCCESS
 */
HWTEST_F(SafAgentFenceRetrieveAuthTokenTest, RetrieveAuthTokenByTaskId_018, TestSize.Level0)
{
    TaskIdentityInfo info = MakeLowRiskInfo("duration_default_001", 3600);
    info.allowableDuration = 0;

    TaskIdentity identity;
    int32_t genResult = SafAgentFence::GenerateTaskIdentity(info, identity);
    EXPECT_EQ(genResult, SAF_SUCCESS);

    // Cleanup
    std::vector<std::string> taskIds = {"duration_default_001"};
    SafAgentFence::RevokeTaskIdentity(taskIds);
}

/**
 * @tc.name: RetrieveAuthTokenByTaskId_019
 * @tc.desc: allowableDuration = 1 (minimum valid ms) passes validation.
 * @tc.type: FUNC
 * @tc.result: SAF_SUCCESS
 */
HWTEST_F(SafAgentFenceRetrieveAuthTokenTest, RetrieveAuthTokenByTaskId_019, TestSize.Level0)
{
    TaskIdentityInfo info = MakeLowRiskInfo("duration_min_001", 3600);
    info.allowableDuration = 1;

    TaskIdentity identity;
    int32_t genResult = SafAgentFence::GenerateTaskIdentity(info, identity);
    EXPECT_EQ(genResult, SAF_SUCCESS);

    // Cleanup
    std::vector<std::string> taskIds = {"duration_min_001"};
    SafAgentFence::RevokeTaskIdentity(taskIds);
}

/**
 * @tc.name: RetrieveAuthTokenByTaskId_020
 * @tc.desc: allowableDuration = 86400000 (maximum valid ms = 24h) passes validation.
 * @tc.type: FUNC
 * @tc.result: SAF_SUCCESS
 */
HWTEST_F(SafAgentFenceRetrieveAuthTokenTest, RetrieveAuthTokenByTaskId_020, TestSize.Level0)
{
    TaskIdentityInfo info = MakeLowRiskInfo("duration_max_001", 3600);
    info.allowableDuration = 86400000;

    TaskIdentity identity;
    int32_t genResult = SafAgentFence::GenerateTaskIdentity(info, identity);
    EXPECT_EQ(genResult, SAF_SUCCESS);

    // Cleanup
    std::vector<std::string> taskIds = {"duration_max_001"};
    SafAgentFence::RevokeTaskIdentity(taskIds);
}

/**
 * @tc.name: RetrieveAuthTokenByTaskId_021
 * @tc.desc: allowableDuration = -1 (negative) fails validation.
 * @tc.type: FUNC
 * @tc.result: SAF_ERR_ARG_INVALID
 */
HWTEST_F(SafAgentFenceRetrieveAuthTokenTest, RetrieveAuthTokenByTaskId_021, TestSize.Level0)
{
    TaskIdentityInfo info = MakeLowRiskInfo("duration_neg_001", 3600);
    info.allowableDuration = -1;

    TaskIdentity identity;
    int32_t genResult = SafAgentFence::GenerateTaskIdentity(info, identity);
    EXPECT_NE(genResult, SAF_SUCCESS);

    // Cleanup
    std::vector<std::string> taskIds = {"duration_neg_001"};
    SafAgentFence::RevokeTaskIdentity(taskIds);
}

/**
 * @tc.name: RetrieveAuthTokenByTaskId_022
 * @tc.desc: allowableDuration = 86400001 (exceeds 24h) fails validation.
 * @tc.type: FUNC
 * @tc.result: SAF_ERR_ARG_INVALID
 */
HWTEST_F(SafAgentFenceRetrieveAuthTokenTest, RetrieveAuthTokenByTaskId_022, TestSize.Level0)
{
    TaskIdentityInfo info = MakeLowRiskInfo("duration_over_001", 3600);
    info.allowableDuration = 86400001;

    TaskIdentity identity;
    int32_t genResult = SafAgentFence::GenerateTaskIdentity(info, identity);
    EXPECT_NE(genResult, SAF_SUCCESS);

    // Cleanup
    std::vector<std::string> taskIds = {"duration_over_001"};
    SafAgentFence::RevokeTaskIdentity(taskIds);
}

// ==================== Scheduled auth token (DELAYED task) ====================

/**
 * @tc.name: RetrieveAuthTokenByTaskId_023
 * @tc.desc: DELAYED task with scheduled authToken: generateTaskIdentity calls
 *           RedeemScheduledAuthToken first (redeem challenge is randomly generated
 *           server-side), then VerifyAuthToken on the redeemed real-time token and
 *           compares it with the redeem challenge. The stored pending challenge is
 *           NOT consulted or consumed on the DELAYED path; the generateTaskChallenge
 *           step below only verifies coexistence compatibility. Task JSON stores
 *           the original scheduled authToken.
 * @tc.type: FUNC
 * @tc.result: SAF_SUCCESS
 */
HWTEST_F(SafAgentFenceRetrieveAuthTokenTest, RetrieveAuthTokenByTaskId_023, TestSize.Level0)
{
    std::vector<int32_t> scheduledToken = {0x53, 0x43, 0x48, 0x45, 0x44};
    TaskIdentityInfo info = MakeDelayedInfo("delayed_scheduled_001", -1);
    info.authToken = scheduledToken;

    std::vector<int32_t> challenge;
    int32_t chResult = SafAgentFence::GenerateTaskChallenge(info, challenge);
    EXPECT_EQ(chResult, SAF_SUCCESS);
    EXPECT_EQ(challenge.size(), 32u);

    TaskIdentity identity;
    int32_t genResult = SafAgentFence::GenerateTaskIdentity(info, identity);
    // Redeeming a test token requires real IAM cooperation; without it the redeem
    // returns an IAM error — but never TASK_IDENTITY_NOT_FOUND (see 025).
    if (genResult == SAF_SUCCESS) {
        EXPECT_FALSE(identity.taskAttributes.empty());
        EXPECT_FALSE(identity.identity.empty());
        EXPECT_FALSE(identity.challenge.empty());
    } else {
        EXPECT_NE(genResult, SAF_ERR_TASK_IDENTITY_NOT_FOUND);
    }

    // Cleanup
    std::vector<std::string> taskIds = {"delayed_scheduled_001"};
    SafAgentFence::RevokeTaskIdentity(taskIds);
}

/**
 * @tc.name: RetrieveAuthTokenByTaskId_024
 * @tc.desc: DELAYED task with scheduled authToken: generate calls RedeemScheduledAuthToken
 *           (needs real IAM cooperation; the fake test token is rejected there),
 *           then retrieve redeems the stored scheduled token. When generation
 *           fails there is no stored record and retrieve must return
 *           TASK_IDENTITY_NOT_FOUND.
 * @tc.type: FUNC
 * @tc.result: SAF_SUCCESS or IAM error (generation) / NotFound (retrieve w/o record)
 */
HWTEST_F(SafAgentFenceRetrieveAuthTokenTest, RetrieveAuthTokenByTaskId_024, TestSize.Level0)
{
    std::vector<int32_t> scheduledToken = {0x53, 0x43, 0x48, 0x45, 0x44};
    TaskIdentityInfo info = MakeDelayedInfo("delayed_scheduled_002", -1);
    info.authToken = scheduledToken;

    std::vector<int32_t> challenge;
    int32_t chResult = SafAgentFence::GenerateTaskChallenge(info, challenge);
    EXPECT_EQ(chResult, SAF_SUCCESS);

    TaskIdentity identity;
    int32_t genResult = SafAgentFence::GenerateTaskIdentity(info, identity);
    // Redeeming a fake scheduled token requires real IAM cooperation (see 023).
    if (genResult != SAF_SUCCESS) {
        EXPECT_NE(genResult, SAF_ERR_TASK_IDENTITY_NOT_FOUND);
    }

    SwitchToRetrieveCaller();
    std::vector<uint8_t> authToken;
    int32_t result = SafAgentFence::RetrieveAuthTokenByTaskId(g_testUid,
        "delayed_scheduled_002", authToken);
    SwitchToGenerateCaller();

    if (genResult == SAF_SUCCESS) {
        // Redeem succeeds only on a real device with IAM service; on test failure
        // the token is empty (low-risk path) or the redeem returns an error.
        // We only verify the flow does not crash.
        if (result == SAF_SUCCESS) {
            EXPECT_FALSE(authToken.empty());
        }
    } else {
        // Generation failed: no record was stored, so the retrieve MUST report
        // a missing task identity rather than any other error.
        EXPECT_EQ(result, SAF_ERR_TASK_IDENTITY_NOT_FOUND);
    }

    // Cleanup
    std::vector<std::string> taskIds = {"delayed_scheduled_002"};
    SafAgentFence::RevokeTaskIdentity(taskIds);
}

/**
 * @tc.name: RetrieveAuthTokenByTaskId_025
 * @tc.desc: DELAYED task with scheduled authToken but WITHOUT generateTaskChallenge:
 *           the DELAYED challenge comparison compares the redeem-time random challenge
 *           with the VerifyAuthToken-returned challenge and never consults the stored
 *           pending challenge, so the result must NOT be TASK_IDENTITY_NOT_FOUND
 *           (any failure may only come from IAM redeem/verify).
 * @tc.type: FUNC
 * @tc.result: not SAF_ERR_TASK_IDENTITY_NOT_FOUND
 */
HWTEST_F(SafAgentFenceRetrieveAuthTokenTest, RetrieveAuthTokenByTaskId_025, TestSize.Level0)
{
    std::vector<int32_t> scheduledToken = {0x53, 0x43, 0x48, 0x45, 0x44};
    TaskIdentityInfo info = MakeDelayedInfo("delayed_no_challenge_025", -1);
    info.authToken = scheduledToken;

    TaskIdentity identity;
    int32_t genResult = SafAgentFence::GenerateTaskIdentity(info, identity);
    EXPECT_NE(genResult, SAF_ERR_TASK_IDENTITY_NOT_FOUND);

    // Cleanup
    std::vector<std::string> taskIds = {"delayed_no_challenge_025"};
    SafAgentFence::RevokeTaskIdentity(taskIds);
}
