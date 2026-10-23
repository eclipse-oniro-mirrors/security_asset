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

#include "accesstoken_kit.h"
#include "iservice_registry.h"
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

} // namespace UnitTest::GuiInnerApiTest