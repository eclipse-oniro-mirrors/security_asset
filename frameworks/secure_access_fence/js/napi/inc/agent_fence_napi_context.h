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
#include "napi/native_node_api.h"
#include "secure_access_fence_type.h"

#ifndef AGENT_FENCE_NAPI_CONTEXT_H
#define AGENT_FENCE_NAPI_CONTEXT_H

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
    napi_env env;
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

class GrantPermissionsContext : public AgentFenceAsyncContext {
    public:
        std::vector<UserAuthResult> userAuthResult {};
        std::vector<VerifyTicketInfo> ticketInfo {};
};
} // namespace SAF
} // namespace Security
} // namespace OHOS

#endif // AGENT_FENCE_NAPI_CONTEXT_H