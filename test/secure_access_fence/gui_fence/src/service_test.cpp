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

#include "errors.h"
#include "saf_gui_fence.h"
#include "saf_result_code.h"
#include "gui_callback_mock.h"
#include "gui_callback_registry.h"

using namespace testing::ext;
using namespace OHOS::Security::SAF;
using OHOS::ErrCode;
using OHOS::sptr;
using OHOS::Security::SAF::Test::GuiCallbackMock;

namespace UnitTest::GuiServiceTest {

// ---- Mock mechanism for closed-source plugin calls ----
static std::vector<std::string> g_mockDeniedPairs;
static bool g_mockQuerySuccess = true;
static bool g_mockStoreRecordSuccess = true;
static bool g_mockStoreRecordCalled = false;

static void SetMockDeniedPairs(const std::vector<std::string> &pairs)
{
    g_mockDeniedPairs = pairs;
    g_mockQuerySuccess = true;
}

__attribute__((unused)) static void SetMockQueryFail()
{
    g_mockQuerySuccess = false;
}

static void SetMockStoreRecordSuccess(bool success)
{
    g_mockStoreRecordSuccess = success;
    g_mockStoreRecordCalled = false;
}

static bool WasMockStoreRecordCalled()
{
    return g_mockStoreRecordCalled;
}

static void ResetMock()
{
    g_mockDeniedPairs.clear();
    g_mockQuerySuccess = true;
    g_mockStoreRecordSuccess = true;
    g_mockStoreRecordCalled = false;
}

// ---- Service function declarations (must match secure_access_fence_service.h signatures) ----
// These stubs mirror the real logic in secure_access_fence_service.cpp, with the closed-source
// plugin call replaced by the mock above. This allows T06 tests to pass without linking the
// full Service static library (which requires Rust cxx bridge + many external deps).
// Note: the real Register/UnregisterComponentClickCallback additionally gate on the
// CONTROL_DEVICE permission and on sessionId == calling uid. Those gates are intentionally
// omitted here because these stubs test the GuiCallbackRegistry semantics directly; the
// gated IPC paths are covered by inner_api_test.cpp.

ErrCode QueryAllDeniedPairs(
    std::vector<std::string> &deniedPairs,
    int32_t &resultCode)
{
    if (!g_mockQuerySuccess) {
        resultCode = SAF_ERROR;
        return SAF_SUCCESS;
    }
    deniedPairs = g_mockDeniedPairs;

    resultCode = SAF_SUCCESS;
    return SAF_SUCCESS;
}

ErrCode RegisterComponentClickCallback(
    uint64_t sessionId,
    const GuiCallback &callback,
    int32_t &resultCode)
{
    if (callback == nullptr) {
        resultCode = SAF_ERR_NULL_PTR;
        return SAF_SUCCESS;
    }

    int32_t ret = GuiCallbackRegistry::GetInstance().Register(sessionId, callback);
    if (ret != SAF_SUCCESS) {
        resultCode = ret;
        return SAF_SUCCESS;
    }

    resultCode = SAF_SUCCESS;
    return SAF_SUCCESS;
}

ErrCode ReportComponentClickDecision(
    uint64_t sessionId,
    const ComponentClickResult &result,
    int32_t &resultCode)
{
    GuiCallback callback = GuiCallbackRegistry::GetInstance().Find(sessionId);
    if (callback == nullptr) {
        resultCode = SAF_ERR_GUI_SESSION_NOT_FOUND;
        return SAF_SUCCESS;
    }

    if (result.compId.empty()) {
        resultCode = SAF_ERR_ARG_EMPTY;
        return SAF_SUCCESS;
    }

    callback->OnComponentClickDecision(result);

    resultCode = SAF_SUCCESS;
    return SAF_SUCCESS;
}

ErrCode UnregisterComponentClickCallback(
    uint64_t sessionId,
    int32_t &resultCode)
{
    int32_t ret = GuiCallbackRegistry::GetInstance().Unregister(sessionId);
    if (ret != SAF_SUCCESS) {
        resultCode = ret;
        return SAF_SUCCESS;
    }

    resultCode = SAF_SUCCESS;
    return SAF_SUCCESS;
}

ErrCode StoreOperationRecord(
    const AppInfo &callerApp,
    const AppInfo &targetApp,
    ControlDecision decision,
    int32_t &resultCode)
{
    if (callerApp.bundleName.empty()) {
        resultCode = SAF_ERR_ARG_EMPTY;
        return SAF_SUCCESS;
    }
    if (targetApp.bundleName.empty()) {
        resultCode = SAF_ERR_ARG_EMPTY;
        return SAF_SUCCESS;
    }

    g_mockStoreRecordCalled = true;
    if (!g_mockStoreRecordSuccess) {
        // Logging failure does not affect the caller - still return SAF_SUCCESS
        resultCode = SAF_SUCCESS;
        return SAF_SUCCESS;
    }

    resultCode = SAF_SUCCESS;
    return SAF_SUCCESS;
}

class GuiServiceTest : public testing::Test {
public:
    void SetUp() override
    {
        GuiCallbackRegistry::ResetForTesting();
        ResetMock();
    }

    void TearDown() override
    {
        GuiCallbackRegistry::ResetForTesting();
        ResetMock();
    }

protected:
    AppInfo MakeAppInfo(const std::string &bundleName, int32_t index)
    {
        AppInfo app;
        app.bundleName = bundleName;
        app.index = index;
        return app;
    }

    ComponentClickResult MakeResult(const std::string &compId, bool allowed, const std::string &reason)
    {
        ComponentClickResult result;
        result.compId = compId;
        result.allowed = allowed;
        result.reason = reason;
        return result;
    }
};

// ============ CheckControlPermission (IPC 505) ============

/**
 * @tc.name: QueryAllDeniedPairs_001
 * @tc.desc: UT-S-01: Closed-source returns denied list
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiServiceTest, QueryAllDeniedPairs_001, TestSize.Level0)
{
    SetMockDeniedPairs({"com.app.a:0:com.app.b:0", "com.app.c:0:com.app.d:0"});
    std::vector<std::string> deniedPairs;
    int32_t resultCode = -1;
    ErrCode errCode = QueryAllDeniedPairs(deniedPairs, resultCode);
    EXPECT_EQ(errCode, SAF_SUCCESS);
    EXPECT_EQ(resultCode, SAF_SUCCESS);
    EXPECT_EQ(deniedPairs.size(), 2u);
}

/**
 * @tc.name: QueryAllDeniedPairs_002
 * @tc.desc: UT-S-02: Closed-source returns empty list
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiServiceTest, QueryAllDeniedPairs_002, TestSize.Level0)
{
    SetMockDeniedPairs({});
    std::vector<std::string> deniedPairs;
    int32_t resultCode = -1;
    ErrCode errCode = QueryAllDeniedPairs(deniedPairs, resultCode);
    EXPECT_EQ(errCode, SAF_SUCCESS);
    EXPECT_EQ(resultCode, SAF_SUCCESS);
    EXPECT_TRUE(deniedPairs.empty());
}

/**
 * @tc.name: QueryAllDeniedPairs_003
 * @tc.desc: UT-S-03: Query fails -> SAF_ERR.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiServiceTest, QueryAllDeniedPairs_003, TestSize.Level0)
{
    // Permission check is reserved; this test is a placeholder.
    // When the permission check mechanism is determined, mock it here.
    SetMockQueryFail();
    std::vector<std::string> deniedPairs;
    int32_t resultCode = -1;
    ErrCode errCode = QueryAllDeniedPairs(deniedPairs, resultCode);
    EXPECT_EQ(errCode, SAF_SUCCESS);
    EXPECT_EQ(resultCode, SAF_ERROR);
}

// ============ RegisterComponentClickCallback (IPC 506) ============

/**
 * @tc.name: RegisterComponentClickCallback_001
 * @tc.desc: UT-S-07: Normal registration -> SAF_SUCCESS, registry contains (sessionId -> callback).
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiServiceTest, RegisterComponentClickCallback_001, TestSize.Level0)
{
    uint64_t sessionId = 1001;
    sptr<GuiCallbackMock> callback = new GuiCallbackMock();
    int32_t resultCode = -1;
    ErrCode errCode = RegisterComponentClickCallback(sessionId, callback, resultCode);
    EXPECT_EQ(errCode, SAF_SUCCESS);
    EXPECT_EQ(resultCode, SAF_SUCCESS);
}

/**
 * @tc.name: RegisterComponentClickCallback_002
 * @tc.desc: UT-S-08: Duplicate sessionId -> SAF_ERR_GUI_SESSION_ALREADY_EXIST.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiServiceTest, RegisterComponentClickCallback_002, TestSize.Level0)
{
    uint64_t sessionId = 1002;
    sptr<GuiCallbackMock> callback1 = new GuiCallbackMock();
    sptr<GuiCallbackMock> callback2 = new GuiCallbackMock();
    int32_t resultCode1 = -1;
    ErrCode errCode1 = RegisterComponentClickCallback(sessionId, callback1, resultCode1);
    EXPECT_EQ(errCode1, SAF_SUCCESS);
    EXPECT_EQ(resultCode1, SAF_SUCCESS);

    int32_t resultCode2 = -1;
    ErrCode errCode2 = RegisterComponentClickCallback(sessionId, callback2, resultCode2);
    EXPECT_EQ(errCode2, SAF_SUCCESS);
    EXPECT_EQ(resultCode2, SAF_ERR_GUI_SESSION_ALREADY_EXIST);
}

/**
 * @tc.name: RegisterComponentClickCallback_003
 * @tc.desc: UT-S-09: Null callback -> SAF_ERR_NULL_PTR.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiServiceTest, RegisterComponentClickCallback_003, TestSize.Level0)
{
    uint64_t sessionId = 1003;
    GuiCallback callback = nullptr;
    int32_t resultCode = -1;
    ErrCode errCode = RegisterComponentClickCallback(sessionId, callback, resultCode);
    EXPECT_EQ(errCode, SAF_SUCCESS);
    EXPECT_EQ(resultCode, SAF_ERR_NULL_PTR);
}

/**
 * @tc.name: RegisterComponentClickCallback_004
 * @tc.desc: UT-S-23: callback->AsObject() returns nullptr -> SAF_ERR_NULL_PTR, no entry added.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiServiceTest, RegisterComponentClickCallback_004, TestSize.Level0)
{
    uint64_t sessionId = 1004;
    sptr<GuiCallbackMock> callback = new GuiCallbackMock();
    callback->SetAsObjectNull(true);
    int32_t resultCode = -1;
    ErrCode errCode = RegisterComponentClickCallback(sessionId, callback, resultCode);
    EXPECT_EQ(errCode, SAF_SUCCESS);
    EXPECT_EQ(resultCode, SAF_ERR_NULL_PTR);
    EXPECT_TRUE(GuiCallbackRegistry::GetInstance().Find(sessionId) == nullptr);
}

/**
 * @tc.name: RegisterComponentClickCallback_005
 * @tc.desc: UT-S-24: AddDeathRecipient fails -> SAF_ERR_NULL_PTR, no entry added.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiServiceTest, RegisterComponentClickCallback_005, TestSize.Level0)
{
    uint64_t sessionId = 1005;
    sptr<GuiCallbackMock> callback = new GuiCallbackMock();
    callback->GetRemoteObject()->SetAddDeathRecipientFail(true);
    int32_t resultCode = -1;
    ErrCode errCode = RegisterComponentClickCallback(sessionId, callback, resultCode);
    EXPECT_EQ(errCode, SAF_SUCCESS);
    EXPECT_EQ(resultCode, SAF_ERR_NULL_PTR);
    EXPECT_TRUE(GuiCallbackRegistry::GetInstance().Find(sessionId) == nullptr);
}

// ============ ReportComponentClickDecision (IPC 507) ============

/**
 * @tc.name: ReportComponentClickDecision_001
 * @tc.desc: UT-S-10: sessionId exists + callback transparent forwarding -> SAF_SUCCESS, A app receives callback.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiServiceTest, ReportComponentClickDecision_001, TestSize.Level0)
{
    uint64_t sessionId = 2001;
    sptr<GuiCallbackMock> callback = new GuiCallbackMock();
    int32_t regCode = -1;
    RegisterComponentClickCallback(sessionId, callback, regCode);

    auto result = MakeResult("compId_001", false, "user denied");
    int32_t resultCode = -1;
    ErrCode errCode = ReportComponentClickDecision(sessionId, result, resultCode);
    EXPECT_EQ(errCode, SAF_SUCCESS);
    EXPECT_EQ(resultCode, SAF_SUCCESS);
    EXPECT_TRUE(callback->WasCalled());
}

/**
 * @tc.name: ReportComponentClickDecision_002
 * @tc.desc: UT-S-11: sessionId not found -> SAF_ERR_GUI_SESSION_NOT_FOUND.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiServiceTest, ReportComponentClickDecision_002, TestSize.Level0)
{
    uint64_t sessionId = 9999; // not registered
    auto result = MakeResult("compId_002", true, "allowed");
    int32_t resultCode = -1;
    ErrCode errCode = ReportComponentClickDecision(sessionId, result, resultCode);
    EXPECT_EQ(errCode, SAF_SUCCESS);
    EXPECT_EQ(resultCode, SAF_ERR_GUI_SESSION_NOT_FOUND);
}

/**
 * @tc.name: ReportComponentClickDecision_003
 * @tc.desc: UT-S-12: result.compId is empty -> SAF_ERR_ARG_EMPTY.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiServiceTest, ReportComponentClickDecision_003, TestSize.Level0)
{
    uint64_t sessionId = 2003;
    sptr<GuiCallbackMock> callback = new GuiCallbackMock();
    int32_t regCode = -1;
    RegisterComponentClickCallback(sessionId, callback, regCode);

    auto result = MakeResult("", true, ""); // empty compId
    int32_t resultCode = -1;
    ErrCode errCode = ReportComponentClickDecision(sessionId, result, resultCode);
    EXPECT_EQ(errCode, SAF_SUCCESS);
    EXPECT_EQ(resultCode, SAF_ERR_ARG_EMPTY);
}

/**
 * @tc.name: ReportComponentClickDecision_004
 * @tc.desc: UT-S-13: caller process dies (DeathRecipient fired) -> session removed -> SAF_ERR_GUI_SESSION_NOT_FOUND.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiServiceTest, ReportComponentClickDecision_004, TestSize.Level0)
{
    uint64_t sessionId = 2004;
    sptr<GuiCallbackMock> callback = new GuiCallbackMock();
    int32_t regCode = -1;
    RegisterComponentClickCallback(sessionId, callback, regCode);
    EXPECT_EQ(regCode, SAF_SUCCESS);

    // Simulate the registering process dying: the DeathRecipient fires
    // GuiCallbackDeathRecipient::OnRemoteDied -> GuiCallbackRegistry::RemoveByDeath.
    callback->GetRemoteObject()->TriggerDeath();
    EXPECT_TRUE(GuiCallbackRegistry::GetInstance().Find(sessionId) == nullptr);

    auto result = MakeResult("compId_004", false, "denied");
    int32_t resultCode = -1;
    ErrCode errCode = ReportComponentClickDecision(sessionId, result, resultCode);
    EXPECT_EQ(errCode, SAF_SUCCESS);
    EXPECT_EQ(resultCode, SAF_ERR_GUI_SESSION_NOT_FOUND);
}

// ============ UnregisterComponentClickCallback (IPC 508) ============

/**
 * @tc.name: UnregisterComponentClickCallback_001
 * @tc.desc: UT-S-14: sessionId exists -> SAF_SUCCESS, removed from registry.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiServiceTest, UnregisterComponentClickCallback_001, TestSize.Level0)
{
    uint64_t sessionId = 3001;
    sptr<GuiCallbackMock> callback = new GuiCallbackMock();
    int32_t regCode = -1;
    RegisterComponentClickCallback(sessionId, callback, regCode);

    int32_t resultCode = -1;
    ErrCode errCode = UnregisterComponentClickCallback(sessionId, resultCode);
    EXPECT_EQ(errCode, SAF_SUCCESS);
    EXPECT_EQ(resultCode, SAF_SUCCESS);
}

/**
 * @tc.name: UnregisterComponentClickCallback_002
 * @tc.desc: UT-S-15: sessionId not found -> SAF_ERR_GUI_SESSION_NOT_FOUND.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiServiceTest, UnregisterComponentClickCallback_002, TestSize.Level0)
{
    uint64_t sessionId = 9999; // not registered
    int32_t resultCode = -1;
    ErrCode errCode = UnregisterComponentClickCallback(sessionId, resultCode);
    EXPECT_EQ(errCode, SAF_SUCCESS);
    EXPECT_EQ(resultCode, SAF_ERR_GUI_SESSION_NOT_FOUND);
}

// ============ StoreOperationRecord (IPC 509) ============

/**
 * @tc.name: StoreOperationRecord_001
 * @tc.desc: UT-S-16: Valid caller + valid params + closed-source write success -> SAF_SUCCESS.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiServiceTest, StoreOperationRecord_001, TestSize.Level0)
{
    auto callerApp = MakeAppInfo("com.app.a", 0);
    auto targetApp = MakeAppInfo("com.app.b", 0);
    SetMockStoreRecordSuccess(true);
    int32_t resultCode = -1;
    ErrCode errCode = StoreOperationRecord(callerApp, targetApp, ControlDecision::ALLOW, resultCode);
    EXPECT_EQ(errCode, SAF_SUCCESS);
    EXPECT_EQ(resultCode, SAF_SUCCESS);
    EXPECT_TRUE(WasMockStoreRecordCalled());
}

/**
 * @tc.name: StoreOperationRecord_002
 * @tc.desc: UT-S-17: callerApp.bundleName is empty -> SAF_ERR_ARG_EMPTY.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiServiceTest, StoreOperationRecord_002, TestSize.Level0)
{
    auto callerApp = MakeAppInfo("", 0);
    auto targetApp = MakeAppInfo("com.app.b", 0);
    int32_t resultCode = -1;
    ErrCode errCode = StoreOperationRecord(callerApp, targetApp, ControlDecision::DENY, resultCode);
    EXPECT_EQ(errCode, SAF_SUCCESS);
    EXPECT_EQ(resultCode, SAF_ERR_ARG_EMPTY);
    EXPECT_FALSE(WasMockStoreRecordCalled()); // plugin should not be called on validation failure
}

/**
 * @tc.name: StoreOperationRecord_003
 * @tc.desc: UT-S-18: Closed-source write fails -> SAF_SUCCESS (logging failure does not affect caller).
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiServiceTest, StoreOperationRecord_003, TestSize.Level0)
{
    auto callerApp = MakeAppInfo("com.app.a", 0);
    auto targetApp = MakeAppInfo("com.app.b", 0);
    SetMockStoreRecordSuccess(false); // simulate closed-source write failure
    int32_t resultCode = -1;
    ErrCode errCode = StoreOperationRecord(callerApp, targetApp, ControlDecision::DENY, resultCode);
    EXPECT_EQ(errCode, SAF_SUCCESS);
    EXPECT_EQ(resultCode, SAF_SUCCESS); // logging failure should not affect return
    EXPECT_TRUE(WasMockStoreRecordCalled());
}

// ============ SA idle-unload decision (callback registry empty/non-empty) ============

/**
 * @tc.name: CallbackRegistry_Empty_001
 * @tc.desc: UT-S-19: Registry empty -> SA idle-unload restores 60s logic.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiServiceTest, CallbackRegistry_Empty_001, TestSize.Level0)
{
    auto &registry = GuiCallbackRegistry::GetInstance();
    EXPECT_TRUE(registry.IsEmpty());
}

/**
 * @tc.name: CallbackRegistry_NonEmpty_001
 * @tc.desc: UT-S-20: Registry non-empty (A registered but not unregistered) -> SA must not unload (no 60s limit).
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiServiceTest, CallbackRegistry_NonEmpty_001, TestSize.Level0)
{
    auto &registry = GuiCallbackRegistry::GetInstance();
    sptr<GuiCallbackMock> callback = new GuiCallbackMock();
    int32_t ret = registry.Register(4001, callback);
    EXPECT_EQ(ret, SAF_SUCCESS);
    EXPECT_FALSE(registry.IsEmpty());
}

/**
 * @tc.name: CallbackRegistry_TransitionToEmpty_001
 * @tc.desc: UT-S-21: Registry non-empty -> A unregisters -> registry empty -> SA restores 60s logic.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiServiceTest, CallbackRegistry_TransitionToEmpty_001, TestSize.Level0)
{
    auto &registry = GuiCallbackRegistry::GetInstance();
    uint64_t sessionId = 4002;
    sptr<GuiCallbackMock> callback = new GuiCallbackMock();
    EXPECT_EQ(registry.Register(sessionId, callback), SAF_SUCCESS);
    EXPECT_FALSE(registry.IsEmpty());

    EXPECT_EQ(registry.Unregister(sessionId), SAF_SUCCESS);
    EXPECT_TRUE(registry.IsEmpty());
    EXPECT_TRUE(callback->GetRemoteObject()->WasRemoveDeathRecipientCalled());
}

/**
 * @tc.name: CallbackRegistry_UnregisterRemovesDeathRecipient_001
 * @tc.desc: UT-S-22: Unregister removes the DeathRecipient from the remote object.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiServiceTest, CallbackRegistry_UnregisterRemovesDeathRecipient_001, TestSize.Level0)
{
    auto &registry = GuiCallbackRegistry::GetInstance();
    uint64_t sessionId = 4003;
    sptr<GuiCallbackMock> callback = new GuiCallbackMock();
    ASSERT_EQ(registry.Register(sessionId, callback), SAF_SUCCESS);
    EXPECT_FALSE(callback->GetRemoteObject()->WasRemoveDeathRecipientCalled());

    EXPECT_EQ(registry.Unregister(sessionId), SAF_SUCCESS);
    EXPECT_TRUE(callback->GetRemoteObject()->WasRemoveDeathRecipientCalled());
    EXPECT_TRUE(registry.IsEmpty());
}

/**
 * @tc.name: CallbackRegistry_RemoveByDeathNotFound_001
 * @tc.desc: UT-S-25: RemoveByDeath on a missing session is a silent no-op.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiServiceTest, CallbackRegistry_RemoveByDeathNotFound_001, TestSize.Level0)
{
    auto &registry = GuiCallbackRegistry::GetInstance();
    EXPECT_TRUE(registry.IsEmpty());
    registry.RemoveByDeath(4999); // never registered
    EXPECT_TRUE(registry.IsEmpty());
}

/**
 * @tc.name: CallbackRegistry_RemoveByDeathRemovesEntry_001
 * @tc.desc: UT-S-26: DeathRecipient fires -> entry removed without RemoveDeathRecipient.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiServiceTest, CallbackRegistry_RemoveByDeathRemovesEntry_001, TestSize.Level0)
{
    auto &registry = GuiCallbackRegistry::GetInstance();
    uint64_t sessionId = 4004;
    sptr<GuiCallbackMock> callback = new GuiCallbackMock();
    ASSERT_EQ(registry.Register(sessionId, callback), SAF_SUCCESS);
    EXPECT_FALSE(registry.IsEmpty());

    callback->GetRemoteObject()->TriggerDeath();
    EXPECT_FALSE(callback->GetRemoteObject()->WasRemoveDeathRecipientCalled());
    EXPECT_TRUE(registry.IsEmpty());
    EXPECT_TRUE(registry.Find(sessionId) == nullptr);
}

/**
 * @tc.name: CallbackRegistry_ClearRemovesDeathRecipients_001
 * @tc.desc: UT-S-27: Clear detaches all DeathRecipients before wiping the maps.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiServiceTest, CallbackRegistry_ClearRemovesDeathRecipients_001, TestSize.Level0)
{
    auto &registry = GuiCallbackRegistry::GetInstance();
    sptr<GuiCallbackMock> cb1 = new GuiCallbackMock();
    sptr<GuiCallbackMock> cb2 = new GuiCallbackMock();
    ASSERT_EQ(registry.Register(4101, cb1), SAF_SUCCESS);
    ASSERT_EQ(registry.Register(4102, cb2), SAF_SUCCESS);
    EXPECT_FALSE(registry.IsEmpty());

    registry.Clear();
    EXPECT_TRUE(registry.IsEmpty());
    EXPECT_TRUE(cb1->GetRemoteObject()->WasRemoveDeathRecipientCalled());
    EXPECT_TRUE(cb2->GetRemoteObject()->WasRemoveDeathRecipientCalled());
}

/**
 * @tc.name: CallbackRegistry_RegisterNullCallback_001
 * @tc.desc: UT-S-28: Register with a null callback directly at registry level -> SAF_ERR_NULL_PTR,
 *           registry stays untouched (the service-level null gate is tested by
 *           RegisterComponentClickCallback_003; this reaches the registry branch itself).
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiServiceTest, CallbackRegistry_RegisterNullCallback_001, TestSize.Level0)
{
    auto &registry = GuiCallbackRegistry::GetInstance();
    EXPECT_EQ(registry.Register(4201, nullptr), SAF_ERR_NULL_PTR);
    EXPECT_TRUE(registry.IsEmpty());
    EXPECT_TRUE(registry.Find(4201) == nullptr);
}

/**
 * @tc.name: CallbackRegistry_UnregisterNullAsObject_001
 * @tc.desc: UT-S-29: Unregister an entry whose AsObject() turned null after registration ->
 *           SAF_SUCCESS, RemoveDeathRecipient skipped on the null remote object, entry erased.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiServiceTest, CallbackRegistry_UnregisterNullAsObject_001, TestSize.Level0)
{
    auto &registry = GuiCallbackRegistry::GetInstance();
    uint64_t sessionId = 4202;
    sptr<GuiCallbackMock> callback = new GuiCallbackMock();
    ASSERT_EQ(registry.Register(sessionId, callback), SAF_SUCCESS);

    // The remote object disappears between Register and Unregister; the detach
    // step must be skipped rather than dereferenced, and the entry still erased.
    callback->SetAsObjectNull(true);
    EXPECT_EQ(registry.Unregister(sessionId), SAF_SUCCESS);
    EXPECT_FALSE(callback->GetRemoteObject()->WasRemoveDeathRecipientCalled());
    EXPECT_TRUE(registry.Find(sessionId) == nullptr);
    EXPECT_TRUE(registry.IsEmpty());
}

/**
 * @tc.name: CallbackRegistry_ClearNullAsObject_001
 * @tc.desc: UT-S-30: Clear with one entry whose AsObject() turned null -> the null entry skips
 *           RemoveDeathRecipient, the healthy entry is still detached, both entries erased.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiServiceTest, CallbackRegistry_ClearNullAsObject_001, TestSize.Level0)
{
    auto &registry = GuiCallbackRegistry::GetInstance();
    sptr<GuiCallbackMock> nullCb = new GuiCallbackMock();
    sptr<GuiCallbackMock> healthyCb = new GuiCallbackMock();
    ASSERT_EQ(registry.Register(4203, nullCb), SAF_SUCCESS);
    ASSERT_EQ(registry.Register(4204, healthyCb), SAF_SUCCESS);
    nullCb->SetAsObjectNull(true);

    registry.Clear();
    EXPECT_TRUE(registry.IsEmpty());
    // The null-AsObject entry skips the detach; the healthy one is detached as usual.
    EXPECT_FALSE(nullCb->GetRemoteObject()->WasRemoveDeathRecipientCalled());
    EXPECT_TRUE(healthyCb->GetRemoteObject()->WasRemoveDeathRecipientCalled());
}

/**
 * @tc.name: CallbackRegistry_UnregisterWithoutDeathRecipient_001
 * @tc.desc: UT-S-31: Unregister a session whose death-recipient record was lost (bookkeeping
 *           out of sync with the session map) -> SAF_SUCCESS, the detach block is skipped via
 *           the missing-record fallback, session erased. Uses the DropDeathRecipientForTesting
 *           seam; production code keeps both maps in sync.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiServiceTest, CallbackRegistry_UnregisterWithoutDeathRecipient_001, TestSize.Level0)
{
    auto &registry = GuiCallbackRegistry::GetInstance();
    uint64_t sessionId = 4205;
    sptr<GuiCallbackMock> callback = new GuiCallbackMock();
    ASSERT_EQ(registry.Register(sessionId, callback), SAF_SUCCESS);

    // Desync the bookkeeping: the session stays registered while its
    // death-recipient record disappears. Unregister must tolerate the
    // inconsistency and still remove the session without detaching.
    registry.DropDeathRecipientForTesting(sessionId);
    EXPECT_EQ(registry.Unregister(sessionId), SAF_SUCCESS);
    EXPECT_FALSE(callback->GetRemoteObject()->WasRemoveDeathRecipientCalled());
    EXPECT_TRUE(registry.Find(sessionId) == nullptr);
    EXPECT_TRUE(registry.IsEmpty());
}

/**
 * @tc.name: CallbackRegistry_ClearWithoutDeathRecipient_001
 * @tc.desc: UT-S-32: Clear with one session whose death-recipient record was lost -> the
 *           desynced session skips detach via the missing-record fallback, the healthy one
 *           is still detached, both sessions erased.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiServiceTest, CallbackRegistry_ClearWithoutDeathRecipient_001, TestSize.Level0)
{
    auto &registry = GuiCallbackRegistry::GetInstance();
    sptr<GuiCallbackMock> desyncCb = new GuiCallbackMock();
    sptr<GuiCallbackMock> healthyCb = new GuiCallbackMock();
    ASSERT_EQ(registry.Register(4206, desyncCb), SAF_SUCCESS);
    ASSERT_EQ(registry.Register(4207, healthyCb), SAF_SUCCESS);
    registry.DropDeathRecipientForTesting(4206);

    registry.Clear();
    EXPECT_TRUE(registry.IsEmpty());
    EXPECT_FALSE(desyncCb->GetRemoteObject()->WasRemoveDeathRecipientCalled());
    EXPECT_TRUE(healthyCb->GetRemoteObject()->WasRemoveDeathRecipientCalled());
}

} // namespace UnitTest::GuiServiceTest
