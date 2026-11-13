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
#include <string>
#include <vector>
#include "accesstoken_kit.h"
#include "ipc_skeleton.h"

#include "saf_agent_fence.h"
#include "saf_result_code.h"
#include "secure_access_fence_type.h"
#include "mock_permission.h"

using namespace testing::ext;
using namespace OHOS::Security::SAF;
using namespace OHOS::Security::AccessToken;

namespace UnitTest::SafAgentFenceControlAppStatusTest {

// Permission required by QueryControlAppStatus on the service side; without it
// the IPC handler returns SAF_ERR_PERMISSION_DENIED instead of SAF_SUCCESS.
const static std::vector<std::string> PERMISSIONS = {
    "ohos.permission.CONTROL_DEVICE",
};

namespace {
// A non-empty bundle name used to build a valid AppInfo for IPC calls.
const std::string TEST_BUNDLE_NAME_A = "com.app.control.caller";
// A second bundle name used as the target in a two-element query list.
const std::string TEST_BUNDLE_NAME_B = "com.app.control.target";
// Application index used together with the bundle names above.
constexpr int32_t TEST_APP_INDEX = 0;

// Returns true if ohos.permission.CONTROL_DEVICE is available on the current device.
// This permission is only registered on 2in1 (PC) devices.
bool IsControlDevicePermissionAvailable()
{
    auto tokenId = OHOS::IPCSkeleton::GetCallingTokenID();
    return AccessTokenKit::VerifyAccessToken(tokenId, "ohos.permission.CONTROL_DEVICE") ==
        PERMISSION_GRANTED;
}
} // namespace

class SafAgentFenceControlAppStatusTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() override {}
    void TearDown() override {}
    static MockToken* mockToken_;

protected:
    // Build an AppInfo with the given bundle name and index.
    static AppInfo MakeAppInfo(const std::string &bundleName, int32_t index)
    {
        AppInfo app;
        app.bundleName = bundleName;
        app.index = index;
        return app;
    }
};

MockToken* SafAgentFenceControlAppStatusTest::mockToken_ = nullptr;

void SafAgentFenceControlAppStatusTest::SetUpTestCase(void)
{
    mockToken_ = new MockToken(PERMISSIONS, true, true);
    ASSERT_NE(mockToken_, nullptr);

    std::string errorMsg = mockToken_->GetMockErrorMsg();
    if (errorMsg != "success") {
        ASSERT_NE(mockToken_->GetTokenId(), INVALID_TOKENID) << "MockToken failed: " << errorMsg;
    } else {
        ASSERT_NE(mockToken_->GetTokenId(), INVALID_TOKENID) << "Failed to create MockToken";
    }
}

void SafAgentFenceControlAppStatusTest::TearDownTestCase(void)
{
    if (mockToken_ != nullptr) {
        delete mockToken_;
        mockToken_ = nullptr;
    }
}

// ============ QueryControlAppStatus ============

/**
 * @tc.name: QueryControlAppStatus_Success_001
 * @tc.desc: Query with a single AppInfo on a device where the SAF service is available;
 *           the IPC call succeeds and returns SAF_SUCCESS.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SafAgentFenceControlAppStatusTest, QueryControlAppStatus_Success_001, TestSize.Level0)
{
    std::vector<AppInfo> appInfoList = { MakeAppInfo(TEST_BUNDLE_NAME_A, TEST_APP_INDEX) };
    std::vector<ControlAppStatusResult> resultList;
    int32_t ret = SafAgentFence::QueryControlAppStatus(appInfoList, resultList);
    if (IsControlDevicePermissionAvailable()) {
        EXPECT_EQ(ret, SAF_SUCCESS);
        EXPECT_EQ(resultList.size(), appInfoList.size());
    } else {
        EXPECT_EQ(ret, SAF_ERR_PERMISSION_DENIED);
    }
}

/**
 * @tc.name: QueryControlAppStatus_Success_Multiple_001
 * @tc.desc: Query with multiple AppInfo entries; each entry gets a corresponding result.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SafAgentFenceControlAppStatusTest, QueryControlAppStatus_Success_Multiple_001, TestSize.Level0)
{
    std::vector<AppInfo> appInfoList = {
        MakeAppInfo(TEST_BUNDLE_NAME_A, TEST_APP_INDEX),
        MakeAppInfo(TEST_BUNDLE_NAME_B, TEST_APP_INDEX),
    };
    std::vector<ControlAppStatusResult> resultList;
    int32_t ret = SafAgentFence::QueryControlAppStatus(appInfoList, resultList);
    if (IsControlDevicePermissionAvailable()) {
        EXPECT_EQ(ret, SAF_SUCCESS);
        EXPECT_EQ(resultList.size(), appInfoList.size());
    } else {
        EXPECT_EQ(ret, SAF_ERR_PERMISSION_DENIED);
    }
}

/**
 * @tc.name: QueryControlAppStatus_EmptyInput_001
 * @tc.desc: Query with an empty AppInfo list; the service rejects it with SAF_ERR_ARG_EMPTY
 *           and leaves the result list empty (covers the zero-length input path through IPC).
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SafAgentFenceControlAppStatusTest, QueryControlAppStatus_EmptyInput_001, TestSize.Level0)
{
    std::vector<AppInfo> appInfoList;
    std::vector<ControlAppStatusResult> resultList;
    int32_t ret = SafAgentFence::QueryControlAppStatus(appInfoList, resultList);
    if (IsControlDevicePermissionAvailable()) {
        EXPECT_EQ(ret, SAF_ERR_ARG_EMPTY);
        EXPECT_TRUE(resultList.empty());
    } else {
        EXPECT_EQ(ret, SAF_ERR_PERMISSION_DENIED);
    }
}

// ============ GetControlDialogBundleName ============

/**
 * @tc.name: GetControlDialogBundleName_Success_001
 * @tc.desc: GetControlDialogBundleName on a device where the SAF service is available;
 *           the IPC call succeeds, returns SAF_SUCCESS, and fills a non-empty bundle name.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SafAgentFenceControlAppStatusTest, GetControlDialogBundleName_Success_001, TestSize.Level0)
{
    std::string bundleName;
    int32_t ret = SafAgentFence::GetControlDialogBundleName(bundleName);
    EXPECT_EQ(ret, SAF_SUCCESS);
    EXPECT_FALSE(bundleName.empty());
}

} // namespace UnitTest::SafAgentFenceControlAppStatusTest
