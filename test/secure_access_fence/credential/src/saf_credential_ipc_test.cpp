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

// White-box unit tests for the credential inner API client
// (frameworks/secure_access_fence/inner_api/credential/src/saf_credential.cpp).
//
// Why mocks instead of the real system ability:
//   saf_credential.cpp has no seam between its public methods and samgr/IPC:
//   GetProxy() talks to SystemAbilityManagerClient and HandleIpcError() drives
//   the ISecureAccessFence proxy. The functional tests (saf_credential_test)
//   can only observe whatever the SA deployed on the device happens to do, so
//   the retry-on-stopping, dead-object and load-failure branches are never
//   reached deterministically on a device whose SA lacks the credential
//   backend. This target compiles saf_credential.cpp and
//   credential_inner_helper.cpp directly into the test binary and replaces the
//   two system headers it includes (isecure_access_fence.h,
//   iservice_registry.h) with the mock/ versions. Production code is not
//   modified.
//
// Branch coverage notes:
//   - Every public API is driven through: no samgr manager, initial load
//     failure, initial load success, CheckSystemAbility hit, business error
//     returned in resultCode, transport error, ERR_DEAD_OBJECT, the
//     ERR_REMOTE_DEAD literal, stopping -> reload success -> retry success,
//     stopping -> reload failure, stopping -> retry transport error,
//     stopping -> retry business error and stopping -> retry with no samgr.
//   - The mock records every samgr/IPC interaction, so each case asserts the
//     exact path taken (which branch was entered) rather than only the return
//     value. LOGI markers named [CRED-UT] are streamed to hilog and can be
//     matched on-device against the case that produced them.
//   - The only unreachable branch is HandleIpcError's `call == nullptr` guard:
//     every caller passes a non-empty lambda, and the helper is in an
//     anonymous namespace so a test cannot call it directly. Covering it would
//     require changing production code (exporting the helper), which is out of
//     scope; it is reported as a known-uncovered defensive branch.

#include <cstdint>
#include <deque>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "errors.h"

#include "isecure_access_fence.h"
#include "iservice_registry.h"
#include "saf_credential.h"
#include "saf_log.h"
#include "saf_result_code.h"

using namespace testing::ext;

namespace OHOS::Security::SAF {
namespace {

// ===== Constants mirrored from saf_credential.cpp =====

// SAF service id and load timeout are private to saf_credential.cpp; the test
// pins their values through the mock samgr so a change is caught here.
constexpr int32_t SAF_SERVICE_ID = 66532;
constexpr int32_t WAIT_TIMEOUT_IN_SEC = 5;

// Literal used by the IPC layer for a dead remote object (mirrors
// credential_inner_helper.cpp; ERR_DEAD_OBJECT comes from errors.h).
constexpr int32_t ERR_REMOTE_DEAD = 29189;

// IPC command codes from ISecureAccessFence.idl; used to verify that each
// public API drives its own proxy method.
constexpr int32_t IPC_ADD_MODEL_CREDENTIAL = 1300;
constexpr int32_t IPC_DELETE_MODEL_CREDENTIAL = 1301;
constexpr int32_t IPC_UPDATE_MODEL_CREDENTIAL = 1302;
constexpr int32_t IPC_GET_ACTIVE_MODEL_CREDENTIAL = 1303;
constexpr int32_t IPC_PRE_GET_ALL_MODEL_CREDENTIALS = 1304;
constexpr int32_t IPC_GET_MODEL_CREDENTIAL_VALUE = 1309;
constexpr int32_t IPC_ADD_TOOL_CREDENTIAL = 1305;
constexpr int32_t IPC_UPDATE_TOOL_CREDENTIAL = 1306;
constexpr int32_t IPC_DELETE_TOOL_CREDENTIAL = 1307;
constexpr int32_t IPC_GET_TOOL_CREDENTIAL = 1308;

// Values written by the mock proxy into the out-parameters on success, so the
// test can prove the references were passed through unchanged.
constexpr char MOCK_HANDLE[] = "mock-handle";
constexpr char MOCK_CREDENTIAL_ID[] = "mock-credential-id";
constexpr char MOCK_MODEL_URL[] = "https://mock.model.endpoint/v1";
constexpr char MOCK_SECRET[] = "mock-secret-value";
constexpr char MOCK_TOOL_ENV[] = "MOCK_ENV";

// ===== Mock IPC reply and proxy =====

struct IpcReply {
    int32_t ret = SAF_SUCCESS;
    int32_t resultCode = SAF_SUCCESS;
};

/**
 * @brief Scriptable stand-in for the generated SecureAccessFenceProxy.
 *
 * replies_ is a FIFO: each proxied call consumes one reply, so a case can
 * script e.g. "first call reports STOPPING, retried call succeeds". The proxy
 * also records how many calls were made and the last IPC code, which lets the
 * test assert which branch of HandleIpcError was executed.
 */
class MockCredentialProxy : public ISecureAccessFence {
public:
    MockCredentialProxy() = default;
    explicit MockCredentialProxy(std::initializer_list<IpcReply> replies) : replies_(replies) {}

    int32_t AddModelCredential(const ModelCredentialContent &, const ModelCredentialInfo &,
                               std::string &credentialHandle, std::string &credentialId,
                               int32_t &resultCode) override
    {
        const IpcReply reply = BeginCall(IPC_ADD_MODEL_CREDENTIAL, resultCode);
        if (IsServiceSuccess(reply)) {
            credentialHandle = MOCK_HANDLE;
            credentialId = MOCK_CREDENTIAL_ID;
        }
        return reply.ret;
    }

    int32_t DeleteModelCredential(const std::vector<ModelCredential> &, CredentialBatchResult &result,
                                  int32_t &resultCode) override
    {
        const IpcReply reply = BeginCall(IPC_DELETE_MODEL_CREDENTIAL, resultCode);
        if (IsServiceSuccess(reply)) {
            FillBatchResult(result);
        }
        return reply.ret;
    }

    int32_t UpdateModelCredential(const std::vector<ModelCredential> &, const std::vector<ModelCredential> &,
                                  CredentialBatchResult &result, int32_t &resultCode) override
    {
        const IpcReply reply = BeginCall(IPC_UPDATE_MODEL_CREDENTIAL, resultCode);
        if (IsServiceSuccess(reply)) {
            FillBatchResult(result);
        }
        return reply.ret;
    }

    int32_t GetActiveModelCredential(const std::string &, const std::string &,
                                     ModelCredentialContent &modelCredentialContent,
                                     int32_t &resultCode) override
    {
        const IpcReply reply = BeginCall(IPC_GET_ACTIVE_MODEL_CREDENTIAL, resultCode);
        if (IsServiceSuccess(reply)) {
            modelCredentialContent.baseUrl = MOCK_MODEL_URL;
            modelCredentialContent.credentialValue = MOCK_SECRET;
        }
        return reply.ret;
    }

    int32_t PreGetAllModelCredentials(std::vector<ModelCredential> &modelCredentials,
                                      int32_t &resultCode) override
    {
        const IpcReply reply = BeginCall(IPC_PRE_GET_ALL_MODEL_CREDENTIALS, resultCode);
        if (IsServiceSuccess(reply)) {
            ModelCredential entry;
            entry.modelCredentialContent.baseUrl = MOCK_MODEL_URL;
            entry.modelCredentialIndex.credentialHandle = MOCK_HANDLE;
            // The service strips credentialValue from this API's result.
            entry.modelCredentialContent.credentialValue.clear();
            modelCredentials.push_back(entry);
        }
        return reply.ret;
    }

    int32_t GetModelCredentialValue(const ModelCredentialIndex &, std::string &credentialValue,
                                    int32_t &resultCode) override
    {
        const IpcReply reply = BeginCall(IPC_GET_MODEL_CREDENTIAL_VALUE, resultCode);
        if (IsServiceSuccess(reply)) {
            credentialValue = MOCK_SECRET;
        }
        return reply.ret;
    }

    int32_t AddToolCredential(const std::vector<ToolCredential> &, CredentialBatchResult &result,
                              int32_t &resultCode) override
    {
        const IpcReply reply = BeginCall(IPC_ADD_TOOL_CREDENTIAL, resultCode);
        if (IsServiceSuccess(reply)) {
            FillBatchResult(result);
        }
        return reply.ret;
    }

    int32_t UpdateToolCredential(const std::vector<ToolCredential> &, CredentialBatchResult &result,
                                 int32_t &resultCode) override
    {
        const IpcReply reply = BeginCall(IPC_UPDATE_TOOL_CREDENTIAL, resultCode);
        if (IsServiceSuccess(reply)) {
            FillBatchResult(result);
        }
        return reply.ret;
    }

    int32_t DeleteToolCredential(const std::vector<ToolCredential> &, CredentialBatchResult &result,
                                 int32_t &resultCode) override
    {
        const IpcReply reply = BeginCall(IPC_DELETE_TOOL_CREDENTIAL, resultCode);
        if (IsServiceSuccess(reply)) {
            FillBatchResult(result);
        }
        return reply.ret;
    }

    int32_t GetToolCredential(const ToolInfo &, std::vector<ToolCredentialInfo> &toolCredentialInfos,
                              int32_t &resultCode) override
    {
        const IpcReply reply = BeginCall(IPC_GET_TOOL_CREDENTIAL, resultCode);
        if (IsServiceSuccess(reply)) {
            ToolCredentialInfo info;
            info.envName = MOCK_TOOL_ENV;
            info.credentialValue = MOCK_SECRET;
            toolCredentialInfos.push_back(info);
        }
        return reply.ret;
    }

    int32_t ipcCalls = 0;
    int32_t lastIpcCode = -1;

private:
    static bool IsServiceSuccess(const IpcReply &reply)
    {
        return reply.ret == SAF_SUCCESS && reply.resultCode == SAF_SUCCESS;
    }

    static void FillBatchResult(CredentialBatchResult &result)
    {
        result.failedCount = 1;
        ErrInfo info;
        info.index = 0;
        info.errCode = SAF_ERR_ARG_INVALID;
        info.message = "mock-failure";
        result.failedErrorInfos.push_back(info);
    }

    IpcReply NextReply()
    {
        if (replies_.empty()) {
            return IpcReply{};
        }
        IpcReply reply = replies_.front();
        replies_.pop_front();
        return reply;
    }

    IpcReply BeginCall(int32_t ipcCode, int32_t &resultCode)
    {
        ++ipcCalls;
        lastIpcCode = ipcCode;
        const IpcReply reply = NextReply();
        if (reply.ret == SAF_SUCCESS) {
            resultCode = reply.resultCode;
        }
        LOGI("[CRED-UT] mock ipc=%{public}d ret=%{public}d resultCode=%{public}d",
            ipcCode, reply.ret, reply.resultCode);
        return reply;
    }

    std::deque<IpcReply> replies_;
};

/**
 * @brief Scriptable ISystemAbilityManager.
 *
 * checkResults/loadResults are FIFOs consumed by CheckSystemAbility/
 * LoadSystemAbility; an empty queue models the corresponding call returning
 * null (SA absent / load failure).
 */
class MockSystemAbilityManager : public ISystemAbilityManager {
public:
    sptr<IRemoteObject> CheckSystemAbility(int32_t systemAbilityId) override
    {
        ++checkCalls;
        lastCheckId = systemAbilityId;
        sptr<IRemoteObject> result = PopFront(checkResults);
        LOGI("[CRED-UT] CheckSystemAbility id=%{public}d -> %{public}s", systemAbilityId,
             result == nullptr ? "null" : "object");
        return result;
    }

    sptr<IRemoteObject> LoadSystemAbility(int32_t systemAbilityId, int32_t timeoutSec) override
    {
        ++loadCalls;
        lastLoadId = systemAbilityId;
        lastLoadTimeout = timeoutSec;
        sptr<IRemoteObject> result = PopFront(loadResults);
        LOGI("[CRED-UT] LoadSystemAbility id=%{public}d timeout=%{public}d -> %{public}s", systemAbilityId,
             timeoutSec, result == nullptr ? "null" : "object");
        return result;
    }

    int32_t checkCalls = 0;
    int32_t loadCalls = 0;
    int32_t lastCheckId = -1;
    int32_t lastLoadId = -1;
    int32_t lastLoadTimeout = -1;
    std::deque<IRemoteObject *> checkResults;
    std::deque<IRemoteObject *> loadResults;

private:
    static sptr<IRemoteObject> PopFront(std::deque<IRemoteObject *> &queue)
    {
        if (queue.empty()) {
            return sptr<IRemoteObject>(nullptr);
        }
        IRemoteObject *object = queue.front();
        queue.pop_front();
        return sptr<IRemoteObject>(object);
    }
};

// ===== Process-wide mock samgr state =====

struct MockState {
    MockSystemAbilityManager samgr;
    // Number of GetSystemAbilityManager() calls that still return a manager.
    // A finite value models samgr becoming unavailable between the first call
    // and the retry (e.g. a dying samgr process).
    int32_t managerCallsRemaining = std::numeric_limits<int32_t>::max();
    int32_t managerGetCalls = 0;
};

MockState &GetMockState()
{
    static MockState state;
    return state;
}

}  // namespace
}  // namespace OHOS::Security::SAF

// ===== Mock samgr client definitions =====
//
// SystemAbilityManagerClient lives in namespace OHOS, so its members must be
// defined there; the mock state accessor is reached through the qualified
// name of the credential namespace.

namespace OHOS {

SystemAbilityManagerClient &SystemAbilityManagerClient::GetInstance()
{
    static SystemAbilityManagerClient client;
    return client;
}

sptr<ISystemAbilityManager> SystemAbilityManagerClient::GetSystemAbilityManager()
{
    Security::SAF::MockState &state = Security::SAF::GetMockState();
    ++state.managerGetCalls;
    if (state.managerCallsRemaining <= 0) {
        LOGI("[CRED-UT] GetSystemAbilityManager -> null");
        return sptr<ISystemAbilityManager>(nullptr);
    }
    --state.managerCallsRemaining;
    LOGI("[CRED-UT] GetSystemAbilityManager -> manager");
    return sptr<ISystemAbilityManager>(&state.samgr);
}

}  // namespace OHOS

namespace OHOS::Security::SAF {
namespace {

// ===== Test data builders =====

ModelCredentialContent MakeModelContent()
{
    ModelCredentialContent content;
    content.baseUrl = "https://api.example.com/v1";
    content.credentialValue = "sk-test";
    return content;
}

ModelCredentialInfo MakeModelInfo()
{
    ModelCredentialInfo info;
    info.provider = "mock-provider";
    info.modelList = {"model-a"};
    info.handleAlias = "alias";
    info.credentialAlias = "cred-alias";
    info.state = true;
    return info;
}

ModelCredential MakeModelCredential()
{
    ModelCredential credential;
    credential.modelCredentialContent = MakeModelContent();
    credential.modelCredentialInfo = MakeModelInfo();
    credential.modelCredentialIndex.credentialHandle = MOCK_HANDLE;
    credential.modelCredentialIndex.credentialId = MOCK_CREDENTIAL_ID;
    return credential;
}

ToolCredential MakeToolCredential()
{
    ToolCredential credential;
    credential.subjectId = "mock-subject";
    ToolCredentialContent content;
    content.envName = MOCK_TOOL_ENV;
    content.credentialValue = MOCK_SECRET;
    credential.toolCredentialContents = {content};
    return credential;
}

// ===== Per-API invocation table =====

// Aggregates every out-parameter used by the 10 public APIs so a single
// scenario runner can drive any of them.
struct ApiInvocation {
    std::string credentialHandle;
    std::string credentialId;
    std::string credentialValue;
    ModelCredentialContent modelContent;
    CredentialBatchResult batchResult;
    std::vector<ModelCredential> modelCredentials;
    std::vector<ToolCredentialInfo> toolCredentialInfos;
};

struct ApiEntry {
    const char *name;
    int32_t ipcCode;
    int32_t (*invoke)(SafCredential &, ApiInvocation &);
    void (*checkSuccessOutput)(const ApiInvocation &);
};

int32_t InvokeAddModel(SafCredential &cred, ApiInvocation &out)
{
    return cred.AddModelCredential(MakeModelContent(), MakeModelInfo(), out.credentialHandle, out.credentialId);
}

int32_t InvokeDeleteModel(SafCredential &cred, ApiInvocation &out)
{
    return cred.DeleteModelCredential({MakeModelCredential()}, out.batchResult);
}

int32_t InvokeUpdateModel(SafCredential &cred, ApiInvocation &out)
{
    return cred.UpdateModelCredential({MakeModelCredential()}, {MakeModelCredential()}, out.batchResult);
}

int32_t InvokeGetActiveModel(SafCredential &cred, ApiInvocation &out)
{
    return cred.GetActiveModelCredential(MOCK_HANDLE, "model-a", out.modelContent);
}

int32_t InvokePreGetAllModel(SafCredential &cred, ApiInvocation &out)
{
    return cred.PreGetAllModelCredentials(out.modelCredentials);
}

int32_t InvokeGetModelValue(SafCredential &cred, ApiInvocation &out)
{
    ModelCredentialIndex index;
    index.credentialHandle = MOCK_HANDLE;
    index.credentialId = MOCK_CREDENTIAL_ID;
    return cred.GetModelCredentialValue(index, out.credentialValue);
}

int32_t InvokeAddTool(SafCredential &cred, ApiInvocation &out)
{
    return cred.AddToolCredential({MakeToolCredential()}, out.batchResult);
}

int32_t InvokeUpdateTool(SafCredential &cred, ApiInvocation &out)
{
    return cred.UpdateToolCredential({MakeToolCredential()}, out.batchResult);
}

int32_t InvokeDeleteTool(SafCredential &cred, ApiInvocation &out)
{
    return cred.DeleteToolCredential({MakeToolCredential()}, out.batchResult);
}

int32_t InvokeGetTool(SafCredential &cred, ApiInvocation &out)
{
    ToolInfo toolInfo;
    toolInfo.agentTokenId = 123456789ULL;
    toolInfo.subjectId = "mock-subject";
    return cred.GetToolCredential(toolInfo, out.toolCredentialInfos);
}

void CheckBatchResult(const ApiInvocation &out)
{
    EXPECT_EQ(out.batchResult.failedCount, 1);
    ASSERT_EQ(out.batchResult.failedErrorInfos.size(), 1U);
    EXPECT_EQ(out.batchResult.failedErrorInfos[0].index, 0);
    EXPECT_EQ(out.batchResult.failedErrorInfos[0].errCode, SAF_ERR_ARG_INVALID);
}

void CheckAddModelOutput(const ApiInvocation &out)
{
    EXPECT_EQ(out.credentialHandle, MOCK_HANDLE);
    EXPECT_EQ(out.credentialId, MOCK_CREDENTIAL_ID);
}

void CheckGetActiveModelOutput(const ApiInvocation &out)
{
    EXPECT_EQ(out.modelContent.baseUrl, MOCK_MODEL_URL);
    EXPECT_EQ(out.modelContent.credentialValue, MOCK_SECRET);
}

void CheckPreGetAllModelOutput(const ApiInvocation &out)
{
    ASSERT_EQ(out.modelCredentials.size(), 1U);
    EXPECT_EQ(out.modelCredentials[0].modelCredentialIndex.credentialHandle, MOCK_HANDLE);
    EXPECT_TRUE(out.modelCredentials[0].modelCredentialContent.credentialValue.empty());
}

void CheckGetModelValueOutput(const ApiInvocation &out)
{
    EXPECT_EQ(out.credentialValue, MOCK_SECRET);
}

void CheckGetToolOutput(const ApiInvocation &out)
{
    ASSERT_EQ(out.toolCredentialInfos.size(), 1U);
    EXPECT_EQ(out.toolCredentialInfos[0].envName, MOCK_TOOL_ENV);
    EXPECT_EQ(out.toolCredentialInfos[0].credentialValue, MOCK_SECRET);
}

const ApiEntry API_TABLE[] = {
    {"AddModelCredential", IPC_ADD_MODEL_CREDENTIAL, InvokeAddModel, CheckAddModelOutput},
    {"DeleteModelCredential", IPC_DELETE_MODEL_CREDENTIAL, InvokeDeleteModel, CheckBatchResult},
    {"UpdateModelCredential", IPC_UPDATE_MODEL_CREDENTIAL, InvokeUpdateModel, CheckBatchResult},
    {"GetActiveModelCredential", IPC_GET_ACTIVE_MODEL_CREDENTIAL, InvokeGetActiveModel, CheckGetActiveModelOutput},
    {"PreGetAllModelCredentials", IPC_PRE_GET_ALL_MODEL_CREDENTIALS, InvokePreGetAllModel, CheckPreGetAllModelOutput},
    {"GetModelCredentialValue", IPC_GET_MODEL_CREDENTIAL_VALUE, InvokeGetModelValue, CheckGetModelValueOutput},
    {"AddToolCredential", IPC_ADD_TOOL_CREDENTIAL, InvokeAddTool, CheckBatchResult},
    {"UpdateToolCredential", IPC_UPDATE_TOOL_CREDENTIAL, InvokeUpdateTool, CheckBatchResult},
    {"DeleteToolCredential", IPC_DELETE_TOOL_CREDENTIAL, InvokeDeleteTool, CheckBatchResult},
    {"GetToolCredential", IPC_GET_TOOL_CREDENTIAL, InvokeGetTool, CheckGetToolOutput},
};

// ===== Fixture =====

class SafCredentialIpcTest : public testing::Test {
public:
    static void SetUpTestCase(void)
    {
        LOGI("[CRED-UT] SafCredentialIpcTest suite start");
    }
    static void TearDownTestCase(void)
    {
        LOGI("[CRED-UT] SafCredentialIpcTest suite end");
    }
    void SetUp(void) override
    {
        ResetMocks();
    }
    void TearDown(void) override {}

protected:
    void ResetMocks()
    {
        proxies_.clear();
        GetMockState() = MockState{};
        LOGI("[CRED-UT] mocks reset");
    }

    // Proxy returned by CheckSystemAbility (SA already running).
    MockCredentialProxy *SetupCheckHit(std::initializer_list<IpcReply> replies)
    {
        MockCredentialProxy *proxy = NewProxy(replies);
        GetMockState().samgr.checkResults.push_back(proxy);
        return proxy;
    }

    // Proxy returned by LoadSystemAbility (SA (re)loaded).
    MockCredentialProxy *SetupLoad(std::initializer_list<IpcReply> replies)
    {
        MockCredentialProxy *proxy = NewProxy(replies);
        GetMockState().samgr.loadResults.push_back(proxy);
        return proxy;
    }

    // Runs body once per public API with a fresh mock state and a fresh
    // SafCredential instance. SCOPED_TRACE reports the API name on failure.
    template <typename Body>
    void ForEachApi(Body &&body)
    {
        for (const ApiEntry &api : API_TABLE) {
            SCOPED_TRACE(api.name);
            ResetMocks();
            SafCredential cred;
            ApiInvocation invocation;
            body(api, cred, invocation);
        }
    }

    MockSystemAbilityManager &Samgr()
    {
        return GetMockState().samgr;
    }

private:
    MockCredentialProxy *NewProxy(std::initializer_list<IpcReply> replies)
    {
        proxies_.push_back(std::make_unique<MockCredentialProxy>(replies));
        return proxies_.back().get();
    }

    std::vector<std::unique_ptr<MockCredentialProxy>> proxies_;
};

// ===== GetProxy() branches =====

/**
 * @tc.name: SafCredentialIpcTest.AllApis_NoSystemAbilityManager_ReturnServiceUnavailable
 * @tc.desc: GetProxy() returns null when samgr itself is unavailable, so every
 *           API returns SAF_ERR_SERVICE_UNAVAILABLE without any IPC.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialIpcTest, AllApis_NoSystemAbilityManager_ReturnServiceUnavailable, TestSize.Level0)
{
    ForEachApi([&](const ApiEntry &api, SafCredential &cred, ApiInvocation &out) {
        GetMockState().managerCallsRemaining = 0;
        EXPECT_EQ(api.invoke(cred, out), SAF_ERR_SERVICE_UNAVAILABLE);
        EXPECT_EQ(GetMockState().managerGetCalls, 1);
        EXPECT_EQ(Samgr().checkCalls, 0);
        EXPECT_EQ(Samgr().loadCalls, 0);
    });
}

/**
 * @tc.name: SafCredentialIpcTest.AllApis_LoadFail_ReturnServiceUnavailable
 * @tc.desc: CheckSystemAbility misses and LoadSystemAbility returns null: the
 *           service id and 5s timeout are still the contract, and the API
 *           reports SAF_ERR_SERVICE_UNAVAILABLE.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialIpcTest, AllApis_LoadFail_ReturnServiceUnavailable, TestSize.Level0)
{
    ForEachApi([&](const ApiEntry &api, SafCredential &cred, ApiInvocation &out) {
        EXPECT_EQ(api.invoke(cred, out), SAF_ERR_SERVICE_UNAVAILABLE);
        EXPECT_EQ(Samgr().checkCalls, 1);
        EXPECT_EQ(Samgr().lastCheckId, SAF_SERVICE_ID);
        EXPECT_EQ(Samgr().loadCalls, 1);
        EXPECT_EQ(Samgr().lastLoadId, SAF_SERVICE_ID);
        EXPECT_EQ(Samgr().lastLoadTimeout, WAIT_TIMEOUT_IN_SEC);
    });
}

/**
 * @tc.name: SafCredentialIpcTest.AllApis_LoadSuccess_ReturnSuccess
 * @tc.desc: CheckSystemAbility misses, LoadSystemAbility succeeds: the proxy is
 *           used and the out-parameters are propagated.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialIpcTest, AllApis_LoadSuccess_ReturnSuccess, TestSize.Level0)
{
    ForEachApi([&](const ApiEntry &api, SafCredential &cred, ApiInvocation &out) {
        MockCredentialProxy *proxy = SetupLoad({{SAF_SUCCESS, SAF_SUCCESS}});
        EXPECT_EQ(api.invoke(cred, out), SAF_SUCCESS);
        EXPECT_EQ(Samgr().checkCalls, 1);
        EXPECT_EQ(Samgr().loadCalls, 1);
        ASSERT_EQ(proxy->ipcCalls, 1);
        EXPECT_EQ(proxy->lastIpcCode, api.ipcCode);
        api.checkSuccessOutput(out);
    });
}

/**
 * @tc.name: SafCredentialIpcTest.AllApis_CheckHit_ReturnSuccess
 * @tc.desc: CheckSystemAbility finds the running SA: LoadSystemAbility is
 *           skipped and the proxy result is returned.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialIpcTest, AllApis_CheckHit_ReturnSuccess, TestSize.Level0)
{
    ForEachApi([&](const ApiEntry &api, SafCredential &cred, ApiInvocation &out) {
        MockCredentialProxy *proxy = SetupCheckHit({{SAF_SUCCESS, SAF_SUCCESS}});
        EXPECT_EQ(api.invoke(cred, out), SAF_SUCCESS);
        EXPECT_EQ(Samgr().checkCalls, 1);
        EXPECT_EQ(Samgr().loadCalls, 0);
        ASSERT_EQ(proxy->ipcCalls, 1);
        EXPECT_EQ(proxy->lastIpcCode, api.ipcCode);
        api.checkSuccessOutput(out);
    });
}

// ===== HandleIpcError() branches =====

/**
 * @tc.name: SafCredentialIpcTest.AllApis_IpcBusinessError_ReturnResultCode
 * @tc.desc: The IPC transport succeeds but the service reports a business error
 *           in resultCode: the API returns that code unchanged and does not
 *           treat it as a transport failure.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialIpcTest, AllApis_IpcBusinessError_ReturnResultCode, TestSize.Level0)
{
    ForEachApi([&](const ApiEntry &api, SafCredential &cred, ApiInvocation &out) {
        MockCredentialProxy *proxy = SetupCheckHit({{SAF_SUCCESS, SAF_ERR_PERMISSION_DENIED}});
        EXPECT_EQ(api.invoke(cred, out), SAF_ERR_PERMISSION_DENIED);
        EXPECT_EQ(proxy->ipcCalls, 1);
        EXPECT_EQ(Samgr().loadCalls, 0);
    });
}

/**
 * @tc.name: SafCredentialIpcTest.AllApis_IpcTransportError_ReturnIpcProxyFail
 * @tc.desc: A non-dead transport error is converted by the API's
 *           IF_ERROR_LOGE_RETURN_ERR guard into SAF_ERR_IPC_PROXY_FAIL.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialIpcTest, AllApis_IpcTransportError_ReturnIpcProxyFail, TestSize.Level0)
{
    ForEachApi([&](const ApiEntry &api, SafCredential &cred, ApiInvocation &out) {
        MockCredentialProxy *proxy = SetupCheckHit({{SAF_ERR_IPC_ERROR, SAF_SUCCESS}});
        EXPECT_EQ(api.invoke(cred, out), SAF_ERR_IPC_PROXY_FAIL);
        EXPECT_EQ(proxy->ipcCalls, 1);
        EXPECT_EQ(Samgr().loadCalls, 0);
    });
}

/**
 * @tc.name: SafCredentialIpcTest.AllApis_DeadObject_ReturnIpcProxyFailWithoutRetry
 * @tc.desc: ERR_DEAD_OBJECT is normalized to SAF_ERR_SERVICE_UNAVAILABLE by
 *           HandleIpcError, which then returns immediately (no retry); the
 *           public API's error guard maps the non-zero result to
 *           SAF_ERR_IPC_PROXY_FAIL.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialIpcTest, AllApis_DeadObject_ReturnIpcProxyFailWithoutRetry, TestSize.Level0)
{
    ForEachApi([&](const ApiEntry &api, SafCredential &cred, ApiInvocation &out) {
        MockCredentialProxy *proxy = SetupCheckHit({{ERR_DEAD_OBJECT, SAF_SUCCESS}});
        EXPECT_EQ(api.invoke(cred, out), SAF_ERR_IPC_PROXY_FAIL);
        EXPECT_EQ(proxy->ipcCalls, 1);
        EXPECT_EQ(Samgr().loadCalls, 0);
        EXPECT_EQ(Samgr().checkCalls, 1);
    });
}

/**
 * @tc.name: SafCredentialIpcTest.AllApis_RemoteDead_ReturnIpcProxyFailWithoutRetry
 * @tc.desc: The IPC-layer ERR_REMOTE_DEAD literal (29189) takes the same
 *           normalize-and-stop path as ERR_DEAD_OBJECT.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialIpcTest, AllApis_RemoteDead_ReturnIpcProxyFailWithoutRetry, TestSize.Level0)
{
    ForEachApi([&](const ApiEntry &api, SafCredential &cred, ApiInvocation &out) {
        MockCredentialProxy *proxy = SetupCheckHit({{ERR_REMOTE_DEAD, SAF_SUCCESS}});
        EXPECT_EQ(api.invoke(cred, out), SAF_ERR_IPC_PROXY_FAIL);
        EXPECT_EQ(proxy->ipcCalls, 1);
        EXPECT_EQ(Samgr().loadCalls, 0);
    });
}

/**
 * @tc.name: SafCredentialIpcTest.AllApis_ServiceStopping_RetrySuccess
 * @tc.desc: SAF_ERR_SERVICE_IS_STOPPING triggers an unconditional reload
 *           (GetProxy(false): CheckSystemAbility is skipped) followed by one
 *           retry; the retried call succeeds and its resultCode is returned.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialIpcTest, AllApis_ServiceStopping_RetrySuccess, TestSize.Level0)
{
    ForEachApi([&](const ApiEntry &api, SafCredential &cred, ApiInvocation &out) {
        MockCredentialProxy *first = SetupCheckHit({{SAF_ERR_SERVICE_IS_STOPPING, SAF_SUCCESS}});
        MockCredentialProxy *second = SetupLoad({{SAF_SUCCESS, SAF_SUCCESS}});
        EXPECT_EQ(api.invoke(cred, out), SAF_SUCCESS);
        EXPECT_EQ(first->ipcCalls, 1);
        ASSERT_EQ(second->ipcCalls, 1);
        EXPECT_EQ(second->lastIpcCode, api.ipcCode);
        // The retry path is GetProxy(false): no second CheckSystemAbility.
        EXPECT_EQ(Samgr().checkCalls, 1);
        EXPECT_EQ(Samgr().loadCalls, 1);
        EXPECT_EQ(Samgr().lastLoadId, SAF_SERVICE_ID);
        EXPECT_EQ(Samgr().lastLoadTimeout, WAIT_TIMEOUT_IN_SEC);
        api.checkSuccessOutput(out);
    });
}

/**
 * @tc.name: SafCredentialIpcTest.AllApis_ServiceStopping_ReloadFail
 * @tc.desc: SAF_ERR_SERVICE_IS_STOPPING and the reload returns null: the retry
 *           is not attempted and the API reports SAF_ERR_IPC_PROXY_FAIL.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialIpcTest, AllApis_ServiceStopping_ReloadFail, TestSize.Level0)
{
    ForEachApi([&](const ApiEntry &api, SafCredential &cred, ApiInvocation &out) {
        MockCredentialProxy *first = SetupCheckHit({{SAF_ERR_SERVICE_IS_STOPPING, SAF_SUCCESS}});
        EXPECT_EQ(api.invoke(cred, out), SAF_ERR_IPC_PROXY_FAIL);
        EXPECT_EQ(first->ipcCalls, 1);
        EXPECT_EQ(Samgr().checkCalls, 1);
        EXPECT_EQ(Samgr().loadCalls, 1);
    });
}

/**
 * @tc.name: SafCredentialIpcTest.AllApis_ServiceStopping_RetryTransportError
 * @tc.desc: The reload succeeds but the retried IPC call fails: HandleIpcError
 *           returns the retry error and the API maps it to
 *           SAF_ERR_IPC_PROXY_FAIL.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialIpcTest, AllApis_ServiceStopping_RetryTransportError, TestSize.Level0)
{
    ForEachApi([&](const ApiEntry &api, SafCredential &cred, ApiInvocation &out) {
        MockCredentialProxy *first = SetupCheckHit({{SAF_ERR_SERVICE_IS_STOPPING, SAF_SUCCESS}});
        MockCredentialProxy *second = SetupLoad({{SAF_ERR_IPC_ERROR, SAF_SUCCESS}});
        EXPECT_EQ(api.invoke(cred, out), SAF_ERR_IPC_PROXY_FAIL);
        EXPECT_EQ(first->ipcCalls, 1);
        EXPECT_EQ(second->ipcCalls, 1);
        EXPECT_EQ(Samgr().loadCalls, 1);
    });
}

/**
 * @tc.name: SafCredentialIpcTest.AllApis_ServiceStopping_RetryBusinessError
 * @tc.desc: The retried IPC call succeeds with a business error, proving the
 *           resultCode of the second call (not the first) is returned.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialIpcTest, AllApis_ServiceStopping_RetryBusinessError, TestSize.Level0)
{
    ForEachApi([&](const ApiEntry &api, SafCredential &cred, ApiInvocation &out) {
        MockCredentialProxy *first = SetupCheckHit({{SAF_ERR_SERVICE_IS_STOPPING, SAF_SUCCESS}});
        MockCredentialProxy *second = SetupLoad({{SAF_SUCCESS, SAF_ERR_DLOPEN_FAIL}});
        EXPECT_EQ(api.invoke(cred, out), SAF_ERR_DLOPEN_FAIL);
        EXPECT_EQ(first->ipcCalls, 1);
        EXPECT_EQ(second->ipcCalls, 1);
    });
}

/**
 * @tc.name: SafCredentialIpcTest.AllApis_ServiceStopping_RetryWithoutSamgr
 * @tc.desc: SAF_ERR_SERVICE_IS_STOPPING and samgr disappears before the
 *           reload: GetProxy(false) returns null on the manager==nullptr branch
 *           and the API reports SAF_ERR_IPC_PROXY_FAIL.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialIpcTest, AllApis_ServiceStopping_RetryWithoutSamgr, TestSize.Level0)
{
    ForEachApi([&](const ApiEntry &api, SafCredential &cred, ApiInvocation &out) {
        MockCredentialProxy *first = SetupCheckHit({{SAF_ERR_SERVICE_IS_STOPPING, SAF_SUCCESS}});
        // The first GetSystemAbilityManager() succeeds, the retry one fails.
        GetMockState().managerCallsRemaining = 1;
        EXPECT_EQ(api.invoke(cred, out), SAF_ERR_IPC_PROXY_FAIL);
        EXPECT_EQ(first->ipcCalls, 1);
        EXPECT_EQ(GetMockState().managerGetCalls, 2);
        EXPECT_EQ(Samgr().loadCalls, 0);
    });
}

// ===== UpdateModelCredential() argument guard =====

/**
 * @tc.name: SafCredentialIpcTest.UpdateModelCredential_SizeMismatch_ReturnArgInvalid
 * @tc.desc: A src/dest length mismatch is rejected before any samgr or IPC
 *           access, in both directions.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialIpcTest, UpdateModelCredential_SizeMismatch_ReturnArgInvalid, TestSize.Level0)
{
    SafCredential cred;
    CredentialBatchResult result;

    std::vector<ModelCredential> one = {MakeModelCredential()};
    std::vector<ModelCredential> empty;
    EXPECT_EQ(cred.UpdateModelCredential(one, empty, result), SAF_ERR_ARG_INVALID);
    EXPECT_EQ(cred.UpdateModelCredential(empty, one, result), SAF_ERR_ARG_INVALID);
    EXPECT_EQ(cred.UpdateModelCredential(one, {MakeModelCredential(), MakeModelCredential()}, result),
              SAF_ERR_ARG_INVALID);
    EXPECT_EQ(GetMockState().managerGetCalls, 0);
    EXPECT_EQ(Samgr().checkCalls, 0);
    EXPECT_EQ(Samgr().loadCalls, 0);
}

/**
 * @tc.name: SafCredentialIpcTest.UpdateModelCredential_EqualEmpty_ReachIpc
 * @tc.desc: Equal lengths (0 == 0) pass the guard and reach IPC; the service
 *           decides whether an empty batch is legal.
 * @tc.type: FUNC
 */
HWTEST_F(SafCredentialIpcTest, UpdateModelCredential_EqualEmpty_ReachIpc, TestSize.Level0)
{
    MockCredentialProxy *proxy = SetupCheckHit({{SAF_SUCCESS, SAF_SUCCESS}});
    SafCredential cred;
    CredentialBatchResult result;
    std::vector<ModelCredential> empty;
    EXPECT_EQ(cred.UpdateModelCredential(empty, empty, result), SAF_SUCCESS);
    EXPECT_EQ(proxy->ipcCalls, 1);
    EXPECT_EQ(proxy->lastIpcCode, IPC_UPDATE_MODEL_CREDENTIAL);
    EXPECT_EQ(result.failedCount, 1);
}

}  // namespace
}  // namespace OHOS::Security::SAF