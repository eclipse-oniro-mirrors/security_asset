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

#include <atomic>
#include <gtest/gtest.h>
#include <string>

#include "settings_observer.h"

using namespace testing::ext;
using namespace OHOS::Security::SAF;
using OHOS::sptr;

namespace UnitTest::SettingsObserverTest {

class SettingsObserverTest : public testing::Test {
public:
    void SetUp() override {}
    void TearDown() override {}
};

// ============ SettingsDataObserver::OnChange ============

/**
 * @tc.name: SettingsDataObserver_OnChange_001
 * @tc.desc: UT-O-01: OnChange invokes the registered callback when callback is non-empty.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SettingsObserverTest, SettingsDataObserver_OnChange_001, TestSize.Level0)
{
    std::atomic<bool> callbackInvoked(false);
    auto observer = sptr<SettingsDataObserver>::MakeSptr([&callbackInvoked]() {
        callbackInvoked = true;
    });
    ASSERT_NE(observer, nullptr);

    observer->OnChange();
    EXPECT_TRUE(callbackInvoked.load());
}

/**
 * @tc.name: SettingsDataObserver_OnChange_002
 * @tc.desc: UT-O-02: OnChange with an empty (default) callback does not crash and returns normally.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SettingsObserverTest, SettingsDataObserver_OnChange_002, TestSize.Level0)
{
    // Construct observer with an empty std::function (falsy). OnChange must skip the call.
    auto observer = sptr<SettingsDataObserver>::MakeSptr(SettingsDataObserver::OnChangeCallback{});
    ASSERT_NE(observer, nullptr);

    // Should not throw or crash; the falsy callback branch is exercised.
    observer->OnChange();
    SUCCEED();
}

/**
 * @tc.name: SettingsDataObserver_OnChange_003
 * @tc.desc: UT-O-03: OnChange can be called multiple times, each invoking the callback.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SettingsObserverTest, SettingsDataObserver_OnChange_003, TestSize.Level0)
{
    std::atomic<int> callCount(0);
    auto observer = sptr<SettingsDataObserver>::MakeSptr([&callCount]() {
        callCount++;
    });
    ASSERT_NE(observer, nullptr);

    observer->OnChange();
    observer->OnChange();
    observer->OnChange();
    EXPECT_EQ(callCount.load(), 3);
}

// ============ RegisterSettingsObserver ============

/**
 * @tc.name: RegisterSettingsObserver_NullObserver_001
 * @tc.desc: UT-O-04: RegisterSettingsObserver with null observer returns false (early-return branch).
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SettingsObserverTest, RegisterSettingsObserver_NullObserver_001, TestSize.Level0)
{
    sptr<SettingsDataObserver> nullObserver = nullptr;
    bool ret = RegisterSettingsObserver("agent_app_control_status", 100, nullObserver);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: RegisterSettingsObserver_NoDataShareHelper_001
 * @tc.desc: UT-O-05: RegisterSettingsObserver with a valid observer but no DataShareHelper available
 *                   (no samgr / SA not loaded in unit-test env) returns false. This covers the
 *                   CreateDataShareHelper failure -> helper == nullptr branch.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SettingsObserverTest, RegisterSettingsObserver_NoDataShareHelper_001, TestSize.Level0)
{
    auto observer = sptr<SettingsDataObserver>::MakeSptr([]() {});
    ASSERT_NE(observer, nullptr);

    // In a unit-test environment there is no running samgr/datashare service, so
    // CreateDataShareHelper returns nullptr and RegisterSettingsObserver returns false.
    bool ret = RegisterSettingsObserver("agent_app_control_status", 100, observer);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: RegisterSettingsObserver_EmptyKey_001
 * @tc.desc: UT-O-06: RegisterSettingsObserver with an empty key still reaches the helper-creation
 *                   path (empty key is not rejected by the function itself); returns false because
 *                   the helper cannot be created in the unit-test environment.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SettingsObserverTest, RegisterSettingsObserver_EmptyKey_001, TestSize.Level0)
{
    auto observer = sptr<SettingsDataObserver>::MakeSptr([]() {});
    ASSERT_NE(observer, nullptr);

    bool ret = RegisterSettingsObserver("", 100, observer);
    EXPECT_FALSE(ret);
}

} // namespace UnitTest::SettingsObserverTest
