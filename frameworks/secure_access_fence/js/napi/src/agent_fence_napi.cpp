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
#include "securec.h"
#include "saf_result_defs.h"
#include "array_wrapper.h"
#include "int_wrapper.h"
#include "string_wrapper.h"
#include "saf_result_code.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "saf_log.h"
#include "secure_access_fence_type.h"
#include "saf_agent_fence.h"
#include "agent_fence_napi_context.h"
#include "napi_common.h"
#include "agent_fence_error_codes.h"

#include "ability.h"
#include "ui_content.h"
#include "want.h"
#include "event_handler.h"
#include "napi_base_context.h"

using namespace OHOS::Security::SAF;
using namespace OHOS::Security::SAF_ASSET_COMMON;

namespace {
constexpr size_t SINGLE_ARG_COUNT = 1;
constexpr size_t DOUBLE_ARG_COUNT = 2;
constexpr size_t VERIFY_CONTROLLER_PACKAGE_ARG_COUNT = DOUBLE_ARG_COUNT;
constexpr size_t REQUEST_TOOL_TICKET_ARG_COUNT = DOUBLE_ARG_COUNT;
constexpr size_t MIN_REQUEST_TOOL_TICKET_ARG_COUNT = SINGLE_ARG_COUNT;
constexpr int32_t DEFAULT_TICKET_EXPIRE_TIME_MS = 10000;
constexpr int32_t INVALID_SESSION_ID = 0;
constexpr uint32_t NAPI_REF_COUNT = 1;
napi_value BuildControlAppStatusResultArray(const napi_env env,
    const std::vector<ControlAppStatusResult> &resultList);

// UIExtensionAbility ability for simulated click control dialog.
// The bundle name is fetched from the closed-source side via IPC at runtime.
const std::string CONTROL_DIALOG_ABILITY_NAME = "AccessibilityControlAuthAbility";
const std::string EXTENSION_TYPE_KEY = "ability.want.params.uiExtensionType";
const std::string UI_EXTENSION_TYPE = "sys/commonUI";
// Want keys for caller �?UIExtensionAbility (input).

const std::string APP_INFO_LIST_KEY = "controlAppInfoList";
// Want keys for UIExtensionAbility �?caller (OnReceive output).
const std::string DECISIONS_KEY = "controlDecisions";

napi_value NapiRequestToolPermissions(const napi_env env, napi_callback_info info)
{
    auto asyncContext = std::unique_ptr<RequestToolPermissionContext>(new (std::nothrow)RequestToolPermissionContext());
    NAPI_THROW(env, asyncContext == nullptr, COMMON_INTERNAL_ERROR, "Failed to create RequestToolPermissionContext");
    asyncContext->parse = [](napi_env env, napi_callback_info info, AgentFenceAsyncContext *context)
        -> napi_status {
        RequestToolPermissionContext *asyncContext = static_cast<RequestToolPermissionContext *>(context);
        size_t argc = SINGLE_ARG_COUNT;
        napi_value argv[SINGLE_ARG_COUNT] = { nullptr };
        NAPI_CALL_RETURN_ERR(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
        NAPI_THROW_RETURN_ERR(env, argc < SINGLE_ARG_COUNT, GENERAL_PARAMETER_ERROR, "Invalid number of arguments");
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

napi_value NapiRequestToolTicket(const napi_env env, napi_callback_info info)
{
    auto asyncContext = std::unique_ptr<RequestToolTicketContext>(new (std::nothrow)RequestToolTicketContext());
    NAPI_THROW(env, asyncContext == nullptr, COMMON_INTERNAL_ERROR, "Failed to create RequestToolTicketContext");
    asyncContext->parse = [](napi_env env, napi_callback_info info, AgentFenceAsyncContext *context)
        -> napi_status {
        RequestToolTicketContext *asyncContext = static_cast<RequestToolTicketContext *>(context);
        size_t argc = REQUEST_TOOL_TICKET_ARG_COUNT;
        napi_value argv[REQUEST_TOOL_TICKET_ARG_COUNT] = { nullptr };
        NAPI_CALL_RETURN_ERR(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
        NAPI_THROW_RETURN_ERR(env, argc < MIN_REQUEST_TOOL_TICKET_ARG_COUNT,
            INVALID_PARAMETER, "Invalid number of arguments");
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, argv[0], asyncContext->cliCmdInfos));
        asyncContext->ticketOptions.ticketExpireTimeMs = DEFAULT_TICKET_EXPIRE_TIME_MS;
        if (argc >= REQUEST_TOOL_TICKET_ARG_COUNT) {
            NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, argv[1], asyncContext->ticketOptions));
        }
        return napi_ok;
    };

    asyncContext->execute = [](napi_env env, void* data) {
        RequestToolTicketContext *asyncContext = static_cast<RequestToolTicketContext *>(data);
        asyncContext->result = SafAgentFence::RequestToolTicket(asyncContext->cliCmdInfos,
            asyncContext->ticketOptions, asyncContext->ticketQueryResult);
    };

    asyncContext->resolve = [](napi_env env, AgentFenceAsyncContext *context) -> napi_value {
        RequestToolTicketContext *asyncContext = static_cast<RequestToolTicketContext *>(context);
        napi_value napiResult = nullptr;
        NAPI_CALL(env, napi_create_object(env, &napiResult));
        NAPI_CALL(env, NapiSetProperty(env, napiResult, "permissionResults",
            asyncContext->ticketQueryResult.permissionResults));
        NAPI_CALL(env, NapiSetProperty(env, napiResult, "ticket",
            asyncContext->ticketQueryResult.ticket));
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
        size_t argc = SINGLE_ARG_COUNT;
        napi_value argv[SINGLE_ARG_COUNT] = { nullptr };
        NAPI_CALL_RETURN_ERR(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
        NAPI_THROW_RETURN_ERR(env, argc < SINGLE_ARG_COUNT, GENERAL_PARAMETER_ERROR, "Invalid number of arguments");
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
        size_t argc = SINGLE_ARG_COUNT;
        napi_value argv[SINGLE_ARG_COUNT] = { nullptr };
        NAPI_CALL_RETURN_ERR(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
        NAPI_THROW_RETURN_ERR(env, argc < SINGLE_ARG_COUNT, GENERAL_PARAMETER_ERROR, "Invalid number of arguments");
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
        size_t argc = SINGLE_ARG_COUNT;
        napi_value argv[SINGLE_ARG_COUNT] = { nullptr };
        NAPI_CALL_RETURN_ERR(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
        NAPI_THROW_RETURN_ERR(env, argc < SINGLE_ARG_COUNT, GENERAL_PARAMETER_ERROR, "Invalid number of arguments");
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
        size_t argc = SINGLE_ARG_COUNT;
        napi_value argv[SINGLE_ARG_COUNT] = { nullptr };
        NAPI_CALL_RETURN_ERR(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
        NAPI_THROW_RETURN_ERR(env, argc < SINGLE_ARG_COUNT, GENERAL_PARAMETER_ERROR, "Invalid number of arguments");
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
        size_t argc = SINGLE_ARG_COUNT;
        napi_value argv[SINGLE_ARG_COUNT] = { nullptr };
        NAPI_CALL_RETURN_ERR(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
        NAPI_THROW_RETURN_ERR(env, argc < SINGLE_ARG_COUNT, GENERAL_PARAMETER_ERROR, "Invalid number of arguments");
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

napi_value NapiGenerateTaskChallenge(const napi_env env, napi_callback_info info)
{
    auto asyncContext = std::unique_ptr<GenerateTaskChallengeContext>(
        new (std::nothrow) GenerateTaskChallengeContext());
    NAPI_THROW(env, asyncContext == nullptr, COMMON_INTERNAL_ERROR,
        "Failed to create GenerateTaskChallengeContext");

    asyncContext->parse = [](napi_env env, napi_callback_info info, AgentFenceAsyncContext *context)
        -> napi_status {
        GenerateTaskChallengeContext *asyncContext = static_cast<GenerateTaskChallengeContext *>(context);
        size_t argc = 1;
        napi_value argv[1] = { nullptr };
        NAPI_CALL_RETURN_ERR(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
        NAPI_THROW_RETURN_ERR(env, argc < 1, GENERAL_PARAMETER_ERROR, "Invalid number of arguments");
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, argv[0], asyncContext->taskIdentityInfo));
        NAPI_CALL_RETURN_ERR(env, ValidateTaskIdentityInfo(env, asyncContext->taskIdentityInfo));
        return napi_ok;
    };

    asyncContext->execute = [](napi_env env, void* data) {
        GenerateTaskChallengeContext *asyncContext = static_cast<GenerateTaskChallengeContext *>(data);
        asyncContext->result = SafAgentFence::GenerateTaskChallenge(
            asyncContext->taskIdentityInfo, asyncContext->challenge);
    };

    asyncContext->resolve = [](napi_env env, AgentFenceAsyncContext *context) -> napi_value {
        GenerateTaskChallengeContext *asyncContext = static_cast<GenerateTaskChallengeContext *>(context);
        void *data = nullptr;
        napi_value arrayBuffer = nullptr;
        size_t challengeSize = asyncContext->challenge.size();
        NAPI_CALL(env, napi_create_arraybuffer(env, challengeSize, &data, &arrayBuffer));
        uint8_t *bytes = static_cast<uint8_t *>(data);
        for (size_t i = 0; i < challengeSize; ++i) {
            bytes[i] = static_cast<uint8_t>(asyncContext->challenge[i]);
        }
        napi_value jsResult = nullptr;
        NAPI_CALL(env, napi_create_typedarray(env, napi_uint8_array,
            challengeSize, arrayBuffer, 0, &jsResult));
        return jsResult;
    };

    return CreateAsyncWork(env, info, std::move(asyncContext), __func__);
}

napi_value NapiGenerateTaskIdentity(const napi_env env, napi_callback_info info)
{
    auto asyncContext = std::unique_ptr<GenerateTaskIdentityContext>(
        new (std::nothrow) GenerateTaskIdentityContext());
    NAPI_THROW(env, asyncContext == nullptr, COMMON_INTERNAL_ERROR,
        "Failed to create GenerateTaskIdentityContext");

    asyncContext->parse = [](napi_env env, napi_callback_info info, AgentFenceAsyncContext *context)
        -> napi_status {
        GenerateTaskIdentityContext *asyncContext = static_cast<GenerateTaskIdentityContext *>(context);
        size_t argc = 1;
        napi_value argv[1] = { nullptr };
        NAPI_CALL_RETURN_ERR(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
        NAPI_THROW_RETURN_ERR(env, argc < 1, GENERAL_PARAMETER_ERROR, "Invalid number of arguments");
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, argv[0], asyncContext->taskIdentityInfo));
        NAPI_CALL_RETURN_ERR(env, ValidateTaskIdentityInfo(env, asyncContext->taskIdentityInfo));
        return napi_ok;
    };

    asyncContext->execute = [](napi_env env, void* data) {
        GenerateTaskIdentityContext *asyncContext = static_cast<GenerateTaskIdentityContext *>(data);
        asyncContext->result = SafAgentFence::GenerateTaskIdentity(
            asyncContext->taskIdentityInfo, asyncContext->taskIdentity);
    };

    asyncContext->resolve = [](napi_env env, AgentFenceAsyncContext *context) -> napi_value {
        GenerateTaskIdentityContext *asyncContext = static_cast<GenerateTaskIdentityContext *>(context);
        napi_value jsResult = nullptr;
        NAPI_CALL(env, napi_create_object(env, &jsResult));
        NAPI_CALL(env, NapiSetProperty(env, jsResult, "taskAttributes",
            asyncContext->taskIdentity.taskAttributes));
        NAPI_CALL(env, NapiSetProperty(env, jsResult, "identity",
            asyncContext->taskIdentity.identity));
        NAPI_CALL(env, NapiSetProperty(env, jsResult, "challenge",
            asyncContext->taskIdentity.challenge));
        return jsResult;
    };

    return CreateAsyncWork(env, info, std::move(asyncContext), __func__);
}

napi_value NapiRevokeTaskIdentity(const napi_env env, napi_callback_info info)
{
    auto asyncContext = std::unique_ptr<RevokeTaskIdentityContext>(
        new (std::nothrow) RevokeTaskIdentityContext());
    NAPI_THROW(env, asyncContext == nullptr, COMMON_INTERNAL_ERROR,
        "Failed to create RevokeTaskIdentityContext");

    asyncContext->parse = [](napi_env env, napi_callback_info info, AgentFenceAsyncContext *context)
        -> napi_status {
        RevokeTaskIdentityContext *asyncContext = static_cast<RevokeTaskIdentityContext *>(context);
        size_t argc = 1;
        napi_value argv[1] = { nullptr };
        NAPI_CALL_RETURN_ERR(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
        NAPI_THROW_RETURN_ERR(env, argc < 1, GENERAL_PARAMETER_ERROR, "Invalid number of arguments");
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, argv[0], asyncContext->taskIds));
        NAPI_THROW_RETURN_ERR(env, asyncContext->taskIds.size() > MAX_REVOKE_TASK_IDS,
            INVALID_PARAMETER, "taskIds exceeds max count");
        return napi_ok;
    };

    asyncContext->execute = [](napi_env env, void* data) {
        RevokeTaskIdentityContext *asyncContext = static_cast<RevokeTaskIdentityContext *>(data);
        asyncContext->result = SafAgentFence::RevokeTaskIdentity(asyncContext->taskIds);
    };

    asyncContext->resolve = [](napi_env env, AgentFenceAsyncContext *context) -> napi_value {
        napi_value jsResult = nullptr;
        NAPI_CALL(env, napi_get_undefined(env, &jsResult));
        return jsResult;
    };

    return CreateAsyncWork(env, info, std::move(asyncContext), __func__);
}

// ModalUIExtension dialog implementation for requestControlAppPermissionFromUser.
// ---------------------------------------------------------------------------

// Extract a string array from WantParams into out.
static void ExtractStringArray(const OHOS::AAFwk::WantParams &receive,
    const std::string &key, std::vector<std::string> &out)
{
    out.clear();
    auto value = receive.GetParam(key);
    auto array = OHOS::AAFwk::IArray::Query(value);
    if (array == nullptr || !OHOS::AAFwk::Array::IsStringArray(array)) {
        return;
    }
    OHOS::AAFwk::Array::ForEach(array, [&](OHOS::AAFwk::IInterface *obj) {
        if (obj == nullptr) {
            return;
        }
        auto str = OHOS::AAFwk::IString::Query(obj);
        if (str != nullptr) {
            out.push_back(OHOS::AAFwk::String::Unbox(str));
        }
    });
}

// Extract an int32_t array from WantParams into out.
static void ExtractIntArray(const OHOS::AAFwk::WantParams &receive,
    const std::string &key, std::vector<int32_t> &out)
{
    out.clear();
    auto value = receive.GetParam(key);
    auto array = OHOS::AAFwk::IArray::Query(value);
    if (array == nullptr || !OHOS::AAFwk::Array::IsIntegerArray(array)) {
        return;
    }
    OHOS::AAFwk::Array::ForEach(array, [&](OHOS::AAFwk::IInterface *obj) {
        if (obj == nullptr) {
            return;
        }
        auto val = OHOS::AAFwk::IInteger::Query(obj);
        if (val != nullptr) {
            out.push_back(OHOS::AAFwk::Integer::Unbox(val));
        }
    });
}

class ControlAppUICallback {
public:
    explicit ControlAppUICallback(std::shared_ptr<RequestControlAppPermissionContext::DialogData> &data)
        : data_(data)
    {}
    ~ControlAppUICallback() = default;
    void SetSessionId(int32_t sessionId) { sessionId_ = sessionId; }

    void OnRelease(int32_t releaseCode)
    {
        LOGI("[ControlAppUICallback] OnRelease code=%{public}d", releaseCode);
        ReleaseHandler(SAF_ERR_SERVICE_UNAVAILABLE);
    }

    void OnReceive(const OHOS::AAFwk::WantParams &receive)
    {
        if (data_ == nullptr) {
            LOGE("[ControlAppUICallback] OnReceive data is null");
            return;
        }
        LOGI("[ControlAppUICallback] OnReceive called");
        ExtractStringArray(receive, APP_INFO_LIST_KEY, data_->resultAppInfoList);
        ExtractIntArray(receive, DECISIONS_KEY, data_->resultDecisions);
        ReleaseHandler(SAF_SUCCESS);
    }

    void OnError(int32_t code, const std::string &name, const std::string &message)
    {
        LOGE("[ControlAppUICallback] OnError code=%{public}d name=%{public}s msg=%{public}s",
            code, name.c_str(), message.c_str());
        ReleaseHandler(SAF_ERR_SERVICE_UNAVAILABLE);
    }

    void OnRemoteReady(const std::shared_ptr<OHOS::Ace::ModalUIExtensionProxy> &uiProxy)
    {
        LOGI("[ControlAppUICallback] OnRemoteReady connected");
    }

    void OnDestroy()
    {
        LOGI("[ControlAppUICallback] OnDestroy");
        if (data_ != nullptr && !data_->releaseFlag) {
            LOGW("[ControlAppUICallback] OnDestroy without OnResult, treat as cancel");
            ReleaseHandler(SAF_ERR_SERVICE_UNAVAILABLE);
        }
    }

    void ReleaseHandler(int32_t code)
    {
        if (data_ == nullptr) {
            LOGE("[ControlAppUICallback] ReleaseHandler data is null");
            return;
        }
        {
            std::lock_guard<std::mutex> lk(data_->mutex);
            if (data_->releaseFlag) {
                LOGW("[ControlAppUICallback] already released");
                return;
            }
            data_->releaseFlag = true;
            data_->resultCode = code;
            data_->done = true;
        }
        data_->cv.notify_one();
        // Note: CloseModalUIExtension + ExecCallbackByInstanceId are handled by the
        // execute thread after waking up, to avoid re-entry into the main thread from
        // a main-thread callback (CloseModalUIExtension must PostSyncTask to main).
    }

private:
    int32_t sessionId_ = INVALID_SESSION_ID;
    std::shared_ptr<RequestControlAppPermissionContext::DialogData> data_ = nullptr;
};

static OHOS::Ace::UIContent *GetUIContentFromContext(RequestControlAppPermissionContext *ctx)
{
    if (ctx == nullptr || ctx->contextRef == nullptr) {
        LOGE("[GetUIContentFromContext] ctx or contextRef is null");
        return nullptr;
    }
    napi_env env = ctx->env;
    napi_value contextValue = nullptr;
    napi_status status = napi_get_reference_value(env, ctx->contextRef, &contextValue);
    if (status != napi_ok || contextValue == nullptr) {
        LOGE("[GetUIContentFromContext] get reference value failed");
        return nullptr;
    }
    auto stageContext = OHOS::AbilityRuntime::GetStageModeContext(env, contextValue);
    if (stageContext == nullptr) {
        LOGE("[GetUIContentFromContext] GetStageModeContext failed");
        return nullptr;
    }
    auto abilityContext = OHOS::AbilityRuntime::Context::ConvertTo<OHOS::AbilityRuntime::AbilityContext>(stageContext);
    if (abilityContext == nullptr) {
        LOGE("[GetUIContentFromContext] ConvertTo AbilityContext failed");
        return nullptr;
    }
    return abilityContext->GetUIContent();
}

static void CreateUIExtensionMainThread(RequestControlAppPermissionContext *ctx,
    OHOS::AAFwk::Want &want,
    const OHOS::Ace::ModalUIExtensionCallbacks &callbacks,
    std::shared_ptr<ControlAppUICallback> &uiCallback)
{
    auto task = [ctx, want, callbacks, uiCallback]() {
        auto uiContent = GetUIContentFromContext(ctx);
        if (uiContent == nullptr) {
            LOGE("[CreateUIExtensionMainThread] GetUIContent failed");
            {
                std::lock_guard<std::mutex> lk(ctx->dialogData->mutex);
                ctx->dialogData->done = true;
                ctx->dialogData->resultCode = SAF_ERR_SERVICE_UNAVAILABLE;
            }
            ctx->dialogData->cv.notify_one();
            return;
        }
        OHOS::Ace::ModalUIExtensionConfig config;
        config.isProhibitBack = true;
        config.isModalRequestFocus = false;
        int32_t sessionId = uiContent->CreateModalUIExtension(want, callbacks, config);
        LOGI("[CreateUIExtensionMainThread] sessionId=%{public}d", sessionId);
        if (sessionId == INVALID_SESSION_ID) {
            LOGE("[CreateUIExtensionMainThread] CreateModalUIExtension failed");
            {
                std::lock_guard<std::mutex> lk(ctx->dialogData->mutex);
                ctx->dialogData->done = true;
                ctx->dialogData->resultCode = SAF_ERR_SERVICE_UNAVAILABLE;
            }
            ctx->dialogData->cv.notify_one();
            return;
        }
        uiCallback->SetSessionId(sessionId);
        ctx->dialogData->sessionId = sessionId;
        ctx->sessionId = sessionId;
    };
    if (ctx->handler != nullptr) {
        ctx->handler->PostSyncTask(task, "SAF:CreateUIExtensionMainThread");
    } else {
        task();
    }
}

static void CloseModalUIExtensionMainThread(RequestControlAppPermissionContext *ctx, int32_t sessionId)
{
    if (ctx == nullptr || sessionId <= INVALID_SESSION_ID) {
        return;
    }
    auto task = [ctx, sessionId]() {
        auto uiContent = GetUIContentFromContext(ctx);
        if (uiContent == nullptr) {
            LOGE("[CloseModalUIExtensionMainThread] GetUIContent failed");
            return;
        }
        uiContent->CloseModalUIExtension(sessionId);
        LOGI("[CloseModalUIExtensionMainThread] sessionId=%{public}d closed", sessionId);
    };
    if (ctx->handler != nullptr) {
        ctx->handler->PostSyncTask(task, "SAF:CloseModalUIExtensionMainThread");
    } else {
        task();
    }
}

// Pop up the ModalUIExtension dialog. Returns SAF_SUCCESS on success.
// ctx->dialogData must be initialized before calling this.
static int32_t PopControlAppDialog(RequestControlAppPermissionContext *ctx,
    const std::string &callerBundleName, int32_t callerIndex,
    const std::vector<AppInfo> &unauthorizedAppList)
{
    LOGI("[PopControlAppDialog] caller=%{public}s:%{public}d, appCount=%{public}zu",
        callerBundleName.c_str(), callerIndex, unauthorizedAppList.size());
    
    // Fetch bundle name from closed-source side via IPC
    std::string bundleName;
    int32_t ret = SafAgentFence::GetControlDialogBundleName(bundleName);
    if (ret != SAF_SUCCESS) {
        LOGE("[PopControlAppDialog] GetControlDialogBundleName failed, ret=%{public}d", ret);
        return ret;
    }
    // Build Want params
    OHOS::AAFwk::Want want;
    want.SetElementName(bundleName, CONTROL_DIALOG_ABILITY_NAME);
    want.SetParam(EXTENSION_TYPE_KEY, UI_EXTENSION_TYPE);
    std::vector<std::string> appInfoStrList;
    appInfoStrList.reserve(unauthorizedAppList.size());
    for (const auto &appInfo : unauthorizedAppList) {
        appInfoStrList.push_back(std::to_string(appInfo.index) + "_" + appInfo.bundleName);
    }
    want.SetParam(APP_INFO_LIST_KEY, appInfoStrList);

    auto uiCallback = std::make_shared<ControlAppUICallback>(ctx->dialogData);
    OHOS::Ace::ModalUIExtensionCallbacks callbacks = {
        [uiCallback](int32_t releaseCode) { uiCallback->OnRelease(releaseCode); },
        [](int32_t resultCode, const OHOS::AAFwk::Want &result) {
            // no-op: OnResult is no longer used; results come through onReceive
        },
        [uiCallback](const OHOS::AAFwk::WantParams &receive) { uiCallback->OnReceive(receive); },
        [uiCallback](int32_t code, const std::string &name, const std::string &message) {
            uiCallback->OnError(code, name, message);
        },
        [uiCallback](const std::shared_ptr<OHOS::Ace::ModalUIExtensionProxy> &uiProxy) {
            uiCallback->OnRemoteReady(uiProxy);
        },
        [uiCallback]() { uiCallback->OnDestroy(); },
    };

    CreateUIExtensionMainThread(ctx, want, callbacks, uiCallback);
    return SAF_SUCCESS;
}

// Parse user decisions returned from UIExtensionAbility into userDecisions + resultList.
static int32_t ParseDialogResult(RequestControlAppPermissionContext *ctx,
    const std::vector<AppInfo> &unauthorizedAppList)
{
    auto &data = ctx->dialogData;
    if (data->resultAppInfoList.size() != data->resultDecisions.size()) {
        LOGE("[ParseDialogResult] size mismatch: appInfoList=%{public}zu decisions=%{public}zu",
            data->resultAppInfoList.size(), data->resultDecisions.size());
        return SAF_ERR_SERVICE_UNAVAILABLE;
    }
    ctx->userDecisions.clear();
    ctx->resultList.clear();
    // Build a lookup map: "index_bundleName" 锟?decision
    std::map<std::string, int32_t> decisionMap;
    for (size_t i = 0; i < data->resultAppInfoList.size(); ++i) {
        decisionMap[data->resultAppInfoList[i]] = data->resultDecisions[i];
    }
    for (const auto &appInfo : unauthorizedAppList) {
        std::string key = std::to_string(appInfo.index) + "_" + appInfo.bundleName;
        auto it = decisionMap.find(key);
        if (it == decisionMap.end()) {
            // Not found in result 锟?user cancelled this one 锟?DENY
            ctx->userDecisions.emplace_back(ControlAppStatus::DENY);
        } else {
            int32_t decision = it->second;
            if (decision == static_cast<int32_t>(ControlAppStatus::ALLOW)) {
                ctx->userDecisions.emplace_back(ControlAppStatus::ALLOW);
            } else {
                ctx->userDecisions.emplace_back(ControlAppStatus::DENY);
            }
        }
        ControlAppStatusResult result;
        result.appInfo = appInfo;
        result.status = ctx->userDecisions.back();
        ctx->resultList.emplace_back(result);
    }
    LOGI("[ParseDialogResult] parsed %{public}zu decisions", ctx->userDecisions.size());
    return SAF_SUCCESS;
}

// Extract caller bundleName from Context for dialog display.
static void ExtractCallerBundleName(napi_env env, napi_value contextValue,
    RequestControlAppPermissionContext *asyncContext)
{
    auto stageContext = OHOS::AbilityRuntime::GetStageModeContext(env, contextValue);
    if (stageContext == nullptr) {
        return;
    }
    auto appInfo = stageContext->GetApplicationInfo();
    if (appInfo != nullptr) {
        asyncContext->callerBundleName = appInfo->bundleName;
    }
    LOGI("[RequestControlAppPermission] callerBundleName=%{public}s",
        asyncContext->callerBundleName.c_str());
}

// Filter unauthorized apps (status == DENY) from queryResultList.
static void FilterUnauthorizedApps(RequestControlAppPermissionContext *asyncContext)
{
    for (const auto &queryResult : asyncContext->queryResultList) {
        if (queryResult.status == ControlAppStatus::DENY) {
            asyncContext->unauthorizedAppList.emplace_back(queryResult.appInfo);
        }
    }
}

// Merge authorized apps (ALLOW from query) + unauthorized apps (user decisions) into resultList.
static void MergeControlAppResults(RequestControlAppPermissionContext *asyncContext)
{
    asyncContext->resultList.clear();
    for (const auto &queryResult : asyncContext->queryResultList) {
        if (queryResult.status == ControlAppStatus::ALLOW) {
            asyncContext->resultList.emplace_back(queryResult);
        }
    }
    for (size_t i = 0; i < asyncContext->unauthorizedAppList.size(); ++i) {
        ControlAppStatusResult result;
        result.appInfo = asyncContext->unauthorizedAppList[i];
        result.status = asyncContext->userDecisions[i];
        asyncContext->resultList.emplace_back(result);
    }
}

// Pop up dialog, wait for result, parse decisions. Returns SAF_SUCCESS on success.
static int32_t PopDialogAndWaitResult(RequestControlAppPermissionContext *asyncContext)
{
    asyncContext->dialogData = std::make_shared<RequestControlAppPermissionContext::DialogData>();
    asyncContext->handler = std::make_shared<OHOS::AppExecFwk::EventHandler>(
        OHOS::AppExecFwk::EventRunner::GetMainEventRunner());

    int32_t popRet = PopControlAppDialog(asyncContext,
        asyncContext->callerBundleName, asyncContext->callerIndex,
        asyncContext->unauthorizedAppList);
    if (popRet != SAF_SUCCESS) {
        return popRet;
    }

    {
        std::unique_lock<std::mutex> lk(asyncContext->dialogData->mutex);
        asyncContext->dialogData->cv.wait(lk,
            [asyncContext]() { return asyncContext->dialogData->done; });
    }
    int32_t dialogResult = asyncContext->dialogData->resultCode;
    if (dialogResult != SAF_SUCCESS) {
        LOGE("[RequestControlAppPermission] dialog failed, ret=%{public}d", dialogResult);
        CloseModalUIExtensionMainThread(asyncContext, asyncContext->sessionId);
        return dialogResult;
    }

    int32_t parseRet = ParseDialogResult(asyncContext, asyncContext->unauthorizedAppList);
    if (parseRet != SAF_SUCCESS) {
        CloseModalUIExtensionMainThread(asyncContext, asyncContext->sessionId);
        return parseRet;
    }

    CloseModalUIExtensionMainThread(asyncContext, asyncContext->sessionId);
    return SAF_SUCCESS;
}

napi_value NapiQueryControlAppStatus(const napi_env env, napi_callback_info info)
{
    auto asyncContext = std::unique_ptr<QueryControlAppStatusContext>(
        new (std::nothrow) QueryControlAppStatusContext());
    NAPI_THROW(env, asyncContext == nullptr, COMMON_INTERNAL_ERROR,
        "Failed to create QueryControlAppStatusContext");

    asyncContext->parse = [](napi_env env, napi_callback_info info, AgentFenceAsyncContext *context)
        -> napi_status {
        QueryControlAppStatusContext *asyncContext =
            static_cast<QueryControlAppStatusContext *>(context);
        size_t argc = SINGLE_ARG_COUNT;
        napi_value argv[SINGLE_ARG_COUNT] = { nullptr };
        NAPI_CALL_RETURN_ERR(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
        NAPI_THROW_RETURN_ERR(env, argc < SINGLE_ARG_COUNT, GENERAL_PARAMETER_ERROR, "Invalid number of arguments");
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, argv[0], asyncContext->appInfoList));
        return napi_ok;
    };

    asyncContext->execute = [](napi_env env, void* data) {
        QueryControlAppStatusContext *asyncContext =
            static_cast<QueryControlAppStatusContext *>(data);
        asyncContext->result = SafAgentFence::QueryControlAppStatus(
            asyncContext->appInfoList, asyncContext->resultList);
    };

    asyncContext->resolve = [](napi_env env, AgentFenceAsyncContext *context) -> napi_value {
        QueryControlAppStatusContext *asyncContext =
            static_cast<QueryControlAppStatusContext *>(context);
        return BuildControlAppStatusResultArray(env, asyncContext->resultList);
    };

    return CreateAsyncWork(env, info, std::move(asyncContext), __func__);
}

static void ExecuteRequestControlAppPermission(RequestControlAppPermissionContext *asyncContext)
{
    asyncContext->result = SafAgentFence::QueryControlAppStatus(
        asyncContext->appInfoList, asyncContext->queryResultList);
    if (asyncContext->result != SAF_SUCCESS) {
        return;
    }

    FilterUnauthorizedApps(asyncContext);

    if (asyncContext->unauthorizedAppList.empty()) {
        asyncContext->resultList = asyncContext->queryResultList;
        return;
    }

    int32_t dialogRet = PopDialogAndWaitResult(asyncContext);
    if (dialogRet != SAF_SUCCESS) {
        asyncContext->result = dialogRet;
        return;
    }

    MergeControlAppResults(asyncContext);
}

napi_value NapiRequestControlAppPermissionFromUser(const napi_env env, napi_callback_info info)
{
    auto asyncContext = std::unique_ptr<RequestControlAppPermissionContext>(
        new (std::nothrow) RequestControlAppPermissionContext());
    NAPI_THROW(env, asyncContext == nullptr, COMMON_INTERNAL_ERROR,
        "Failed to create RequestControlAppPermissionContext");

    asyncContext->parse = [](napi_env env, napi_callback_info info, AgentFenceAsyncContext *context)
        -> napi_status {
        RequestControlAppPermissionContext *asyncContext =
            static_cast<RequestControlAppPermissionContext *>(context);
        size_t argc = DOUBLE_ARG_COUNT;
        napi_value argv[DOUBLE_ARG_COUNT] = { nullptr, nullptr };
        NAPI_CALL_RETURN_ERR(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
        NAPI_THROW_RETURN_ERR(env, argc < DOUBLE_ARG_COUNT, GENERAL_PARAMETER_ERROR, "Invalid number of arguments");
        NAPI_CALL_RETURN_ERR(env, napi_create_reference(env, argv[0], NAPI_REF_COUNT, &asyncContext->contextRef));
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, argv[1], asyncContext->appInfoList));
        ExtractCallerBundleName(env, argv[0], asyncContext);
        return napi_ok;
    };

    asyncContext->execute = [](napi_env env, void* data) {
        auto *asyncContext = static_cast<RequestControlAppPermissionContext *>(data);
        ExecuteRequestControlAppPermission(asyncContext);
    };

    asyncContext->resolve = [](napi_env env, AgentFenceAsyncContext *context) -> napi_value {
        auto *asyncContext = static_cast<RequestControlAppPermissionContext *>(context);
        return BuildControlAppStatusResultArray(env, asyncContext->resultList);
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

napi_value DeclareExecuteType(const napi_env env)
{
    napi_value executeType = nullptr;
    NAPI_CALL(env, napi_create_object(env, &executeType));
    AddUint32Property(env, executeType, "INSTANT", static_cast<uint32_t>(ExecuteType::INSTANT));
    AddUint32Property(env, executeType, "DELAYED", static_cast<uint32_t>(ExecuteType::DELAYED));
    return executeType;
}

napi_value DeclareControlAppStatus(const napi_env env)
{
    napi_value status = nullptr;
    NAPI_CALL(env, napi_create_object(env, &status));
    AddUint32Property(env, status, "ALLOW", static_cast<uint32_t>(ControlAppStatus::ALLOW));
    AddUint32Property(env, status, "DENY", static_cast<uint32_t>(ControlAppStatus::DENY));
    return status;
}

napi_value BuildControlAppStatusResultArray(const napi_env env,
    const std::vector<ControlAppStatusResult> &resultList)
{
    napi_value jsResult = nullptr;
    NAPI_CALL(env, napi_create_array(env, &jsResult));
    for (uint32_t i = 0; i < resultList.size(); ++i) {
        napi_value jsResultItem = nullptr;
        NAPI_CALL(env, napi_create_object(env, &jsResultItem));
        NAPI_CALL(env, NapiSetProperty(env, jsResultItem, "appInfo", resultList[i].appInfo));
        NAPI_CALL(env, NapiSetProperty(env, jsResultItem, "status",
            static_cast<int32_t>(resultList[i].status)));
        NAPI_CALL(env, napi_set_element(env, jsResult, i, jsResultItem));
    }
    return jsResult;
}

napi_value Register(const napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("requestToolPermissions", NapiRequestToolPermissions),
        DECLARE_NAPI_FUNCTION("requestToolTicket", NapiRequestToolTicket),
        DECLARE_NAPI_FUNCTION("grantToolPermissionsByUser", NapiGrantToolPermissionsByUser),
        DECLARE_NAPI_FUNCTION("generateControlledDevicePackage", NapiGenerateControlledDevicePackage),
        DECLARE_NAPI_FUNCTION("verifyControlledDevicePackage", NapiVerifyControlledDevicePackage),
        DECLARE_NAPI_FUNCTION("generateControllerDevicePackage", NapiGenerateControllerDevicePackage),
        DECLARE_NAPI_FUNCTION("verifyControllerDevicePackage", NapiVerifyControllerDevicePackage),
        DECLARE_NAPI_FUNCTION("getRemoteGrantStatus", NapiGetRemoteGrantStatus),
        DECLARE_NAPI_FUNCTION("updateRemoteGrantStatus", NapiUpdateRemoteGrantStatus),
        DECLARE_NAPI_FUNCTION("generateTaskChallenge", NapiGenerateTaskChallenge),
        DECLARE_NAPI_FUNCTION("generateTaskIdentity", NapiGenerateTaskIdentity),
        DECLARE_NAPI_FUNCTION("revokeTaskIdentity", NapiRevokeTaskIdentity),
        DECLARE_NAPI_FUNCTION("queryControlAppStatus", NapiQueryControlAppStatus),
        DECLARE_NAPI_FUNCTION("requestControlAppPermissionFromUser", NapiRequestControlAppPermissionFromUser),

        DECLARE_NAPI_PROPERTY("OperationType", DeclareOperationType(env)),
        DECLARE_NAPI_PROPERTY("ExecuteType", DeclareExecuteType(env)),
        DECLARE_NAPI_PROPERTY("AuthStatus", DeclareAuthStatus(env)),
        DECLARE_NAPI_PROPERTY("Role", DeclareRole(env)),
        DECLARE_NAPI_PROPERTY("RemoteGrantStatus", DeclareRemoteGrantStatus(env)),
        DECLARE_NAPI_PROPERTY("ControlAppStatus", DeclareControlAppStatus(env)),
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
