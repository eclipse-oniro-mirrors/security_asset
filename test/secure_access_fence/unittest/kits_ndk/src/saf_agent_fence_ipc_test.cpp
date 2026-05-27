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

#include "saf_agent_fence_ipc_test.h"

#include <gtest/gtest.h>
#include <functional>
#include <mutex>

#include "saf_result_code.h"
#include "isecure_access_fence.h"
#include "saf_agent_params_checker.h"
#include "iservice_registry.h"
#include "../../../../../frameworks/secure_access_fence/inner_api/agent_fence/src/saf_agent_fence.cpp"

using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::Security::SAF;

namespace UnitTest::SafAgentFenceIpcTest {

class SafAgentFenceIpcTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
};

void SafAgentFenceIpcTest::SetUpTestCase(void)
{
}

void SafAgentFenceIpcTest::TearDownTestCase(void)
{
}

void SafAgentFenceIpcTest::SetUp(void)
{
}

void SafAgentFenceIpcTest::TearDown(void)
{
}

HWTEST_F(SafAgentFenceIpcTest, GetProxyTest001, TestSize.Level0)
{
    auto proxy = GetProxy(g_mutex, true);
    EXPECT_NE(proxy, nullptr);
}

HWTEST_F(SafAgentFenceIpcTest, GetProxyTest002, TestSize.Level0)
{
    auto proxy = GetProxy(g_mutex, false);
    EXPECT_NE(proxy, nullptr);
}

HWTEST_F(SafAgentFenceIpcTest, HandleIpcErrorTest001, TestSize.Level0)
{
    OHOS::sptr<OHOS::Security::SAF::ISecureAccessFence> proxy = nullptr;
    std::function<int32_t()> call = nullptr;

    int32_t result = HandleIpcError(proxy, call);
    EXPECT_EQ(result, SAF_ERR_SERVICE_UNAVAILABLE);
}

HWTEST_F(SafAgentFenceIpcTest, HandleIpcErrorTest002, TestSize.Level0)
{
    OHOS::sptr<OHOS::Security::SAF::ISecureAccessFence> proxy = nullptr;
    std::function<int32_t()> call = []() { return ERR_DEAD_OBJECT; };

    int32_t result = HandleIpcError(proxy, call);
    EXPECT_EQ(result, SAF_ERR_SERVICE_UNAVAILABLE);

    call = []() { return ERR_REMOTE_DEAD; };
    result = HandleIpcError(proxy, call);
    EXPECT_EQ(result, SAF_ERR_SERVICE_UNAVAILABLE);
}

HWTEST_F(SafAgentFenceIpcTest, HandleIpcErrorTest003, TestSize.Level0)
{
    OHOS::sptr<OHOS::Security::SAF::ISecureAccessFence> proxy = nullptr;
    std::function<int32_t()> call = []() { return 29189; };

    int32_t result = HandleIpcError(proxy, call);
    EXPECT_EQ(result, SAF_ERR_SERVICE_UNAVAILABLE);
}

HWTEST_F(SafAgentFenceIpcTest, HandleIpcErrorTest004, TestSize.Level0)
{
    OHOS::sptr<OHOS::Security::SAF::ISecureAccessFence> proxy = nullptr;
    std::function<int32_t()> call = []() { return SAF_ERR_SERVICE_IS_STOPPING; };

    int32_t result = HandleIpcError(proxy, call);
    EXPECT_NE(result, SAF_ERR_SERVICE_UNAVAILABLE);
}

HWTEST_F(SafAgentFenceIpcTest, HandleIpcErrorTest005, TestSize.Level0)
{
    OHOS::sptr<OHOS::Security::SAF::ISecureAccessFence> proxy = nullptr;
    std::function<int32_t()> call = []() { return SAF_SUCCESS; };

    int32_t result = HandleIpcError(proxy, call);
    EXPECT_EQ(result, SAF_SUCCESS);
}

HWTEST_F(SafAgentFenceIpcTest, HandleIpcErrorTest006, TestSize.Level0)
{
    OHOS::sptr<OHOS::Security::SAF::ISecureAccessFence> proxy = nullptr;
    std::function<int32_t()> call = []() { return -1; };

    int32_t result = HandleIpcError(proxy, call);
    EXPECT_EQ(result, -1);
}

HWTEST_F(SafAgentFenceIpcTest, HandleIpcErrorTest007, TestSize.Level0)
{
    OHOS::sptr<OHOS::Security::SAF::ISecureAccessFence> proxy = nullptr;
    std::function<int32_t()> call = []() { return SAF_ERR_IPC_ERROR; };

    int32_t result = HandleIpcError(proxy, call);
    EXPECT_EQ(result, SAF_ERR_IPC_ERROR);
}

}