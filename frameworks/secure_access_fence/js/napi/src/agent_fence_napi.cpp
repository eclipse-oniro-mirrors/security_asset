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
constexpr size_t VERIFY_CONTROLLER_PACKAGE_ARG_COUNT = 2;
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

napi_value NapiGenerateControlledDevicePackage(const napi_env env, napi_callback_info info)
{
    auto asyncContext = std::unique_ptr<GenerateControlledDevicePackageContext>(
        new (std::nothrow) GenerateControlledDevicePackageContext());
    NAPI_THROW(env, asyncContext == nullptr, COMMON_INTERNAL_ERROR,
        "Failed to create GenerateControlledDevicePackageContext");

    asyncContext->parse = [](napi_env env, napi_callback_info info, AgentFenceAsyncContext *context)
        -> napi_status {
        GenerateControlledDevicePackageContext *asyncContext =
            static_cast<GenerateControlledDevicePackageContext *>(context);
        size_t argc = 1;
        napi_value argv[1] = { nullptr };
        NAPI_CALL_RETURN_ERR(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
        NAPI_THROW_RETURN_ERR(env, argc < 1, GENERAL_PARAMETER_ERROR, "Invalid number of arguments");
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, argv[0], asyncContext->permissionQuery));
        return napi_ok;
    };

    asyncContext->execute = [](napi_env env, void* data) {
        GenerateControlledDevicePackageContext *asyncContext =
            static_cast<GenerateControlledDevicePackageContext *>(data);
        asyncContext->result = SafAgentFence::GenerateControlledDevicePackage(
            asyncContext->permissionQuery, asyncContext->remoteAuthPackage);
    };

    asyncContext->resolve = [](napi_env env, AgentFenceAsyncContext *context) -> napi_value {
        GenerateControlledDevicePackageContext *asyncContext =
            static_cast<GenerateControlledDevicePackageContext *>(context);
        napi_value jsResult = nullptr;
        NAPI_CALL(env, napi_create_array(env, &jsResult));
        for (uint32_t i = 0; i < asyncContext->remoteAuthPackage.size(); ++i) {
            napi_value jsResultItem = nullptr;
            NAPI_CALL(env, napi_create_object(env, &jsResultItem));
            NAPI_CALL(env, NapiSetProperty(env, jsResultItem, "remoteMessage",
                asyncContext->remoteAuthPackage[i].remoteMessage));
            NAPI_CALL(env, NapiSetProperty(env, jsResultItem, "challenge",
                asyncContext->remoteAuthPackage[i].challenge));
            NAPI_CALL(env, NapiSetProperty(env, jsResultItem, "ticket",
                asyncContext->remoteAuthPackage[i].ticket));
            NAPI_CALL(env, napi_set_element(env, jsResult, i, jsResultItem));
        }
        return jsResult;
    };

    return CreateAsyncWork(env, info, std::move(asyncContext), __func__);
}

napi_value NapiVerifyControlledDevicePackage(const napi_env env, napi_callback_info info)
{
    auto asyncContext = std::unique_ptr<VerifyControlledDevicePackageContext>(
        new (std::nothrow) VerifyControlledDevicePackageContext());
    NAPI_THROW(env, asyncContext == nullptr, COMMON_INTERNAL_ERROR,
        "Failed to create VerifyControlledDevicePackageContext");

    asyncContext->parse = [](napi_env env, napi_callback_info info, AgentFenceAsyncContext *context)
        -> napi_status {
        VerifyControlledDevicePackageContext *asyncContext =
            static_cast<VerifyControlledDevicePackageContext *>(context);
        size_t argc = 1;
        napi_value argv[1] = { nullptr };
        NAPI_CALL_RETURN_ERR(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
        NAPI_THROW_RETURN_ERR(env, argc < 1, GENERAL_PARAMETER_ERROR, "Invalid number of arguments");
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, argv[0], asyncContext->ticketInfo));
        return napi_ok;
    };

    asyncContext->execute = [](napi_env env, void* data) {
        VerifyControlledDevicePackageContext *asyncContext =
            static_cast<VerifyControlledDevicePackageContext *>(data);
        asyncContext->result = SafAgentFence::VerifyControlledDevicePackage(
            asyncContext->ticketInfo, asyncContext->verifyRes);
    };

    asyncContext->resolve = [](napi_env env, AgentFenceAsyncContext *context) -> napi_value {
        VerifyControlledDevicePackageContext *asyncContext =
            static_cast<VerifyControlledDevicePackageContext *>(context);
        napi_value jsResult = nullptr;
        NAPI_CALL(env, napi_create_array(env, &jsResult));
        for (uint32_t i = 0; i < asyncContext->verifyRes.size(); ++i) {
            napi_value jsResultItem = nullptr;
            NAPI_CALL(env, napi_get_boolean(env, asyncContext->verifyRes[i], &jsResultItem));
            NAPI_CALL(env, napi_set_element(env, jsResult, i, jsResultItem));
        }
        return jsResult;
    };

    return CreateAsyncWork(env, info, std::move(asyncContext), __func__);
}

napi_value NapiGenerateControllerDevicePackage(const napi_env env, napi_callback_info info)
{
    auto asyncContext = std::unique_ptr<GenerateControllerDevicePackageContext>(
        new (std::nothrow) GenerateControllerDevicePackageContext());
    NAPI_THROW(env, asyncContext == nullptr, COMMON_INTERNAL_ERROR,
        "Failed to create GenerateControllerDevicePackageContext");

    asyncContext->parse = [](napi_env env, napi_callback_info info, AgentFenceAsyncContext *context)
        -> napi_status {
        GenerateControllerDevicePackageContext *asyncContext =
            static_cast<GenerateControllerDevicePackageContext *>(context);
        size_t argc = 1;
        napi_value argv[1] = { nullptr };
        NAPI_CALL_RETURN_ERR(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
        NAPI_THROW_RETURN_ERR(env, argc < 1, GENERAL_PARAMETER_ERROR, "Invalid number of arguments");
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, argv[0], asyncContext->remoteUserAuthResults));
        return napi_ok;
    };

    asyncContext->execute = [](napi_env env, void* data) {
        GenerateControllerDevicePackageContext *asyncContext =
            static_cast<GenerateControllerDevicePackageContext *>(data);
        asyncContext->result = SafAgentFence::GenerateControllerDevicePackage(
            asyncContext->remoteUserAuthResults, asyncContext->remoteAuthPackage);
    };

    asyncContext->resolve = [](napi_env env, AgentFenceAsyncContext *context) -> napi_value {
        GenerateControllerDevicePackageContext *asyncContext =
            static_cast<GenerateControllerDevicePackageContext *>(context);
        napi_value jsResult = nullptr;
        NAPI_CALL(env, napi_create_array(env, &jsResult));
        for (uint32_t i = 0; i < asyncContext->remoteAuthPackage.size(); ++i) {
            napi_value jsResultItem = nullptr;
            NAPI_CALL(env, napi_create_object(env, &jsResultItem));
            NAPI_CALL(env, NapiSetProperty(env, jsResultItem, "remoteMessage",
                asyncContext->remoteAuthPackage[i].remoteMessage));
            NAPI_CALL(env, NapiSetProperty(env, jsResultItem, "challenge",
                asyncContext->remoteAuthPackage[i].challenge));
            NAPI_CALL(env, NapiSetProperty(env, jsResultItem, "ticket",
                asyncContext->remoteAuthPackage[i].ticket));
            NAPI_CALL(env, napi_set_element(env, jsResult, i, jsResultItem));
        }
        return jsResult;
    };

    return CreateAsyncWork(env, info, std::move(asyncContext), __func__);
}

napi_value NapiVerifyControllerDevicePackage(const napi_env env, napi_callback_info info)
{
    auto asyncContext = std::unique_ptr<VerifyControllerDevicePackageContext>(
        new (std::nothrow) VerifyControllerDevicePackageContext());
    NAPI_THROW(env, asyncContext == nullptr, COMMON_INTERNAL_ERROR,
        "Failed to create VerifyControllerDevicePackageContext");

    asyncContext->parse = [](napi_env env, napi_callback_info info, AgentFenceAsyncContext *context)
        -> napi_status {
        VerifyControllerDevicePackageContext *asyncContext =
            static_cast<VerifyControllerDevicePackageContext *>(context);
        size_t argc = VERIFY_CONTROLLER_PACKAGE_ARG_COUNT;
        napi_value argv[VERIFY_CONTROLLER_PACKAGE_ARG_COUNT] = { nullptr, nullptr };
        NAPI_CALL_RETURN_ERR(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
        NAPI_THROW_RETURN_ERR(env, argc < VERIFY_CONTROLLER_PACKAGE_ARG_COUNT, GENERAL_PARAMETER_ERROR,
            "Invalid number of arguments");
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, argv[0], asyncContext->ticketInfo));
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, argv[1], asyncContext->remoteInfo));
        return napi_ok;
    };

    asyncContext->execute = [](napi_env env, void* data) {
        VerifyControllerDevicePackageContext *asyncContext =
            static_cast<VerifyControllerDevicePackageContext *>(data);
        asyncContext->result = SafAgentFence::VerifyControllerDevicePackage(
            asyncContext->ticketInfo, asyncContext->remoteInfo, asyncContext->verifyRes);
    };

    asyncContext->resolve = [](napi_env env, AgentFenceAsyncContext *context) -> napi_value {
        VerifyControllerDevicePackageContext *asyncContext =
            static_cast<VerifyControllerDevicePackageContext *>(context);
        napi_value jsResult = nullptr;
        NAPI_CALL(env, napi_create_array(env, &jsResult));
        for (uint32_t i = 0; i < asyncContext->verifyRes.size(); ++i) {
            napi_value jsResultItem = nullptr;
            NAPI_CALL(env, napi_get_boolean(env, asyncContext->verifyRes[i], &jsResultItem));
            NAPI_CALL(env, napi_set_element(env, jsResult, i, jsResultItem));
        }
        return jsResult;
    };

    return CreateAsyncWork(env, info, std::move(asyncContext), __func__);
}

napi_value NapiGetRemoteGrantStatus(const napi_env env, napi_callback_info info)
{
    auto asyncContext = std::unique_ptr<GetRemoteGrantStatusContext>(
        new (std::nothrow) GetRemoteGrantStatusContext());
    NAPI_THROW(env, asyncContext == nullptr, COMMON_INTERNAL_ERROR,
        "Failed to create GetRemoteGrantStatusContext");

    asyncContext->parse = [](napi_env env, napi_callback_info info, AgentFenceAsyncContext *context)
        -> napi_status {
        return napi_ok;
    };

    asyncContext->execute = [](napi_env env, void* data) {
        GetRemoteGrantStatusContext *asyncContext =
            static_cast<GetRemoteGrantStatusContext *>(data);
        asyncContext->result = SafAgentFence::GetRemoteGrantStatus(asyncContext->remoteGrantStatus);
    };

    asyncContext->resolve = [](napi_env env, AgentFenceAsyncContext *context) -> napi_value {
        GetRemoteGrantStatusContext *asyncContext =
            static_cast<GetRemoteGrantStatusContext *>(context);
        napi_value jsResult = nullptr;
        NAPI_CALL(env, napi_create_uint32(env, static_cast<uint32_t>(asyncContext->remoteGrantStatus),
            &jsResult));
        return jsResult;
    };

    return CreateAsyncWork(env, info, std::move(asyncContext), __func__);
}

napi_value NapiUpdateRemoteGrantStatus(const napi_env env, napi_callback_info info)
{
    auto asyncContext = std::unique_ptr<UpdateRemoteGrantStatusContext>(
        new (std::nothrow) UpdateRemoteGrantStatusContext());
    NAPI_THROW(env, asyncContext == nullptr, COMMON_INTERNAL_ERROR,
        "Failed to create UpdateRemoteGrantStatusContext");

    asyncContext->parse = [](napi_env env, napi_callback_info info, AgentFenceAsyncContext *context)
        -> napi_status {
        UpdateRemoteGrantStatusContext *asyncContext =
            static_cast<UpdateRemoteGrantStatusContext *>(context);
        size_t argc = 1;
        napi_value argv[1] = { nullptr };
        NAPI_CALL_RETURN_ERR(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
        NAPI_THROW_RETURN_ERR(env, argc < 1, GENERAL_PARAMETER_ERROR, "Invalid number of arguments");
        NAPI_CALL_RETURN_ERR(env, napi_get_value_uint32(env, argv[0],
            reinterpret_cast<uint32_t*>(&asyncContext->remoteGrantStatus)));
        return napi_ok;
    };

    asyncContext->execute = [](napi_env env, void* data) {
        UpdateRemoteGrantStatusContext *asyncContext =
            static_cast<UpdateRemoteGrantStatusContext *>(data);
        asyncContext->result = SafAgentFence::UpdateRemoteGrantStatus(asyncContext->remoteGrantStatus);
    };

    asyncContext->resolve = [](napi_env env, AgentFenceAsyncContext *context) -> napi_value {
        napi_value jsResult = nullptr;
        NAPI_CALL(env, napi_get_undefined(env, &jsResult));
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

napi_value DeclareRole(const napi_env env)
{
    napi_value role = nullptr;
    NAPI_CALL(env, napi_create_object(env, &role));
    AddUint32Property(env, role, "CONTROLLER", static_cast<uint32_t>(Role::CONTROLLER));
    AddUint32Property(env, role, "CONTROLLED", static_cast<uint32_t>(Role::CONTROLLED));
    return role;
}

napi_value DeclareRemoteGrantStatus(const napi_env env)
{
    napi_value status = nullptr;
    NAPI_CALL(env, napi_create_object(env, &status));
    AddUint32Property(env, status, "ENABLE", static_cast<uint32_t>(RemoteGrantStatus::ENABLE));
    AddUint32Property(env, status, "DISABLE", static_cast<uint32_t>(RemoteGrantStatus::DISABLE));
    return status;
}

napi_value Register(const napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("requestToolPermissions", NapiRequestToolPermissions),
        DECLARE_NAPI_FUNCTION("grantToolPermissionsByUser", NapiGrantToolPermissionsByUser),
        DECLARE_NAPI_FUNCTION("generateControlledDevicePackage", NapiGenerateControlledDevicePackage),
        DECLARE_NAPI_FUNCTION("verifyControlledDevicePackage", NapiVerifyControlledDevicePackage),
        DECLARE_NAPI_FUNCTION("generateControllerDevicePackage", NapiGenerateControllerDevicePackage),
        DECLARE_NAPI_FUNCTION("verifyControllerDevicePackage", NapiVerifyControllerDevicePackage),
        DECLARE_NAPI_FUNCTION("getRemoteGrantStatus", NapiGetRemoteGrantStatus),
        DECLARE_NAPI_FUNCTION("updateRemoteGrantStatus", NapiUpdateRemoteGrantStatus),

        DECLARE_NAPI_PROPERTY("OperationType", DeclareOperationType(env)),
        DECLARE_NAPI_PROPERTY("AuthStatus", DeclareAuthStatus(env)),
        DECLARE_NAPI_PROPERTY("Role", DeclareRole(env)),
        DECLARE_NAPI_PROPERTY("RemoteGrantStatus", DeclareRemoteGrantStatus(env)),
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
    .nm_priv = nullptr,
    .reserved = { 0 },
};

} // anonymous namespace

extern "C" __attribute__((constructor)) void RegisterModule(void)
{
    napi_module_register(&g_module);
}
