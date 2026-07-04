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
#include "napi_common.h"
#include "secure_access_fence_type.h"
#include "saf_log.h"
#include "saf_result_code.h"
#include "agent_fence_error_codes.h"

namespace OHOS {
namespace Security {
namespace SAF_ASSET_COMMON {
namespace {
    #define MAX_BUFF_SIZE 4096 // 4KB
    constexpr uint32_t MAX_PERMISSION_NAME_SIZE = 256;
    AgentFenceErrorCode MapErrorCode(const int32_t safResult)
    {
        switch (safResult) {
            case SAF_ERR_PERMISSION_DENIED:
                return PERMISSION_DENAIL;
            case SAF_ERR_NOT_SYSTEM_APP:
                return NOT_SYSTEM_APP;
            case SAF_ERR_ARG_INVALID:
                return INVALID_PARAMETER;
            case SAF_ERR_SCREENLOCK_IS_LOCKED:
                return OPERATION_FAILED_UNDER_SCREEN_LOCK;
            case SAF_ERR_TRUSTED_RING_REMOTE_TOKEN_EXPIRED:
                return REMOTE_TOKEN_EXPIRED;
            case SAF_ERR_TRUSTED_RING_REMOTE_DEVICE_UNTRUSTED:
                return REMOTE_DEVICE_UNTRUSTED;
            case SAF_ERR_TRUSTED_RING_LACK_OF_AUTH_TOKEN:
                return LACK_AUTH_TOKEN;
            case SAF_ERR_TRUSTED_RING_AUTH_TOKEN_IS_EXPIRED:
                return AUTH_TOKEN_EXPIRED;
            case SAF_ERR_NO_NETWORK:
                return ENVIRONMENT_ERROR;
            case SAF_ERR_ACCOUNT_NOT_LOGGED_IN:
                return ENVIRONMENT_ERROR;
            case SAF_ERR_QUERIED_PERMISSION_NOT_EXIST:
                return INVALID_PERMISSION;
        }
        return COMMON_INTERNAL_ERROR;
    }

    std::string GetErrorMessage(const AgentFenceErrorCode errorCode)
    {
        switch (errorCode) {
            case PERMISSION_DENAIL:
                return "Permission denied. Caller does not have the required permission";
            case NOT_SYSTEM_APP:
                return "The caller is not a system application.";
            case GENERAL_PARAMETER_ERROR:
                return "Invalid parameter.";
            case INVALID_PARAMETER:
                return "Invalid parameter. Passed in parameter is invalid";
            case SERVICE_ABNORMAL:
                return "Service is abnormal.";
            case COMMON_INTERNAL_ERROR:
                return "Common internal error. An internal error occurs when querying CLI "
                    "permissions or runtime permission information.";
            case ENVIRONMENT_ERROR:
                return "The account is not logged in, network is unavailable, timeout, etc.";
            case INVALID_PERMISSION:
                return "Invalid permission. A permission in permissionInfo does not exist.";
            case GRANT_PERMISSION_FAILED:
                return "Grant permission failed. The application specified by the tokenID is"
                    "not allowed to be branted with the specified permission,"
                    "the specified permission cannot be granted by user, etc.";
            case OPERATION_FAILED_UNDER_SCREEN_LOCK:
                return "The requested operation is not allowed to be execute while the device is locked";
            case REMOTE_TOKEN_EXPIRED:
                return "remote token is expired.";
            case REMOTE_DEVICE_UNTRUSTED:
                return "The remote device is untrusted.";
            case LACK_AUTH_TOKEN:
                return "Lack auth token.";
            case AUTH_TOKEN_EXPIRED:
                return "The auth token is expired.";
            default:
                return "Unknown error";
        }
    }

    void ResolvePromise(napi_env env, SAF::AgentFenceAsyncContext *context)
    {
        if (context->result == SAF_SUCCESS) {
            napi_value result = context->resolve(env, context);
            NAPI_CALL_RETURN_VOID(env, napi_resolve_deferred(env, context->deferred, result));
        } else {
            AgentFenceErrorCode errCode = MapErrorCode(context->result);
            napi_value result = NapiCreateError(env, static_cast<int32_t>(errCode), GetErrorMessage(errCode).c_str());
            NAPI_CALL_RETURN_VOID(env, napi_reject_deferred(env, context->deferred, result));
        }
    }

    int32_t GetPermissionStatus(const int32_t permissionStatus, SAF::PermissionStatus& safPermissionStatus)
    {
        switch (static_cast<SAF::PermissionStatus>(permissionStatus)) {
            case SAF::PermissionStatus::DENIED:
                safPermissionStatus = SAF::PermissionStatus::DENIED;
                break;
            case SAF::PermissionStatus::GRANTED:
                safPermissionStatus = SAF::PermissionStatus::GRANTED;
                break;
            case SAF::PermissionStatus::NOT_DETERMINED:
                safPermissionStatus = SAF::PermissionStatus::NOT_DETERMINED;
                break;
            case SAF::PermissionStatus::INVALID:
                safPermissionStatus = SAF::PermissionStatus::INVALID;
                break;
            case SAF::PermissionStatus::RESTRICTED:
                safPermissionStatus = SAF::PermissionStatus::RESTRICTED;
                break;
            default:
                return SAF_ERR_ARG_INVALID;
        }
        return SAF_SUCCESS;
    }
}

napi_value NapiCreateError(const napi_env env, int32_t errCode, const char *errMsg)
{
    napi_value code = nullptr;
    NAPI_CALL(env, napi_create_int32(env, errCode, &code));

    napi_value message = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, errMsg, strlen(errMsg), &message));

    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_error(env, code, message, &result));
    return result;
}

napi_status NapiGetProperty(const napi_env env, napi_value object, bool &value)
{
    NAPI_RETURN_IF_VALUE_UNDEFINED(env, object);
    NAPI_CALL_RETURN_ERR(env, napi_get_value_bool(env, object, &value));
    return napi_ok;
}

napi_status NapiGetProperty(const napi_env env, napi_value object, int32_t &value)
{
    NAPI_RETURN_IF_VALUE_UNDEFINED(env, object);
    NAPI_CALL_RETURN_ERR(env, napi_get_value_int32(env, object, &value));
    return napi_ok;
}

napi_status NapiGetProperty(const napi_env env, napi_value object, std::string &value)
{
    NAPI_RETURN_IF_VALUE_UNDEFINED(env, object);
    NAPI_THROW_RETURN_ERR(env, type != napi_string, INVALID_PARAMETER, "Invalid type. Expect string");
    char buffer[MAX_BUFF_SIZE] = { 0 };
    size_t length = 0;
    NAPI_CALL_RETURN_ERR(env, napi_get_value_string_utf8(env, object, buffer, MAX_BUFF_SIZE, &length));
    value = buffer;
    return napi_ok;
}

napi_status NapiGetProperty(const napi_env env, napi_value object, SAF::PermissionQuery &permissionQuery)
{
    NAPI_RETURN_IF_VALUE_UNDEFINED(env, object);
    napi_value propValue;
    if (napi_get_named_property(env, object, "needTicket", &propValue) == napi_ok) {
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, permissionQuery.needTicket));
    }
    if (napi_get_named_property(env, object, "ticketExpireTimeMs", &propValue) == napi_ok) {
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, permissionQuery.ticketExpireTimeMs));
    }
    if (napi_get_named_property(env, object, "callerTokenId", &propValue) == napi_ok) {
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, permissionQuery.callerTokenId));
    }
    if (napi_get_named_property(env, object, "domainId", &propValue) == napi_ok) {
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, permissionQuery.domainId));
    }
    if (napi_get_named_property(env, object, "operationInfo", &propValue) == napi_ok) {
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, permissionQuery.operationInfo));
    }
    if (napi_get_named_property(env, object, "remoteInfo", &propValue) == napi_ok) {
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, permissionQuery.remoteInfo));
    }
    return napi_ok;
}

napi_status NapiGetProperty(const napi_env env, napi_value object, SAF::RemoteControlParams &remoteControlParams)
{
    NAPI_RETURN_IF_VALUE_UNDEFINED(env, object);
    napi_value propValue;
    if (napi_get_named_property(env, object, "challenge", &propValue) == napi_ok) {
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, remoteControlParams.challenge));
    }
    if (napi_get_named_property(env, object, "remoteControlTicket", &propValue) == napi_ok) {
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, remoteControlParams.remoteControlTicket));
    }
    if (napi_get_named_property(env, object, "controlledDeviceName", &propValue) == napi_ok) {
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, remoteControlParams.controlledDeviceName));
    }
    if (napi_get_named_property(env, object, "controllerDeviceName", &propValue) == napi_ok) {
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, remoteControlParams.controllerDeviceName));
    }
    if (napi_get_named_property(env, object, "signVerifyMsg", &propValue) == napi_ok) {
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, remoteControlParams.signVerifyMsg));
    }
    return napi_ok;
}

napi_status NapiGetProperty(const napi_env env, napi_value object, SAF::RemoteInfo &remoteInfo)
{
    NAPI_RETURN_IF_VALUE_UNDEFINED(env, object);
    napi_value propValue;
    int32_t role = 0;
    if (napi_get_named_property(env, object, "role", &propValue) != napi_ok) {
        return napi_ok;
    }
    NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, role));
    if (role < static_cast<int32_t>(SAF::Role::CONTROLLER) || role > static_cast<int32_t>(SAF::Role::CONTROLLED)) {
        NAPI_THROW_RETURN_ERR(env, true, GENERAL_PARAMETER_ERROR, "Invalid role");
    }
    remoteInfo.role = static_cast<SAF::Role>(role);
    if (napi_get_named_property(env, object, "remoteId", &propValue) == napi_ok) {
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, remoteInfo.remoteId));
    }
    if (napi_get_named_property(env, object, "domainId", &propValue) == napi_ok) {
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, remoteInfo.domainId));
    }
    if (napi_get_named_property(env, object, "remoteControlParams", &propValue) == napi_ok) {
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, remoteInfo.remoteControlParams));
    }
    return napi_ok;
}

napi_status NapiGetProperty(const napi_env env, napi_value object, std::vector<SAF::OperationInfo> &operationInfoVector)
{
    NAPI_RETURN_IF_VALUE_UNDEFINED(env, object);
    uint32_t count = 0;
    NAPI_CALL_RETURN_ERR(env, napi_get_array_length(env, object, &count));
    NAPI_THROW_RETURN_ERR(env, count == 0, GENERAL_PARAMETER_ERROR, "OperationInfo cannot be empty");
    for (uint32_t i = 0; i < count; ++i) {
        napi_value item = nullptr;
        NAPI_CALL_RETURN_ERR(env, napi_get_element(env, object, i, &item));
        SAF::OperationInfo operationInfo;
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, item, operationInfo));
        operationInfoVector.emplace_back(operationInfo);
    }
    return napi_ok;
}

napi_status NapiGetProperty(const napi_env env, napi_value object, SAF::OperationInfo &operationInfo)
{
    NAPI_RETURN_IF_VALUE_UNDEFINED(env, object);
    napi_value propValue;
    int32_t operationType = 0;

    if (napi_get_named_property(env, object, "operationType", &propValue) == napi_ok) {
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, operationType));
    }
    if (operationType == static_cast<int32_t>(SAF::OperationType::CLI)) {
        operationInfo.operationType = SAF::OperationType::CLI;
        if (napi_get_named_property(env, object, "info", &propValue) == napi_ok) {
            NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, operationInfo.cliCmdInfo));
        }
    } else if (operationType == static_cast<int32_t>(SAF::OperationType::API)) {
        operationInfo.operationType = SAF::OperationType::API;
        if (napi_get_named_property(env, object, "info", &propValue) == napi_ok) {
            NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, operationInfo.permission));
        }
    } else {
        NAPI_THROW_RETURN_ERR(env, true, GENERAL_PARAMETER_ERROR, "Invalid operationType");
    }
    return napi_ok;
}

napi_status NapiGetProperty(const napi_env env, napi_value object, SAF::CommandInfo &commandInfo)
{
    NAPI_RETURN_IF_VALUE_UNDEFINED(env, object);
    napi_value propValue = nullptr;
    if (napi_get_named_property(env, object, "cliCmdName", &propValue) == napi_ok) {
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, commandInfo.cmdName));
    }
    if (napi_get_named_property(env, object, "subCliCmdName", &propValue) == napi_ok) {
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, commandInfo.subCmd));
    }
    return napi_ok;
}

napi_status NapiGetProperty(const napi_env env, napi_value object,
    std::vector<SAF::UserAuthResult> &userAuthResultVector)
{
    NAPI_RETURN_IF_VALUE_UNDEFINED(env, object);
    uint32_t count = 0;
    NAPI_CALL_RETURN_ERR(env, napi_get_array_length(env, object, &count));
    NAPI_THROW_RETURN_ERR(env, count == 0, GENERAL_PARAMETER_ERROR, "UserAuthResult cannot be empty");
    for (uint32_t i = 0; i < count; ++i) {
        napi_value item = nullptr;
        NAPI_CALL_RETURN_ERR(env, napi_get_element(env, object, i, &item));
        SAF::UserAuthResult userAuthResult;
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, item, userAuthResult));
        userAuthResultVector.emplace_back(userAuthResult);
    }
    return napi_ok;
}

napi_status NapiGetProperty(const napi_env env, napi_value object, SAF::UserAuthResult &userAuthResult)
{
    NAPI_RETURN_IF_VALUE_UNDEFINED(env, object);
    napi_value propValue = nullptr;
    if (napi_get_named_property(env, object, "permissionQuery", &propValue) == napi_ok) {
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, userAuthResult.permissionQuery));
    }
    if (napi_get_named_property(env, object, "permissionInfo", &propValue) == napi_ok) {
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, userAuthResult.permissionInfo));
    }
    return napi_ok;
}

napi_status NapiGetProperty(const napi_env env, napi_value object,
    std::vector<SAF::PermissionInfo> &permissionInfoVector)
{
    NAPI_RETURN_IF_VALUE_UNDEFINED(env, object);
    uint32_t count = 0;
    NAPI_CALL_RETURN_ERR(env, napi_get_array_length(env, object, &count));
    NAPI_THROW_RETURN_ERR(env, count == 0, GENERAL_PARAMETER_ERROR, "PermissionInfo cannot be empty");
    for (uint32_t i = 0; i < count; ++i) {
        napi_value item = nullptr;
        NAPI_CALL_RETURN_ERR(env, napi_get_element(env, object, i, &item));
        SAF::PermissionInfo permissionInfo;
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, item, permissionInfo));
        permissionInfoVector.emplace_back(permissionInfo);
    }
    return napi_ok;
}

napi_status NapiGetProperty(const napi_env env, napi_value object, SAF::PermissionInfo &permissionInfo)
{
    NAPI_RETURN_IF_VALUE_UNDEFINED(env, object);
    napi_value propValue = nullptr;
    if (napi_get_named_property(env, object, "permission", &propValue) == napi_ok) {
        std::string permission = "";
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, permission));
        if (permission.size() > MAX_PERMISSION_NAME_SIZE) {
            NAPI_THROW_RETURN_ERR(env, true, INVALID_PARAMETER, "Permission name exceeds 256");
        }
        permissionInfo.permission = permission;
    }
    int32_t permissionStatus = 0;
    if (napi_get_named_property(env, object, "permissionStatus", &propValue) == napi_ok) {
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, permissionStatus));
        NAPI_THROW_RETURN_ERR(env,
            GetPermissionStatus(permissionStatus, permissionInfo.permissionStatus) != SAF_SUCCESS,
            INVALID_PARAMETER, "PermissionStatus is invalid");
    }
    return napi_ok;
}

napi_status NapiSetProperty(const napi_env env, napi_value object, const char *propertyName, const bool value)
{
    napi_value jsResult = nullptr;
    NAPI_CALL_RETURN_ERR(env, napi_get_boolean(env, value, &jsResult));
    NAPI_CALL_RETURN_ERR(env, napi_set_named_property(env, object, propertyName, jsResult));
    return napi_ok;
}

napi_status NapiSetProperty(const napi_env env, napi_value object, const char *propertyName, const int32_t value)
{
    napi_value jsResult = nullptr;
    NAPI_CALL_RETURN_ERR(env, napi_create_int32(env, value, &jsResult));
    NAPI_CALL_RETURN_ERR(env, napi_set_named_property(env, object, propertyName, jsResult));
    return napi_ok;
}

napi_status NapiSetProperty(const napi_env env, napi_value object, const char *propertyName, const std::string value)
{
    napi_value jsResult = nullptr;
    NAPI_CALL_RETURN_ERR(env, napi_create_string_utf8(env, value.c_str(), NAPI_AUTO_LENGTH, &jsResult));
    NAPI_CALL_RETURN_ERR(env, napi_set_named_property(env, object, propertyName, jsResult));
    return napi_ok;
}

napi_status NapiSetProperty(const napi_env env, napi_value object, const char *propertyName,
    SAF::VerifyTicketInfo ticket)
{
    napi_value jsResult = nullptr;
    NAPI_CALL_RETURN_ERR(env, napi_create_object(env, &jsResult));
    NAPI_CALL_RETURN_ERR(env, NapiSetProperty(env, jsResult, "message", ticket.message));
    NAPI_CALL_RETURN_ERR(env, NapiSetProperty(env, jsResult, "challenge", ticket.message));
    NAPI_CALL_RETURN_ERR(env, NapiSetProperty(env, jsResult, "ticket", ticket.ticket));
    NAPI_CALL_RETURN_ERR(env, napi_set_named_property(env, object, propertyName, jsResult));
    return napi_ok;
}

napi_status NapiSetPropertyUndefined(const napi_env env, napi_value object, const char *propertyName)
{
    napi_value jsResult = nullptr;
    NAPI_CALL_RETURN_ERR(env, napi_get_undefined(env, &jsResult));
    NAPI_CALL_RETURN_ERR(env, napi_set_named_property(env, object, propertyName, jsResult));
    return napi_ok;
}

napi_status NapiSetProperty(const napi_env env, napi_value object, const char *propertyName,
    const SAF::AuthStatusInfo authStatusInfo)
{
    napi_value jsResult = nullptr;
    NAPI_CALL_RETURN_ERR(env, napi_create_object(env, &jsResult));
    NAPI_CALL_RETURN_ERR(env, NapiSetProperty(env, jsResult, "authStatus",
        static_cast<int32_t>(authStatusInfo.authStatus)));
    NAPI_CALL_RETURN_ERR(env, NapiSetProperty(env, jsResult, "flag",
        static_cast<int32_t>(authStatusInfo.flag)));
    NAPI_CALL_RETURN_ERR(env, napi_set_named_property(env, object, propertyName, jsResult));
    return napi_ok;
}

napi_status NapiSetProperty(const napi_env env, napi_value object, const char *propertyName,
    const std::vector<SAF::PermissionInfo> &permissionResults)
{
    napi_value jsResult = nullptr;
    NAPI_CALL_RETURN_ERR(env, napi_create_array(env, &jsResult));
    for (uint32_t i = 0; i < permissionResults.size(); ++i) {
        napi_value jsResultItem = nullptr;
        NAPI_CALL_RETURN_ERR(env, napi_create_object(env, &jsResultItem));
        NAPI_CALL_RETURN_ERR(env, NapiSetProperty(env, jsResultItem, "permission",
            permissionResults[i].permission));
        NAPI_CALL_RETURN_ERR(env, NapiSetProperty(env, jsResultItem, "permissionStatus",
            static_cast<int32_t>(permissionResults[i].permissionStatus)));
        NAPI_CALL_RETURN_ERR(env, NapiSetProperty(env, jsResultItem, "authStatusInfo",
            permissionResults[i].authStatusInfo));
        NAPI_CALL_RETURN_ERR(env, napi_set_element(env, jsResult, i, jsResultItem));
    }
    NAPI_CALL_RETURN_ERR(env, napi_set_named_property(env, object, propertyName, jsResult));
    return napi_ok;
}

napi_value CreateAsyncWork(napi_env env, napi_callback_info info, std::unique_ptr<SAF::AgentFenceAsyncContext> context,
    const char *resourceName)
{
    if (context->parse != nullptr) {
        NAPI_CALL(env, context->parse(env, info, context.get()));
    }
    napi_value promise;
    NAPI_CALL(env, napi_create_promise(env, &context->deferred, &promise));
    napi_value resource = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, resourceName, NAPI_AUTO_LENGTH, &resource));
    NAPI_CALL(env, napi_create_async_work(
        env, nullptr, resource, context->execute,
        [](napi_env env, napi_status status, void* data) {
            SAF::AgentFenceAsyncContext *asyncContext = static_cast<SAF::AgentFenceAsyncContext *>(data);
            ResolvePromise(env, asyncContext);
            delete asyncContext;
        },
        static_cast<void *>(context.get()), &context->work));
    context->env = env;
    NAPI_CALL(env, napi_queue_async_work(env, context->work));
    context.release();
    return promise;
}

void AddUint32Property(const napi_env env, napi_value object, const char *name, uint32_t value)
{
    napi_value property = nullptr;
    NAPI_CALL_RETURN_VOID(env, napi_create_uint32(env, value, &property));
    NAPI_CALL_RETURN_VOID(env, napi_set_named_property(env, object, name, property));
}
} // SAF_ASSET_COMMON
} // Security
} // OHOS
