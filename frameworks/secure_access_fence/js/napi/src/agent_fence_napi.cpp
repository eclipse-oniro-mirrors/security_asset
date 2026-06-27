/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#include <cstdint>
#include "saf_result_defs.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "saf_log.h"
#include "secure_access_fence_type.h"
#include "saf_agent_fence.h"
#include "agent_fence_napi_context.h"
#include "napi_common.h"
#include "agent_fence_error_codes.h"

using namespace OHOS::Security::SAF;
using namespace OHOS::Security::SAF_ASSET_COMMON;

namespace {
napi_value NapiRequestToolPermissions(const napi_env env, napi_callback_info info)
{
    auto asyncContext = std::unique_ptr<RequestToolPermissionContext>(new (std::nothrow)RequestToolPermissionContext());
    NAPI_THROW(env, asyncContext == nullptr, COMMON_INTERNAL_ERROR, "Failed to create RequestToolPermissionContext");
    asyncContext->parse = [](napi_env env, napi_callback_info info, AgentFenceAsyncContext *context)
        -> napi_status {
        RequestToolPermissionContext *asyncContext = static_cast<RequestToolPermissionContext *>(context);
        size_t argc = 1;
        napi_value argv[1] = { nullptr };
        NAPI_CALL_RETURN_ERR(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
        NAPI_THROW_RETURN_ERR(env, argc < 1, GENERAL_PARAMETER_ERROR, "Invalid number of arguments");
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, argv[0], asyncContext->permissionQuery));
        return napi_ok;
    };

    asyncContext->execute = [](napi_env env, void* data) {
        RequestToolPermissionContext *asyncContext = static_cast<RequestToolPermissionContext *>(data);
        asyncContext -> result = SafAgentFence::RequestToolPermissions(asyncContext->permissionQuery,
            asyncContext->permissionQueryResult);
    };

    asyncContext->resolve = [](napi_env env, AgentFenceAsyncContext *context) -> napi_value {
        RequestToolPermissionContext *asyncContext = static_cast<RequestToolPermissionContext *>(context);
        napi_value napiResult = nullptr;
        NAPI_CALL(env, napi_create_object(env, &napiResult));
        NAPI_CALL(env, NapiSetProperty(env, napiResult, "needDialog", asyncContext->permissionQueryResult.needDialog));
        NAPI_CALL(env, NapiSetProperty(env, napiResult, "permissionResults",
            asyncContext->permissionQueryResult.permissionResults));
        if (!asyncContext->permissionQueryResult.hasTicket) {
            NAPI_CALL(env, NapiSetPropertyUndefined(env, napiResult, "ticket"));
        } else {
            NAPI_CALL(env, NapiSetProperty(env, napiResult, "ticket", asyncContext->permissionQueryResult.ticket));
        }
        return napiResult;
    };
    return CreateAsyncWork(env, info, std::move(asyncContext), __func__);
}

napi_value NapiGrantToolPermissionsByUser(const napi_env env, napi_callback_info info)
{
    auto asyncContext = std::unique_ptr<GrantPermissionsContext>(new (std::nothrow)GrantPermissionsContext());
    NAPI_THROW(env, asyncContext == nullptr, COMMON_INTERNAL_ERROR, "Failed to create GrantPermissionsContext");
    asyncContext->parse = [](napi_env env, napi_callback_info info, AgentFenceAsyncContext *context)
        -> napi_status {
        GrantPermissionsContext *asyncContext = static_cast<GrantPermissionsContext *>(context);
        size_t argc = 1;
        napi_value argv[1] = { nullptr };
        NAPI_CALL_RETURN_ERR(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
        NAPI_THROW_RETURN_ERR(env, argc < 1, GENERAL_PARAMETER_ERROR, "Invalid number of arguments");
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, argv[0], asyncContext->userAuthResult));
        return napi_ok;
    };

    asyncContext->execute = [](napi_env env, void* data) {
        GrantPermissionsContext *asyncContext = static_cast<GrantPermissionsContext *>(data);
        asyncContext->result = SafAgentFence::GrantToolPermissionsByUser(asyncContext->userAuthResult,
            asyncContext->ticketInfo);
    };

    asyncContext->resolve = [](napi_env env, AgentFenceAsyncContext *context) -> napi_value {
        GrantPermissionsContext *asyncContext = static_cast<GrantPermissionsContext *>(context);
        napi_value jsResult = nullptr;
        NAPI_CALL(env, napi_create_array(env, &jsResult));
        for (uint32_t i = 0; i < asyncContext->ticketInfo.size(); ++i) {
            napi_value jsResultItem = nullptr;
            NAPI_CALL(env, napi_create_object(env, &jsResultItem));
            NAPI_CALL(env, NapiSetProperty(env, jsResultItem, "message", asyncContext->ticketInfo[i].message));
            NAPI_CALL(env, NapiSetProperty(env, jsResultItem, "challenge", asyncContext->ticketInfo[i].challenge));
            NAPI_CALL(env, NapiSetProperty(env, jsResultItem, "ticket", asyncContext->ticketInfo[i].ticket));
            NAPI_CALL(env, napi_set_element(env, jsResult, i, jsResultItem));
        }
        return jsResult;
    };
    return CreateAsyncWork(env, info, std::move(asyncContext), __func__);
}

napi_value DeclareOperationType(const napi_env env)
{
    napi_value status = nullptr;
    NAPI_CALL(env, napi_create_object(env, &status));
    AddUint32Property(env, status, "CLI", static_cast<uint32_t>(OperationType::CLI));
    AddUint32Property(env, status, "API", static_cast<uint32_t>(OperationType::API));
    return status;
}

napi_value DeclareAuthStatus(const napi_env env)
{
    napi_value status = nullptr;
    NAPI_CALL(env, napi_create_object(env, &status));
    AddUint32Property(env, status, "REQUIRE_AUTH", static_cast<uint32_t>(AuthStatus::REQUIRE_AUTH));
    AddUint32Property(env, status, "FORBIDDEN", static_cast<uint32_t>(AuthStatus::FORBIDDEN));
    AddUint32Property(env, status, "AUTHORIZED", static_cast<uint32_t>(AuthStatus::AUTHORIZED));
    AddUint32Property(env, status, "RESTRICTED", static_cast<uint32_t>(AuthStatus::RESTRICTED));
    AddUint32Property(env, status, "REMOTE_RESTRICTED", static_cast<uint32_t>(AuthStatus::REMOTE_RESTRICTED));
    return status;
}

napi_value Register(const napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        // register function
        DECLARE_NAPI_FUNCTION("requestToolPermissions", NapiRequestToolPermissions),
        DECLARE_NAPI_FUNCTION("grantToolPermissionsByUser", NapiGrantToolPermissionsByUser),

        // register enum
        DECLARE_NAPI_PROPERTY("OperationType", DeclareOperationType(env)),
        DECLARE_NAPI_PROPERTY("AuthStatus", DeclareAuthStatus(env)),
    };

    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    return exports;
}

napi_module g_module = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Register,
    .nm_modname = "abilityToolAccessCtrl",
    .nm_priv = static_cast<void *>(0),
    .reserved = { 0 },
};

} // anonymous namespace

extern "C" __attribute__((constructor)) void RegisterModule(void)
{
    napi_module_register(&g_module);
}
