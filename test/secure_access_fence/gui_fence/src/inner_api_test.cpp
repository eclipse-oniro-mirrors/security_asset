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

#include <chrono>
#include <condition_variable>
#include <gtest/gtest.h>
#include <mutex>
#include <thread>
#include <unistd.h>

#include "accesstoken_kit.h"
#include "errors.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "message_option.h"
#include "message_parcel.h"
#include "nativetoken_kit.h"
#include "system_ability_load_callback_stub.h"
#include "token_setproc.h"

#include "saf_gui_fence.h"
#include "saf_result_code.h"
#include "gui_cache_manager.h"

using namespace testing::ext;
using namespace OHOS::Security::SAF;

namespace UnitTest::GuiInnerApiTest {

// Test constants
constexpr int32_t TEST_APP_INDEX = 0;
constexpr int64_t CACHE_HIT_LATENCY_THRESHOLD_US = 2000; // 2ms = 2000us
// A fake OS account ID guaranteed to differ from any real foreground user ID
// (real IDs are >= 100), used to simulate a user-switch mismatch in the cache.
constexpr int32_t MISMATCHED_USER_ID = -2;

static void GrantTestPermission()
{
    const char *permissions[] = {
        "ohos.permission.CONTROL_DEVICE",
    };

    NativeTokenInfoParams info = {
        .dcapsNum = 0,
        .permsNum = 1,
        .aclsNum = 0,
        .dcaps = nullptr,
        .perms = permissions,
        .acls = nullptr,
        .processName = "saf_gui_inner_api_test",
        .aplStr = "system_basic",
    };
    uint64_t tokenId = GetAccessTokenId(&info);
    SetSelfTokenID(tokenId);
}

// Whether ohos.permission.CONTROL_DEVICE is currently verifiable for this test
// process. The permission is only registered on 2in1 (PC) devices; on other
// device forms the Register/UnregisterComponentClickCallback service gates
// return SAF_ERR_PERMISSION_DENIED first, so the tests below must branch on it.
static bool IsControlDevicePermissionAvailable()
{
    auto tokenId = OHOS::IPCSkeleton::GetCallingTokenID();
    return OHOS::Security::AccessToken::AccessTokenKit::VerifyAccessToken(
        tokenId, "ohos.permission.CONTROL_DEVICE") == OHOS::Security::AccessToken::PERMISSION_GRANTED;
}

// The service identifies a click-result feedback session by the calling uid
// (anti-forgery: one controller cannot unregister or intercept another
// controller's results), so register/unregister must pass this process's uid.
static uint64_t OwnSessionId()
{
    return static_cast<uint64_t>(getuid());
}

// Real IGuiCallback stub inside this test process. Unlike GuiCallbackMock (a
// plain class whose AsObject returns a mock object), it is a genuine binder
// object, so it can travel through real IPC and the service stores a proxy to
// it in the session registry. The parcel field order mirrors the app-side
// ComponentClickCallbackStub in the napi layer.
class InnerApiCallbackStub : public OHOS::IRemoteStub<IGuiCallback> {
public:
    InnerApiCallbackStub() = default;
    ~InnerApiCallbackStub() override = default;

    void OnComponentClickDecision(const ComponentClickResult &result) override
    {
        lastResult_ = result;
        called_ = true;
    }

    int OnRemoteRequest(uint32_t code, OHOS::MessageParcel &data, OHOS::MessageParcel &reply,
        OHOS::MessageOption &option) override
    {
        if (code == static_cast<uint32_t>(IGuiCallback::ON_COMPONENT_CLICK_DECISION)) {
            if (data.ReadInterfaceToken() != IGuiCallback::GetDescriptor()) {
                return OHOS::ERR_INVALID_DATA;
            }
            ComponentClickResult result;
            result.compId = data.ReadString();
            result.allowed = data.ReadBool();
            result.reason = data.ReadString();
            OnComponentClickDecision(result);
            return OHOS::ERR_NONE;
        }
        return OHOS::IPCObjectStub::OnRemoteRequest(code, data, reply, option);
    }

    bool WasCalled() const { return called_; }
    const ComponentClickResult &GetLastResult() const { return lastResult_; }

private:
    bool called_ = false;
    ComponentClickResult lastResult_ {};
};

// SA load callback that signals a condition_variable when loading completes.
class SafLoadCallback : public OHOS::SystemAbilityLoadCallbackStub {
public:
    void OnLoadSystemAbilitySuccess(int32_t systemAbilityId,
        const OHOS::sptr<OHOS::IRemoteObject> &remoteObject) override
    {
        std::lock_guard<std::mutex> lock(mtx_);
        loaded_ = true;
        cv_.notify_one();
    }

    void OnLoadSystemAbilityFail(int32_t systemAbilityId) override
    {
        std::lock_guard<std::mutex> lock(mtx_);
        loaded_ = true;
        cv_.notify_one();
    }

    bool WaitForLoad(int32_t timeoutSec)
    {
        std::unique_lock<std::mutex> lock(mtx_);
        return cv_.wait_for(lock, std::chrono::seconds(timeoutSec), [this] {
            return loaded_;
        });
    }

private:
    std::mutex mtx_;
    std::condition_variable cv_;
    bool loaded_ = false;
};

class GuiInnerApiTest : public testing::Test {
public:
    static void SetUpTestCase(void)
    {
        GrantTestPermission();
    }

    static void TearDownTestCase(void)
    {
    }

    void SetUp() override
    {
        GuiCacheManager::ResetForTesting();
    }

    void TearDown() override
    {
        GuiCacheManager::ResetForTesting();
    }

protected:
    AppInfo MakeAppInfo(const std::string &bundleName, int32_t index)
    {
        AppInfo app;
        app.bundleName = bundleName;
        app.index = index;
        return app;
    }

    ComponentClickResult MakeComponentClickResult(const std::string &compId, bool allowed, const std::string &reason)
    {
        ComponentClickResult result;
        result.compId = compId;
        result.allowed = allowed;
        result.reason = reason;
        return result;
    }
};

// ================== CheckControlPermission tests =================

/**
 * @tc.name: InnerApi_CheckControlPermission_001
 * @tc.desc: First call with valid params -> SA loaded via LoadSystemAbility, cache initialized.
 * @tc.type: FUNC
 * @tc.require:
 */

HWTEST_F(GuiInnerApiTest, InnerApi_CheckControlPermission_001, TestSize.Level0)
{
    auto callerApp = MakeAppInfo("com.app.a", TEST_APP_INDEX);
    auto targetApp = MakeAppInfo("com.app.b", TEST_APP_INDEX);
    
    ControlDecision decision = ControlDecision::ALLOW;
    int32_t ret = SafGuiFence::CheckControlPermission(callerApp, targetApp, decision);
    EXPECT_EQ(ret, SAF_SUCCESS);
    EXPECT_TRUE(decision == ControlDecision::ALLOW);
    EXPECT_TRUE(GuiCacheManager::GetInstance().IsInitialized());
}

/**
 * @tc.name: InnerApi_CheckControlPermission_002
 * @tc.desc: Second call after first (cache hit) + verify latency < 2ms.
 * @tc.type: PERF
 * @tc.require:
 */

HWTEST_F(GuiInnerApiTest, InnerApi_CheckControlPermission_002, TestSize.Level0)
{
    auto callerApp = MakeAppInfo("com.app.a", TEST_APP_INDEX);
    auto targetApp = MakeAppInfo("com.app.b", TEST_APP_INDEX);

    // First call - initialize cache via SA
    ControlDecision firstDecision = ControlDecision::ALLOW;
    int32_t firstRet = SafGuiFence::CheckControlPermission(callerApp, targetApp, firstDecision);
    EXPECT_EQ(firstRet, SAF_SUCCESS);
    EXPECT_TRUE(GuiCacheManager::GetInstance().IsInitialized());

    // Second call - cache hit, verify latency < 2ms
    auto start = std::chrono::steady_clock::now();
    ControlDecision secondDecision = ControlDecision::ALLOW;
    int32_t secondRet = SafGuiFence::CheckControlPermission(callerApp, targetApp, secondDecision);
    auto end = std::chrono::steady_clock::now();
    auto durationUs = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    EXPECT_EQ(secondRet, SAF_SUCCESS);
    EXPECT_LT(durationUs, CACHE_HIT_LATENCY_THRESHOLD_US);
}

 /**
 * @tc.name: InnerApi_CheckControlPermission_003
 * @tc.desc: Call with valid params -> SA returns success and a valid decision.
 * @tc.type: FUNC
 * @tc.require:
 */

HWTEST_F(GuiInnerApiTest, InnerApi_CheckControlPermission_003, TestSize.Level0)
{
    auto callerApp = MakeAppInfo("com.app.a", TEST_APP_INDEX);
    auto targetApp = MakeAppInfo("com.app.b", TEST_APP_INDEX);

    ControlDecision decision = ControlDecision::ALLOW;
    int32_t ret = SafGuiFence::CheckControlPermission(callerApp, targetApp, decision);
    EXPECT_EQ(ret, SAF_SUCCESS);
    EXPECT_TRUE(decision == ControlDecision::ALLOW || decision == ControlDecision::DENY);
}

/**
 * @tc.name: InnerApi_CheckControlPermission_004
 * @tc.desc: callerApp.bundleName empty -> SAF_ERR_ARG_EMPTY (param validation before IPC).
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiInnerApiTest, InnerApi_CheckControlPermission_004, TestSize.Level0)
{
    auto callerApp = MakeAppInfo("", TEST_APP_INDEX); // empty bundleName
    auto targetApp = MakeAppInfo("com.app.b", TEST_APP_INDEX);
    ControlDecision decision = ControlDecision::ALLOW;
    int32_t ret = SafGuiFence::CheckControlPermission(callerApp, targetApp, decision);
    EXPECT_EQ(ret, SAF_ERR_ARG_EMPTY);
}

/**
 * @tc.name: InnerApi_CheckControlPermission_005
 * @tc.desc: targetApp.bundleName empty -> SAF_ERR_ARG_EMPTY (param validation before IPC).
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiInnerApiTest, InnerApi_CheckControlPermission_005, TestSize.Level0)
{
    auto callerApp = MakeAppInfo("com.app.a", TEST_APP_INDEX);
    auto targetApp = MakeAppInfo("", TEST_APP_INDEX); // empty bundleName
    ControlDecision decision = ControlDecision::ALLOW;
    int32_t ret = SafGuiFence::CheckControlPermission(callerApp, targetApp, decision);
    EXPECT_EQ(ret, SAF_ERR_ARG_EMPTY);
}

/**
 * @tc.name: InnerApi_CheckControlPermission_006
 * @tc.desc: Self-pair (callerApp == targetApp) -> SA returns success and a valid decision.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiInnerApiTest, InnerApi_CheckControlPermission_006, TestSize.Level0)
{
    auto callerApp = MakeAppInfo("com.app.a", TEST_APP_INDEX);
    auto targetApp = MakeAppInfo("com.app.a", TEST_APP_INDEX);
    ControlDecision decision = ControlDecision::ALLOW;
    int32_t ret = SafGuiFence::CheckControlPermission(callerApp, targetApp, decision);
    EXPECT_EQ(ret, SAF_SUCCESS);
    EXPECT_TRUE(decision == ControlDecision::ALLOW || decision == ControlDecision::DENY);
}

// ================== RegisterComponentClickCallback tests =================

/**
 * @tc.name: InnerApi_RegisterComponentClickCallback_001
 * @tc.desc: Null callback -> SAF_ERR_NULL_PTR (param validation before IPC, no SA needed).
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiInnerApiTest, InnerApi_RegisterComponentClickCallback_001, TestSize.Level0)
{
    GuiCallback callback = nullptr;
    int32_t ret = SafGuiFence::RegisterComponentClickCallback(70001, callback);
    EXPECT_EQ(ret, SAF_ERR_NULL_PTR);
}

/**
 * @tc.name: InnerApi_RegisterComponentClickCallback_002
 * @tc.desc: Register with the caller's own uid and a real callback stub -> SAF_SUCCESS
 *          (permission + anti-forgery session gates pass), then unregister -> SAF_SUCCESS.
 *          On non-2in1 devices CONTROL_DEVICE is not registered, so the permission gate
 *          rejects with SAF_ERR_PERMISSION_DENIED instead.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiInnerApiTest, InnerApi_RegisterComponentClickCallback_002, TestSize.Level0)
{
    GuiCallback callback = new InnerApiCallbackStub();
    ASSERT_NE(callback, nullptr);
    // Tolerate a stale session left by an earlier run of this test binary; the
    // death recipient normally cleans it up when the previous process exits.
    SafGuiFence::UnregisterComponentClickCallback(OwnSessionId());
    int32_t ret = SafGuiFence::RegisterComponentClickCallback(OwnSessionId(), callback);
    if (IsControlDevicePermissionAvailable()) {
        EXPECT_EQ(ret, SAF_SUCCESS);
        EXPECT_EQ(SafGuiFence::UnregisterComponentClickCallback(OwnSessionId()), SAF_SUCCESS);
    } else {
        EXPECT_EQ(ret, SAF_ERR_PERMISSION_DENIED);
    }
}

/**
 * @tc.name: InnerApi_RegisterComponentClickCallback_003
 * @tc.desc: Register with a session id that is not the caller's uid -> rejected with
 *          SAF_ERR_ARG_INVALID by the anti-forgery gate (a controller must not be able
 *          to intercept another controller's click results). The unregister path
 *          enforces the same gate. On non-2in1 devices the permission gate rejects
 *          first with SAF_ERR_PERMISSION_DENIED.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiInnerApiTest, InnerApi_RegisterComponentClickCallback_003, TestSize.Level0)
{
    // A session id that can never equal this test process's uid.
    uint64_t forgedSessionId = OwnSessionId() + 1;
    GuiCallback callback = new InnerApiCallbackStub();
    ASSERT_NE(callback, nullptr);
    int32_t ret = SafGuiFence::RegisterComponentClickCallback(forgedSessionId, callback);
    if (IsControlDevicePermissionAvailable()) {
        EXPECT_EQ(ret, SAF_ERR_ARG_INVALID);
        EXPECT_EQ(SafGuiFence::UnregisterComponentClickCallback(forgedSessionId), SAF_ERR_ARG_INVALID);
    } else {
        EXPECT_EQ(ret, SAF_ERR_PERMISSION_DENIED);
    }
}

// ================== ReportComponentClickDecision tests =================

/**
 * @tc.name: InnerApi_ReportComponentClickDecision_001
 * @tc.desc: result.compId is empty -> SAF_ERR_ARG_EMPTY (param validation before IPC, no SA needed).
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiInnerApiTest, InnerApi_ReportComponentClickDecision_001, TestSize.Level0)
{
    auto result = MakeComponentClickResult("", true, "allowed");
    int32_t ret = SafGuiFence::ReportComponentClickDecision(80001, result);
    EXPECT_EQ(ret, SAF_ERR_ARG_EMPTY);
}

/**
 * @tc.name: InnerApi_ReportComponentClickDecision_002
 * @tc.desc: Valid compId but unknown sessionId -> service returns SAF_ERR_GUI_SESSION_NOT_FOUND.
 *          Covers GetProxy success, IPC success, and the resultCode != SAF_SUCCESS branch.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiInnerApiTest, InnerApi_ReportComponentClickDecision_002, TestSize.Level0)
{
    // Unique sessionId that was never registered.
    auto result = MakeComponentClickResult("comp_report_002", false, "user denied");
    int32_t ret = SafGuiFence::ReportComponentClickDecision(987651, result);
    EXPECT_EQ(ret, SAF_ERR_GUI_SESSION_NOT_FOUND);
}

/**
 * @tc.name: InnerApi_ReportComponentClickDecision_003
 * @tc.desc: E2E: register an own-uid session, report a valid decision, and the real service
 *          forwards it through the GuiCallbackProxy back into this process, where the callback
 *          stub receives compId/allowed/reason intact; finally unregister. Covers the
 *          service-side Report happy path (session found, compId non-empty, proxy callback)
 *          and the proxy oneway marshalling. On non-2in1 devices CONTROL_DEVICE is not
 *          registered, so the register permission gate rejects and no session is established.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiInnerApiTest, InnerApi_ReportComponentClickDecision_003, TestSize.Level0)
{
    InnerApiCallbackStub *stub = new InnerApiCallbackStub();
    GuiCallback callback = stub;
    // Tolerate a stale session left by an earlier run of this test binary; the
    // death recipient normally cleans it up when the previous process exits.
    SafGuiFence::UnregisterComponentClickCallback(OwnSessionId());
    int32_t regRet = SafGuiFence::RegisterComponentClickCallback(OwnSessionId(), callback);
    if (!IsControlDevicePermissionAvailable()) {
        EXPECT_EQ(regRet, SAF_ERR_PERMISSION_DENIED);
        return;
    }
    ASSERT_EQ(regRet, SAF_SUCCESS);

    auto result = MakeComponentClickResult(
        "{\"bundleName\":\"com.example.myapplication\",\"index\":\"0\"}", true, "policy allowed");
    int32_t reportRet = SafGuiFence::ReportComponentClickDecision(OwnSessionId(), result);
    EXPECT_EQ(reportRet, SAF_SUCCESS);

    // GuiCallbackProxy delivers the decision with MessageOption::TF_ASYNC, so the
    // callback may land after the report IPC returns; poll the stub for ~2s.
    bool delivered = false;
    for (int32_t i = 0; i < 100 && !delivered; ++i) {
        delivered = stub->WasCalled();
        if (!delivered) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }
    ASSERT_TRUE(delivered);
    const ComponentClickResult &deliveredResult = stub->GetLastResult();
    EXPECT_EQ(deliveredResult.compId, result.compId);
    EXPECT_TRUE(deliveredResult.allowed);
    EXPECT_EQ(deliveredResult.reason, result.reason);

    EXPECT_EQ(SafGuiFence::UnregisterComponentClickCallback(OwnSessionId()), SAF_SUCCESS);
}

// ================== UnregisterComponentClickCallback tests =================

/**
 * @tc.name: InnerApi_UnregisterComponentClickCallback_001
 * @tc.desc: Own-uid session that was never registered -> service returns SAF_ERR_GUI_SESSION_NOT_FOUND.
 *          The session id must be the caller's own uid; any other value is rejected by the
 *          anti-forgery gate before the registry lookup. On non-2in1 devices CONTROL_DEVICE is
 *          not registered, so the permission gate rejects first.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiInnerApiTest, InnerApi_UnregisterComponentClickCallback_001, TestSize.Level0)
{
    // The service only accepts the caller's own uid as the session id.
    int32_t ret = SafGuiFence::UnregisterComponentClickCallback(OwnSessionId());
    if (IsControlDevicePermissionAvailable()) {
        EXPECT_EQ(ret, SAF_ERR_GUI_SESSION_NOT_FOUND);
    } else {
        EXPECT_EQ(ret, SAF_ERR_PERMISSION_DENIED);
    }
}

// ================== PreloadAgentAppStatus tests =================

/**
 * @tc.name: InnerApi_PreloadAgentAppStatus_001
 * @tc.desc: Preload after cache is already initialized -> hits the "cache already initialized"
 *          early-return branch and returns SAF_SUCCESS without re-initializing.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiInnerApiTest, InnerApi_PreloadAgentAppStatus_001, TestSize.Level0)
{
    // Prime the cache via a normal CheckControlPermission call so the
    // "cache already initialized" branch inside PreloadAgentAppStatus is taken.
    auto callerApp = MakeAppInfo("com.app.preload", TEST_APP_INDEX);
    auto targetApp = MakeAppInfo("com.app.target", TEST_APP_INDEX);
    ControlDecision decision = ControlDecision::ALLOW;
    int32_t primeRet = SafGuiFence::CheckControlPermission(callerApp, targetApp, decision);
    ASSERT_EQ(primeRet, SAF_SUCCESS);
    ASSERT_TRUE(GuiCacheManager::GetInstance().IsInitialized());

    // Now PreloadAgentAppStatus must short-circuit on the initialized cache.
    int32_t ret = SafGuiFence::PreloadAgentAppStatus();
    EXPECT_EQ(ret, SAF_SUCCESS);
    EXPECT_TRUE(GuiCacheManager::GetInstance().IsInitialized());
}

/**
 * @tc.name: InnerApi_PreloadAgentAppStatus_002
 * @tc.desc: Preload with an uninitialized cache -> falls through to InitCacheFromService,
 *          which loads the denied-pairs list via IPC and registers the settings observer.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiInnerApiTest, InnerApi_PreloadAgentAppStatus_002, TestSize.Level0)
{
    // TearDown of the previous case resets the cache, so here it is uninitialized
    // and PreloadAgentAppStatus must take the InitCacheFromService path.
    ASSERT_FALSE(GuiCacheManager::GetInstance().IsInitialized());

    int32_t ret = SafGuiFence::PreloadAgentAppStatus();
    EXPECT_EQ(ret, SAF_SUCCESS);
    EXPECT_TRUE(GuiCacheManager::GetInstance().IsInitialized());
}

/**
 * @tc.name: InnerApi_PreloadAgentAppStatus_UserSwitch_001
 * @tc.desc: Preload when cache is initialized but cachedUserId differs from the current
 *          foreground user -> the "cache already initialized" early-return is skipped and
 *          InitCacheFromService runs again (covers the userId-mismatch branch).
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GuiInnerApiTest, InnerApi_PreloadAgentAppStatus_UserSwitch_001, TestSize.Level0)
{
    // Prime the cache so it is initialized with the real current user ID.
    ASSERT_EQ(SafGuiFence::PreloadAgentAppStatus(), SAF_SUCCESS);
    ASSERT_TRUE(GuiCacheManager::GetInstance().IsInitialized());

    // Tamper with the cached user ID so it no longer matches the foreground user,
    // simulating a user switch. MISMATCHED_USER_ID is a value the real account
    // manager will never return, so the comparison is guaranteed to mismatch.
    GuiCacheManager::GetInstance().SetCachedUserId(MISMATCHED_USER_ID);
    ASSERT_EQ(GuiCacheManager::GetInstance().GetCachedUserId(), MISMATCHED_USER_ID);

    // Preload must detect the mismatch and re-run InitCacheFromService.
    int32_t ret = SafGuiFence::PreloadAgentAppStatus();
    EXPECT_EQ(ret, SAF_SUCCESS);
    // After the re-fetch, cachedUserId must be updated back to the real user.
    EXPECT_NE(GuiCacheManager::GetInstance().GetCachedUserId(), MISMATCHED_USER_ID);
}

} // namespace UnitTest::GuiInnerApiTest