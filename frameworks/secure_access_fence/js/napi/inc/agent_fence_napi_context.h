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

#ifndef AGENT_FENCE_NAPI_CONTEXT_H
#define AGENT_FENCE_NAPI_CONTEXT_H

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include "napi/native_node_api.h"
#include "secure_access_fence_type.h"
#include "saf_result_code.h"
#include "event_handler.h"
#include "want.h"

namespace OHOS {
namespace Security {
namespace SAF {

class AgentFenceAsyncContext {
public:
    virtual ~ AgentFenceAsyncContext()
    {
        if (work != nullptr && env != nullptr) {
            napi_delete_async_work(env, work);
            work = nullptr;
            env = nullptr;
        }
    }
    napi_env env = nullptr;
    napi_async_work work = nullptr;
    napi_deferred deferred = nullptr;
    int32_t result;

    std::function<napi_status(napi_env, napi_callback_info, AgentFenceAsyncContext *)> parse;
    napi_async_execute_callback execute;
    std::function<napi_value(napi_env, AgentFenceAsyncContext *)> resolve;
};

class RequestToolPermissionContext : public AgentFenceAsyncContext {
    public:
        PermissionQuery permissionQuery {};
        PermissionQueryResult permissionQueryResult {};
};

class RequestToolTicketContext : public AgentFenceAsyncContext {
    public:
        std::vector<CommandInfo> cliCmdInfos {};
        TicketOptions ticketOptions {};
        TicketQueryResult ticketQueryResult {};
};

class GrantPermissionsContext : public AgentFenceAsyncContext {
    public:
        std::vector<UserAuthResult> userAuthResult {};
        std::vector<VerifyTicketInfo> ticketInfo {};
};

class GenerateControlledDevicePackageContext : public AgentFenceAsyncContext {
public:
    std::vector<PermissionQuery> permissionQuery {};
    std::vector<RemoteAuthPackage> remoteAuthPackage {};
};

class VerifyControlledDevicePackageContext : public AgentFenceAsyncContext {
public:
    std::vector<RemoteAuthPackage> ticketInfo {};
    std::vector<bool> verifyRes {};
};

class GenerateControllerDevicePackageContext : public AgentFenceAsyncContext {
public:
    std::vector<RemoteUserAuthResults> remoteUserAuthResults {};
    std::vector<RemoteAuthPackage> remoteAuthPackage {};
};

class VerifyControllerDevicePackageContext : public AgentFenceAsyncContext {
public:
    std::vector<RemoteAuthPackage> ticketInfo {};
    RemoteInfo remoteInfo {};
    std::vector<bool> verifyRes {};
};

class GetRemoteGrantStatusContext : public AgentFenceAsyncContext {
public:
    int32_t remoteGrantStatus {};
};

class UpdateRemoteGrantStatusContext : public AgentFenceAsyncContext {
public:
    int32_t remoteGrantStatus {};
};

class GenerateTaskChallengeContext : public AgentFenceAsyncContext {
public:
    TaskIdentityInfo taskIdentityInfo {};
    std::vector<int32_t> challenge {};
};

class GenerateTaskIdentityContext : public AgentFenceAsyncContext {
public:
    TaskIdentityInfo taskIdentityInfo {};
    TaskIdentity taskIdentity {};
};

class RevokeTaskIdentityContext : public AgentFenceAsyncContext {
public:
    std::vector<std::string> taskIds {};
};

class QueryControlAppStatusContext : public AgentFenceAsyncContext {
public:
    std::vector<AppInfo> appInfoList {};
    std::vector<ControlAppStatusResult> resultList {};
};

class RequestControlAppPermissionContext : public AgentFenceAsyncContext {
public:
    std::vector<AppInfo> appInfoList {};
    std::vector<ControlAppStatusResult> resultList {};
    std::vector<ControlAppStatusResult> queryResultList {};
    std::vector<AppInfo> unauthorizedAppList {};
    std::vector<ControlAppStatus> userDecisions {};
    napi_ref contextRef = nullptr;
    int32_t instanceId = -1;
    int32_t sessionId = -1;
    bool releaseFlag = false;
    std::atomic<bool> isOnResult {false};
    // Caller app info obtained via IPC context (populated in execute Step 1).
    std::string callerBundleName;
    int32_t callerIndex = -1;
    std::shared_ptr<AppExecFwk::EventHandler> handler = nullptr;
    // Independent shared data holder for async dialog callbacks.
    // Lifetime: tied to RequestControlAppPermissionContext via shared_ptr.
    // The dialog callbacks capture this shared_ptr, decoupled from context lifetime.
    struct DialogData {
        std::vector<std::string> resultAppInfoList;   // parsed from OnReceive Want
        std::vector<int32_t> resultDecisions;         // parsed from OnReceive Want
        std::mutex mutex;
        std::condition_variable cv;
        bool done = false;
        int32_t resultCode = SAF_SUCCESS;
        int32_t sessionId = 0;
        int32_t instanceId = -1;
        bool releaseFlag = false;
    };
    std::shared_ptr<DialogData> dialogData;
};
} // namespace SAF
} // namespace Security
} // namespace OHOS

#endif // AGENT_FENCE_NAPI_CONTEXT_H