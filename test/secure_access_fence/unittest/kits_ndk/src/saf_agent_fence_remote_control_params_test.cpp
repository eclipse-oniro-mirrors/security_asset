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

#include "saf_agent_fence_remote_control_params_test.h"

#include <gtest/gtest.h>

#include "saf_agent_fence.h"
#include "saf_result_code.h"

using namespace testing::ext;
namespace UnitTest::SafAgentFenceRemoteControlParamsTest {

class SafAgentFenceRemoteControlParamsTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
};

void SafAgentFenceRemoteControlParamsTest::SetUpTestCase(void)
{
}

void SafAgentFenceRemoteControlParamsTest::TearDownTestCase(void)
{
}

void SafAgentFenceRemoteControlParamsTest::SetUp(void)
{
}

void SafAgentFenceRemoteControlParamsTest::TearDown(void)
{
}

// Mirrors saf_definition::RemoteGrantStatus: Enable = 1, Disable = 2.
constexpr int32_t REMOTE_GRANT_STATUS_ENABLE = 1;
constexpr int32_t REMOTE_GRANT_STATUS_DISABLE = 2;

/**
 * @tc.name: GetRemoteGrantStatusTest001
 * @tc.desc: GetRemoteGrantStatus is not supported, expect SAF_ERR_PLUGIN_NOT_SUPPORT.
 * @tc.type: FUNC
 * @tc.result: SAF_ERR_PLUGIN_NOT_SUPPORT
 */
HWTEST_F(SafAgentFenceRemoteControlParamsTest, GetRemoteGrantStatusTest001, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    int32_t remoteGrantStatus = 0;
    int32_t result = agentFence.GetRemoteGrantStatus(remoteGrantStatus);
    EXPECT_EQ(result, SAF_ERR_PLUGIN_NOT_SUPPORT);
}

/**
 * @tc.name: UpdateRemoteGrantStatusTest001
 * @tc.desc: UpdateRemoteGrantStatus with Enable is not supported, expect SAF_ERR_PLUGIN_NOT_SUPPORT.
 * @tc.type: FUNC
 * @tc.result: SAF_ERR_PLUGIN_NOT_SUPPORT
 */
HWTEST_F(SafAgentFenceRemoteControlParamsTest, UpdateRemoteGrantStatusTest001, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    int32_t result = agentFence.UpdateRemoteGrantStatus(REMOTE_GRANT_STATUS_ENABLE);
    EXPECT_EQ(result, SAF_ERR_PLUGIN_NOT_SUPPORT);
}

/**
 * @tc.name: UpdateRemoteGrantStatusTest002
 * @tc.desc: UpdateRemoteGrantStatus with Disable is not supported, expect SAF_ERR_PLUGIN_NOT_SUPPORT.
 * @tc.type: FUNC
 * @tc.result: SAF_ERR_PLUGIN_NOT_SUPPORT
 */
HWTEST_F(SafAgentFenceRemoteControlParamsTest, UpdateRemoteGrantStatusTest002, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    int32_t result = agentFence.UpdateRemoteGrantStatus(REMOTE_GRANT_STATUS_DISABLE);
    EXPECT_EQ(result, SAF_ERR_PLUGIN_NOT_SUPPORT);
}

/**
 * @tc.name: UpdateRemoteGrantStatusTest003
 * @tc.desc: UpdateRemoteGrantStatus with status 0, expect SAF_ERR_ARG_INVALID.
 * @tc.type: FUNC
 * @tc.result: SAF_ERR_ARG_INVALID
 */
HWTEST_F(SafAgentFenceRemoteControlParamsTest, UpdateRemoteGrantStatusTest003, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    int32_t result = agentFence.UpdateRemoteGrantStatus(0);
    EXPECT_EQ(result, SAF_ERR_ARG_INVALID);
}

/**
 * @tc.name: UpdateRemoteGrantStatusTest004
 * @tc.desc: UpdateRemoteGrantStatus with status 3, expect SAF_ERR_ARG_INVALID.
 * @tc.type: FUNC
 * @tc.result: SAF_ERR_ARG_INVALID
 */
HWTEST_F(SafAgentFenceRemoteControlParamsTest, UpdateRemoteGrantStatusTest004, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    int32_t result = agentFence.UpdateRemoteGrantStatus(3);
    EXPECT_EQ(result, SAF_ERR_ARG_INVALID);
}

/**
 * @tc.name: UpdateRemoteGrantStatusTest005
 * @tc.desc: UpdateRemoteGrantStatus with negative status, expect SAF_ERR_ARG_INVALID.
 * @tc.type: FUNC
 * @tc.result: SAF_ERR_ARG_INVALID
 */
HWTEST_F(SafAgentFenceRemoteControlParamsTest, UpdateRemoteGrantStatusTest005, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    int32_t result = agentFence.UpdateRemoteGrantStatus(-1);
    EXPECT_EQ(result, SAF_ERR_ARG_INVALID);
}

HWTEST_F(SafAgentFenceRemoteControlParamsTest, GenerateControlledDevicePackageParamsTest001, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    std::vector<OHOS::Security::SAF::PermissionQuery> queries;

    std::vector<OHOS::Security::SAF::RemoteAuthPackage> packages;
    int32_t result = agentFence.GenerateControlledDevicePackage(queries, packages);
    EXPECT_NE(result, SAF_SUCCESS);
}

HWTEST_F(SafAgentFenceRemoteControlParamsTest, GenerateControlledDevicePackageParamsTest002, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    std::vector<OHOS::Security::SAF::PermissionQuery> queries;
    for (int i = 0; i < 11; i++) {
        OHOS::Security::SAF::PermissionQuery query;
        queries.push_back(query);
    }

    std::vector<OHOS::Security::SAF::RemoteAuthPackage> packages;
    int32_t result = agentFence.GenerateControlledDevicePackage(queries, packages);
    EXPECT_NE(result, SAF_SUCCESS);
}

HWTEST_F(SafAgentFenceRemoteControlParamsTest, GenerateControlledDevicePackageParamsTest003, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    std::vector<OHOS::Security::SAF::PermissionQuery> queries;
    OHOS::Security::SAF::PermissionQuery query;
    query.callerTokenId = 0;
    queries.push_back(query);

    std::vector<OHOS::Security::SAF::RemoteAuthPackage> packages;
    int32_t result = agentFence.GenerateControlledDevicePackage(queries, packages);
    EXPECT_NE(result, SAF_SUCCESS);
}

HWTEST_F(SafAgentFenceRemoteControlParamsTest, GenerateControlledDevicePackageParamsTest004, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    std::vector<OHOS::Security::SAF::PermissionQuery> queries;
    OHOS::Security::SAF::PermissionQuery query;
    query.domainId = "";
    queries.push_back(query);

    std::vector<OHOS::Security::SAF::RemoteAuthPackage> packages;
    int32_t result = agentFence.GenerateControlledDevicePackage(queries, packages);
    EXPECT_NE(result, SAF_SUCCESS);
}

HWTEST_F(SafAgentFenceRemoteControlParamsTest, GenerateControlledDevicePackageParamsTest005, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    std::vector<OHOS::Security::SAF::PermissionQuery> queries;
    OHOS::Security::SAF::PermissionQuery query;
    OHOS::Security::SAF::RemoteInfo remoteInfo;
    remoteInfo.role = OHOS::Security::SAF::Role::CONTROLLED;
    query.remoteInfo = remoteInfo;
    queries.push_back(query);

    std::vector<OHOS::Security::SAF::RemoteAuthPackage> packages;
    int32_t result = agentFence.GenerateControlledDevicePackage(queries, packages);
    EXPECT_NE(result, SAF_SUCCESS);
}

HWTEST_F(SafAgentFenceRemoteControlParamsTest, GenerateControlledDevicePackageParamsTest006, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    std::vector<OHOS::Security::SAF::PermissionQuery> queries;
    OHOS::Security::SAF::PermissionQuery query;
    OHOS::Security::SAF::RemoteInfo remoteInfo;
    remoteInfo.role = OHOS::Security::SAF::Role::CONTROLLER;
    query.remoteInfo = remoteInfo;
    queries.push_back(query);

    std::vector<OHOS::Security::SAF::RemoteAuthPackage> packages;
    int32_t result = agentFence.GenerateControlledDevicePackage(queries, packages);
    EXPECT_NE(result, SAF_SUCCESS);
}

HWTEST_F(SafAgentFenceRemoteControlParamsTest, GenerateControlledDevicePackageParamsTest007, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    std::vector<OHOS::Security::SAF::PermissionQuery> queries;
    OHOS::Security::SAF::PermissionQuery query;
    OHOS::Security::SAF::RemoteInfo remoteInfo;
    remoteInfo.role = OHOS::Security::SAF::Role::CONTROLLED;
    OHOS::Security::SAF::RemoteControlParams remoteControlParams;
    remoteControlParams.challenge = "";
    remoteInfo.remoteControlParams = remoteControlParams;
    query.remoteInfo = remoteInfo;
    queries.push_back(query);

    std::vector<OHOS::Security::SAF::RemoteAuthPackage> packages;
    int32_t result = agentFence.GenerateControlledDevicePackage(queries, packages);
    EXPECT_NE(result, SAF_SUCCESS);
}

HWTEST_F(SafAgentFenceRemoteControlParamsTest, VerifyControlledDevicePackageParamsTest001, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    std::vector<OHOS::Security::SAF::RemoteAuthPackage> packages;

    std::vector<bool> verifyRes;
    int32_t result = agentFence.VerifyControlledDevicePackage(packages, verifyRes);
    EXPECT_NE(result, SAF_SUCCESS);
}

HWTEST_F(SafAgentFenceRemoteControlParamsTest, VerifyControlledDevicePackageParamsTest002, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    std::vector<OHOS::Security::SAF::RemoteAuthPackage> packages;
    for (int i = 0; i < 11; i++) {
        OHOS::Security::SAF::RemoteAuthPackage pkg;
        packages.push_back(pkg);
    }

    std::vector<bool> verifyRes;
    int32_t result = agentFence.VerifyControlledDevicePackage(packages, verifyRes);
    EXPECT_NE(result, SAF_SUCCESS);
}

HWTEST_F(SafAgentFenceRemoteControlParamsTest, VerifyControlledDevicePackageParamsTest003, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    std::vector<OHOS::Security::SAF::RemoteAuthPackage> packages;
    OHOS::Security::SAF::RemoteAuthPackage pkg;
    pkg.remoteMessage = "";
    packages.push_back(pkg);

    std::vector<bool> verifyRes;
    int32_t result = agentFence.VerifyControlledDevicePackage(packages, verifyRes);
    EXPECT_NE(result, SAF_SUCCESS);
}

HWTEST_F(SafAgentFenceRemoteControlParamsTest, VerifyControlledDevicePackageParamsTest004, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    std::vector<OHOS::Security::SAF::RemoteAuthPackage> packages;
    OHOS::Security::SAF::RemoteAuthPackage pkg;
    pkg.challenge = "invalid_challenge";
    packages.push_back(pkg);

    std::vector<bool> verifyRes;
    int32_t result = agentFence.VerifyControlledDevicePackage(packages, verifyRes);
    EXPECT_NE(result, SAF_SUCCESS);
}

HWTEST_F(SafAgentFenceRemoteControlParamsTest, GenerateControllerDevicePackageParamsTest001, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    std::vector<OHOS::Security::SAF::RemoteUserAuthResults> authResults;

    std::vector<OHOS::Security::SAF::RemoteAuthPackage> packages;
    int32_t result = agentFence.GenerateControllerDevicePackage(authResults, packages);
    EXPECT_NE(result, SAF_SUCCESS);
}

HWTEST_F(SafAgentFenceRemoteControlParamsTest, GenerateControllerDevicePackageParamsTest002, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    std::vector<OHOS::Security::SAF::RemoteUserAuthResults> authResults;
    for (int i = 0; i < 11; i++) {
        OHOS::Security::SAF::RemoteUserAuthResults authResult;
        authResults.push_back(authResult);
    }

    std::vector<OHOS::Security::SAF::RemoteAuthPackage> packages;
    int32_t result = agentFence.GenerateControllerDevicePackage(authResults, packages);
    EXPECT_NE(result, SAF_SUCCESS);
}

HWTEST_F(SafAgentFenceRemoteControlParamsTest, GenerateControllerDevicePackageParamsTest003, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    std::vector<OHOS::Security::SAF::RemoteUserAuthResults> authResults;
    OHOS::Security::SAF::RemoteUserAuthResults authResult;
    authResult.results.clear();
    authResults.push_back(authResult);

    std::vector<OHOS::Security::SAF::RemoteAuthPackage> packages;
    int32_t result = agentFence.GenerateControllerDevicePackage(authResults, packages);
    EXPECT_NE(result, SAF_SUCCESS);
}

HWTEST_F(SafAgentFenceRemoteControlParamsTest, GenerateControllerDevicePackageParamsTest004, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    std::vector<OHOS::Security::SAF::RemoteUserAuthResults> authResults;
    OHOS::Security::SAF::RemoteUserAuthResults authResult;
    OHOS::Security::SAF::RemoteUserAuthItem item;
    item.permission = "";
    authResult.results.push_back(item);
    authResults.push_back(authResult);

    std::vector<OHOS::Security::SAF::RemoteAuthPackage> packages;
    int32_t result = agentFence.GenerateControllerDevicePackage(authResults, packages);
    EXPECT_NE(result, SAF_SUCCESS);
}

HWTEST_F(SafAgentFenceRemoteControlParamsTest, GenerateControllerDevicePackageParamsTest005, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    std::vector<OHOS::Security::SAF::RemoteUserAuthResults> authResults;
    OHOS::Security::SAF::RemoteUserAuthResults authResult;
    OHOS::Security::SAF::RemoteUserAuthItem item;
    item.permission = "ohos.permission.TEST";
    item.authResult = "";
    authResult.results.push_back(item);
    authResults.push_back(authResult);

    std::vector<OHOS::Security::SAF::RemoteAuthPackage> packages;
    int32_t result = agentFence.GenerateControllerDevicePackage(authResults, packages);
    EXPECT_NE(result, SAF_SUCCESS);
}

HWTEST_F(SafAgentFenceRemoteControlParamsTest, GenerateControllerDevicePackageParamsTest006, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    std::vector<OHOS::Security::SAF::RemoteUserAuthResults> authResults;
    OHOS::Security::SAF::RemoteUserAuthResults authResult;
    OHOS::Security::SAF::RemoteUserAuthItem item;
    item.permission = "ohos.permission.TEST";
    item.authResult = "granted";
    authResult.results.push_back(item);
    authResult.permissionQuery.remoteInfo.role = OHOS::Security::SAF::Role::CONTROLLER;
    authResults.push_back(authResult);

    std::vector<OHOS::Security::SAF::RemoteAuthPackage> packages;
    int32_t result = agentFence.GenerateControllerDevicePackage(authResults, packages);
    EXPECT_NE(result, SAF_SUCCESS);
}

HWTEST_F(SafAgentFenceRemoteControlParamsTest, GenerateControllerDevicePackageParamsTest007, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    std::vector<OHOS::Security::SAF::RemoteUserAuthResults> authResults;
    OHOS::Security::SAF::RemoteUserAuthResults authResult;
    OHOS::Security::SAF::RemoteUserAuthItem item;
    item.permission = "ohos.permission.TEST";
    item.authResult = "granted";
    authResult.results.push_back(item);
    authResult.permissionQuery.remoteInfo.role = OHOS::Security::SAF::Role::CONTROLLED;
    authResults.push_back(authResult);

    std::vector<OHOS::Security::SAF::RemoteAuthPackage> packages;
    int32_t result = agentFence.GenerateControllerDevicePackage(authResults, packages);
    EXPECT_NE(result, SAF_SUCCESS);
}

HWTEST_F(SafAgentFenceRemoteControlParamsTest, VerifyControllerDevicePackageParamsTest001, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    std::vector<OHOS::Security::SAF::RemoteAuthPackage> packages;
    OHOS::Security::SAF::RemoteInfo remoteInfo;
    remoteInfo.role = OHOS::Security::SAF::Role::CONTROLLER;

    std::vector<bool> verifyRes;
    int32_t result = agentFence.VerifyControllerDevicePackage(packages, remoteInfo, verifyRes);
    EXPECT_NE(result, SAF_SUCCESS);
}

HWTEST_F(SafAgentFenceRemoteControlParamsTest, VerifyControllerDevicePackageParamsTest002, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    std::vector<OHOS::Security::SAF::RemoteAuthPackage> packages;
    for (int i = 0; i < 11; i++) {
        OHOS::Security::SAF::RemoteAuthPackage pkg;
        packages.push_back(pkg);
    }
    OHOS::Security::SAF::RemoteInfo remoteInfo;
    remoteInfo.role = OHOS::Security::SAF::Role::CONTROLLER;

    std::vector<bool> verifyRes;
    int32_t result = agentFence.VerifyControllerDevicePackage(packages, remoteInfo, verifyRes);
    EXPECT_NE(result, SAF_SUCCESS);
}

HWTEST_F(SafAgentFenceRemoteControlParamsTest, VerifyControllerDevicePackageParamsTest003, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    std::vector<OHOS::Security::SAF::RemoteAuthPackage> packages;
    OHOS::Security::SAF::RemoteAuthPackage pkg;
    pkg.remoteMessage = "";
    packages.push_back(pkg);

    OHOS::Security::SAF::RemoteInfo remoteInfo;
    remoteInfo.role = OHOS::Security::SAF::Role::CONTROLLER;

    std::vector<bool> verifyRes;
    int32_t result = agentFence.VerifyControllerDevicePackage(packages, remoteInfo, verifyRes);
    EXPECT_NE(result, SAF_SUCCESS);
}

HWTEST_F(SafAgentFenceRemoteControlParamsTest, VerifyControllerDevicePackageParamsTest004, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    std::vector<OHOS::Security::SAF::RemoteAuthPackage> packages;
    OHOS::Security::SAF::RemoteAuthPackage pkg;
    pkg.remoteMessage = "valid_message";
    packages.push_back(pkg);

    OHOS::Security::SAF::RemoteInfo remoteInfo;
    remoteInfo.role = OHOS::Security::SAF::Role::CONTROLLED;

    std::vector<bool> verifyRes;
    int32_t result = agentFence.VerifyControllerDevicePackage(packages, remoteInfo, verifyRes);
    EXPECT_NE(result, SAF_SUCCESS);
}

HWTEST_F(SafAgentFenceRemoteControlParamsTest, VerifyControllerDevicePackageParamsTest005, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    std::vector<OHOS::Security::SAF::RemoteAuthPackage> packages;
    OHOS::Security::SAF::RemoteAuthPackage pkg;
    pkg.remoteMessage = "valid_message";
    packages.push_back(pkg);

    OHOS::Security::SAF::RemoteInfo remoteInfo;
    remoteInfo.role = OHOS::Security::SAF::Role::CONTROLLER;
    remoteInfo.remoteId = "";

    std::vector<bool> verifyRes;
    int32_t result = agentFence.VerifyControllerDevicePackage(packages, remoteInfo, verifyRes);
    EXPECT_NE(result, SAF_SUCCESS);
}

HWTEST_F(SafAgentFenceRemoteControlParamsTest, VerifyControllerDevicePackageParamsTest006, TestSize.Level0)
{
    OHOS::Security::SAF::SafAgentFence agentFence;

    std::vector<OHOS::Security::SAF::RemoteAuthPackage> packages;
    OHOS::Security::SAF::RemoteAuthPackage pkg;
    pkg.remoteMessage = "valid_message";
    packages.push_back(pkg);

    OHOS::Security::SAF::RemoteInfo remoteInfo;
    remoteInfo.role = OHOS::Security::SAF::Role::CONTROLLER;
    remoteInfo.remoteId = "test_remote_id";
    remoteInfo.remoteControlParams.challenge = "";

    std::vector<bool> verifyRes;
    int32_t result = agentFence.VerifyControllerDevicePackage(packages, remoteInfo, verifyRes);
    EXPECT_NE(result, SAF_SUCCESS);
}

}