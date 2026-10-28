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

#include <vector>
#include "napi_common.h"
#include "secure_access_fence_type.h"
#include "saf_log.h"
#include "saf_result_code.h"
#include "agent_fence_error_codes.h"

namespace OHOS {
namespace Security {
namespace SAF_ASSET_COMMON {
namespace {
    constexpr uint32_t MAX_BUFF_SIZE = 1024 * 25;   // 25KB
    constexpr uint32_t MAX_PERMISSION_NAME_SIZE = 256;
    constexpr uint32_t MAX_REMOTE_QUERY_ARRAY_SIZE = 10;
    constexpr uint32_t MAX_CLI_CMD_INFOS_COUNT = 300;
    constexpr uint32_t MAX_CMD_NAME_LENGTH = 512;
    constexpr int32_t MAX_TICKET_EXPIRE_TIME_MS = 60000;

    // IMPORTANT: Keep in sync with PATH_TRAVERSAL_PATTERNS in
    //   services/secure_access_fence/core_service/src/task_identity/constants.rs
    const std::vector<std::string> PATH_TRAVERSAL_PATTERNS = {
        "../", "..//", "./", ".\\", "//", "\\", "/", "/..", "\\..", ".."
    };

    AgentFenceErrorCode MapErrorCode(const int32_t safResult)
    {
        switch (safResult) {
            case SAF_ERR_PERMISSION_DENIED:
                return PERMISSION_DENAIL;
            case SAF_ERR_NOT_SYSTEM_APP:
                return NOT_SYSTEM_APP;
            case SAF_ERR_ARG_INVALID:
            case SAF_ERR_ARG_EMPTY:
            case SAF_ERR_INVALID_ARRAY_LEN:
            case SAF_ERR_NULL_PTR:
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
            case SAF_ERR_TRUSTED_RING_TIMEOUT:
            case SAF_ERR_TRUSTED_RING_NETWORK_DISCONNECTED:
            case SAF_ERR_TRUSTED_RING_NOT_LOGGED_IN:
                return ENVIRONMENT_ERROR;
            case SAF_ERR_QUERIED_PERMISSION_NOT_EXIST:
                return INVALID_PERMISSION;
            case SAF_ERR_SERVICE_UNAVAILABLE:
            case SAF_ERR_SERVICE_IS_STOPPING:
            case SAF_ERR_IPC_WRITE_DATA_FAIL:
            case SAF_ERR_IPC_READ_DATA_FAIL:
            case SAF_ERR_IPC_SEND_REQUEST_FAIL:
            case SAF_ERR_IPC_PROXY_FAIL:
            case SAF_ERR_IPC_ERROR:
            case SAF_ERR_IPC_INVALID_IPC_CODE:
                return SERVICE_ABNORMAL;
            case SAF_ERR_GET_BUNDLE_NAME_BY_TOKEN_FAILED:
            case SAF_ERR_DLOPEN_FAIL:
            case SAF_ERR_DLSYM_FAIL:
            case SAF_ERR_DATA_TYPE_MISMATCH:
            case SAF_ERR_MAP_KEY_NOT_FOUND:
            case SAF_ERR_CREATE_PLUGIN_MGR_FAIL:
            case SAF_ERR_PLUGIN_INVALID_EVENT_TYPE:
            case SAF_ERR_PLUGIN_NOT_SUPPORT:
            case SAF_ERR_INVALID_PLUGIN:
            case SAF_ERR_INVALID_OS_ACCOUNT_ID:
                return COMMON_INTERNAL_ERROR;
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
} // namespace

bool ContainsPathTraversalChars(const std::string &str)
{
    for (const auto &pattern : PATH_TRAVERSAL_PATTERNS) {
        if (str.find(pattern) != std::string::npos) {
            return true;
        }
    }
    return false;
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

napi_status NapiGetProperty(const napi_env env, napi_value object, int64_t &value)
{
    NAPI_RETURN_IF_VALUE_UNDEFINED(env, object);
    NAPI_CALL_RETURN_ERR(env, napi_get_value_int64(env, object, &value));
    return napi_ok;
}

napi_status NapiGetProperty(const napi_env env, napi_value object, std::string &value)
{
    NAPI_RETURN_IF_VALUE_UNDEFINED(env, object);
    NAPI_THROW_RETURN_ERR(env, type != napi_string, INVALID_PARAMETER, "Invalid type. Expect string");
    size_t length = 0;
    NAPI_CALL_RETURN_ERR(env, napi_get_value_string_utf8(env, object, nullptr, 0, &length));
    NAPI_THROW_RETURN_ERR(env, length >= MAX_BUFF_SIZE, INVALID_PARAMETER,
        "String length exceeds MAX_BUFF_SIZE");
    std::vector<char> buffer(length + 1, 0);
    NAPI_CALL_RETURN_ERR(env, napi_get_value_string_utf8(env, object, buffer.data(), length + 1, &length));
    value = buffer.data();
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
    if (napi_get_named_property(env, object, "taskIdentity", &propValue) == napi_ok) {
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, permissionQuery.taskIdentity));
    }
    return napi_ok;
}

napi_status NapiGetProperty(const napi_env env, napi_value object, SAF::TaskIdentity &taskIdentity)
{
    NAPI_RETURN_IF_VALUE_UNDEFINED(env, object);
    napi_value propValue;
    if (napi_get_named_property(env, object, TaskIdentityKeys::TASK_ATTRIBUTES, &propValue) == napi_ok) {
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, taskIdentity.taskAttributes));
    }
    if (napi_get_named_property(env, object, TaskIdentityKeys::IDENTITY, &propValue) == napi_ok) {
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, taskIdentity.identity));
    }
    if (napi_get_named_property(env, object, TaskIdentityKeys::CHALLENGE, &propValue) == napi_ok) {
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, taskIdentity.challenge));
    }
    return napi_ok;
}

napi_status NapiGetProperty(const napi_env env, napi_value object, SAF::TaskIdentityInfo &taskIdentityInfo)
{
    NAPI_RETURN_IF_VALUE_UNDEFINED(env, object);
    napi_value propValue;
    if (napi_get_named_property(env, object, TaskIdentityKeys::TASK_ID, &propValue) == napi_ok) {
        NAPI_CHECK_REQUIRED(env, propValue, "taskId");
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, taskIdentityInfo.taskId));
    }
    if (napi_get_named_property(env, object, TaskIdentityKeys::TASK_EXPIRE_TIME, &propValue) == napi_ok) {
        NAPI_CHECK_REQUIRED(env, propValue, "taskExpireTime");
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, taskIdentityInfo.taskExpireTime));
    }
    if (napi_get_named_property(env, object, TaskIdentityKeys::EXECUTE_TYPE, &propValue) == napi_ok) {
        NAPI_CHECK_REQUIRED(env, propValue, "executeType");
        int32_t executeType = 0;
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, executeType));
        if (executeType < static_cast<int32_t>(SAF::ExecuteType::INSTANT) ||
            executeType > static_cast<int32_t>(SAF::ExecuteType::DELAYED)) {
            NAPI_THROW_RETURN_ERR(env, true, INVALID_PARAMETER, "Invalid executeType");
        }
        taskIdentityInfo.executeType = static_cast<SAF::ExecuteType>(executeType);
    }
    if (napi_get_named_property(env, object, TaskIdentityKeys::ALLOW_SCREEN_LOCK_EXECUTION, &propValue) == napi_ok) {
        NAPI_CHECK_REQUIRED(env, propValue, "allowScreenLockExecution");
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, taskIdentityInfo.allowScreenLockExecution));
    }
    if (napi_get_named_property(env, object, TaskIdentityKeys::ALLOWABLE_DURATION, &propValue) == napi_ok) {
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, taskIdentityInfo.allowableDuration));
        if (taskIdentityInfo.allowableDuration == 0) {
            taskIdentityInfo.allowableDuration = DEFAULT_ALLOWABLE_DURATION;
        } else if (taskIdentityInfo.allowableDuration < 0 ||
                   taskIdentityInfo.allowableDuration > MAX_ALLOWABLE_DURATION) {
            NAPI_THROW_RETURN_ERR(env, true, INVALID_PARAMETER, "allowableDuration must be in (0, 60]");
        }
    }
    if (napi_get_named_property(env, object, TaskIdentityKeys::AUTH_TOKEN, &propValue) == napi_ok) {
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, taskIdentityInfo.authToken));
    }
    return napi_ok;
}

napi_status ValidateTaskIdentityInfo(const napi_env env, const SAF::TaskIdentityInfo &taskIdentityInfo)
{
    if (taskIdentityInfo.taskId.empty()) {
        NAPI_THROW_RETURN_ERR(env, true, INVALID_PARAMETER, "taskId is required");
    }
    if (taskIdentityInfo.taskId.length() > MAX_TASK_ID_LEN) {
        NAPI_THROW_RETURN_ERR(env, true, INVALID_PARAMETER, "taskId exceeds max length");
    }
    if (ContainsPathTraversalChars(taskIdentityInfo.taskId)) {
        NAPI_THROW_RETURN_ERR(env, true, INVALID_PARAMETER, "taskId contains path traversal characters");
    }
    if (taskIdentityInfo.authToken.size() > MAX_AUTH_TOKEN_SIZE) {
        NAPI_THROW_RETURN_ERR(env, true, INVALID_PARAMETER, "authToken exceeds max size");
    }
    return napi_ok;
}

/// Parses a JS value as a byte array (Uint8Array or ArrayBuffer) into vector<int32_t>.
/// Each byte (0~255) is stored as one int32_t element, matching the IDL List<int> type
/// used for authToken transport. Values are always 0~255 (u8 range), validated in
/// ValidateTaskIdentityInfo and Rust validator.
napi_status NapiGetProperty(const napi_env env, napi_value object, std::vector<int32_t> &values)
{
    NAPI_RETURN_IF_VALUE_UNDEFINED(env, object);
    bool isTypedArray = false;
    NAPI_CALL_RETURN_ERR(env, napi_is_typedarray(env, object, &isTypedArray));
    if (isTypedArray) {
        napi_typedarray_type type;
        size_t length = 0;
        void *data = nullptr;
        napi_value arraybuffer = nullptr;
        size_t byteOffset = 0;
        NAPI_CALL_RETURN_ERR(env, napi_get_typedarray_info(env, object, &type, &length, &data,
            &arraybuffer, &byteOffset));
        if (type != napi_uint8_array) {
            NAPI_THROW_RETURN_ERR(env, true, INVALID_PARAMETER, "authToken must be Uint8Array");
        }
        if (data == nullptr) {
            NAPI_THROW_RETURN_ERR(env, true, INVALID_PARAMETER, "authToken data is null");
        }
        const uint8_t *bytes = static_cast<const uint8_t *>(data);
        for (size_t i = 0; i < length; ++i) {
            values.push_back(static_cast<int32_t>(bytes[i]));
        }
        return napi_ok;
    }
    bool isArrayBuffer = false;
    NAPI_CALL_RETURN_ERR(env, napi_is_arraybuffer(env, object, &isArrayBuffer));
    if (isArrayBuffer) {
        void *data = nullptr;
        size_t length = 0;
        NAPI_CALL_RETURN_ERR(env, napi_get_arraybuffer_info(env, object, &data, &length));
        if (data == nullptr) {
            NAPI_THROW_RETURN_ERR(env, true, INVALID_PARAMETER, "authToken data is null");
        }
        const uint8_t *bytes = static_cast<const uint8_t *>(data);
        for (size_t i = 0; i < length; ++i) {
            values.push_back(static_cast<int32_t>(bytes[i]));
        }
        return napi_ok;
    }
    return napi_ok;
}

napi_status NapiGetProperty(const napi_env env, napi_value object, std::vector<std::string> &values)
{
    NAPI_RETURN_IF_VALUE_UNDEFINED(env, object);
    uint32_t count = 0;
    NAPI_CALL_RETURN_ERR(env, napi_get_array_length(env, object, &count));
    values.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        napi_value item = nullptr;
        NAPI_CALL_RETURN_ERR(env, napi_get_element(env, object, i, &item));
        std::string value;
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, item, value));
        values.emplace_back(value);
    }
    return napi_ok;
}

napi_status NapiSetProperty(const napi_env env, napi_value object, const char *propertyName,
    const SAF::TaskIdentity &taskIdentity)
{
    napi_value jsResult = nullptr;
    NAPI_CALL_RETURN_ERR(env, napi_create_object(env, &jsResult));
    NAPI_CALL_RETURN_ERR(env,
        NapiSetProperty(env, jsResult, TaskIdentityKeys::TASK_ATTRIBUTES, taskIdentity.taskAttributes));
    NAPI_CALL_RETURN_ERR(env, NapiSetProperty(env, jsResult, TaskIdentityKeys::IDENTITY, taskIdentity.identity));
    NAPI_CALL_RETURN_ERR(env, NapiSetProperty(env, jsResult, TaskIdentityKeys::CHALLENGE, taskIdentity.challenge));
    NAPI_CALL_RETURN_ERR(env, napi_set_named_property(env, object, propertyName, jsResult));
    return napi_ok;
}

napi_status NapiSetProperty(const napi_env env, napi_value object, const char *propertyName,
    const std::vector<int32_t> &values)
{
    void *data = nullptr;
    napi_value arrayBuffer = nullptr;
    NAPI_CALL_RETURN_ERR(env, napi_create_arraybuffer(env, values.size(), &data, &arrayBuffer));
    uint8_t *bytes = static_cast<uint8_t *>(data);
    for (size_t i = 0; i < values.size(); ++i) {
        bytes[i] = static_cast<uint8_t>(values[i]);
    }
    napi_value jsResult = nullptr;
    NAPI_CALL_RETURN_ERR(env, napi_create_typedarray(env, napi_uint8_array, values.size(), arrayBuffer, 0, &jsResult));
    NAPI_CALL_RETURN_ERR(env, napi_set_named_property(env, object, propertyName, jsResult));
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
        NAPI_THROW_RETURN_ERR(env, commandInfo.cmdName.empty(), INVALID_PARAMETER,
            "cliCmdName cannot be empty");
        NAPI_THROW_RETURN_ERR(env, commandInfo.cmdName.length() > MAX_CMD_NAME_LENGTH,
            INVALID_PARAMETER, "cliCmdName length exceeds limit");
    }
    if (napi_get_named_property(env, object, "subCliCmdName", &propValue) == napi_ok) {
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, commandInfo.subCmd));
        NAPI_THROW_RETURN_ERR(env, commandInfo.subCmd.length() > MAX_CMD_NAME_LENGTH,
            INVALID_PARAMETER, "subCliCmdName length exceeds limit");
    }
    return napi_ok;
}

napi_status NapiGetProperty(const napi_env env, napi_value object,
    std::vector<SAF::CommandInfo> &commandInfoVector)
{
    NAPI_RETURN_IF_VALUE_UNDEFINED(env, object);
    uint32_t count = 0;
    NAPI_CALL_RETURN_ERR(env, napi_get_array_length(env, object, &count));
    NAPI_THROW_RETURN_ERR(env, count == 0, INVALID_PARAMETER, "CommandInfo cannot be empty");
    NAPI_THROW_RETURN_ERR(env, count > MAX_CLI_CMD_INFOS_COUNT, INVALID_PARAMETER,
        "CommandInfo count exceeds limit");
    for (uint32_t i = 0; i < count; ++i) {
        napi_value item = nullptr;
        NAPI_CALL_RETURN_ERR(env, napi_get_element(env, object, i, &item));
        SAF::CommandInfo commandInfo;
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, item, commandInfo));
        commandInfoVector.emplace_back(commandInfo);
    }
    return napi_ok;
}

napi_status NapiGetProperty(const napi_env env, napi_value object, SAF::TicketOptions &ticketOptions)
{
    NAPI_RETURN_IF_VALUE_UNDEFINED(env, object);
    napi_value propValue = nullptr;
    if (napi_get_named_property(env, object, "ticketExpireTimeMs", &propValue) == napi_ok) {
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, ticketOptions.ticketExpireTimeMs));
        NAPI_THROW_RETURN_ERR(env, ticketOptions.ticketExpireTimeMs > MAX_TICKET_EXPIRE_TIME_MS,
            INVALID_PARAMETER, "ticketExpireTimeMs exceeds limit");
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
    NAPI_CALL_RETURN_ERR(env, NapiSetProperty(env, jsResult, "challenge", ticket.challenge));
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

napi_status NapiGetProperty(const napi_env env, napi_value object,
    std::vector<SAF::PermissionQuery> &permissionQueryVector)
{
    NAPI_RETURN_IF_VALUE_UNDEFINED(env, object);
    uint32_t count = 0;
    NAPI_CALL_RETURN_ERR(env, napi_get_array_length(env, object, &count));
    NAPI_THROW_RETURN_ERR(env, count == 0, GENERAL_PARAMETER_ERROR, "PermissionQuery cannot be empty");
    NAPI_THROW_RETURN_ERR(env, count > MAX_REMOTE_QUERY_ARRAY_SIZE, GENERAL_PARAMETER_ERROR,
        "PermissionQuery count exceeds limit");
    for (uint32_t i = 0; i < count; ++i) {
        napi_value item = nullptr;
        NAPI_CALL_RETURN_ERR(env, napi_get_element(env, object, i, &item));
        SAF::PermissionQuery query;
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, item, query));
        permissionQueryVector.emplace_back(query);
    }
    return napi_ok;
}

napi_status NapiGetProperty(const napi_env env, napi_value object,
    std::vector<SAF::RemoteAuthPackage> &remoteAuthPackageVector)
{
    NAPI_RETURN_IF_VALUE_UNDEFINED(env, object);
    uint32_t count = 0;
    NAPI_CALL_RETURN_ERR(env, napi_get_array_length(env, object, &count));
    NAPI_THROW_RETURN_ERR(env, count == 0, GENERAL_PARAMETER_ERROR, "RemoteAuthPackage cannot be empty");
    for (uint32_t i = 0; i < count; ++i) {
        napi_value item = nullptr;
        NAPI_CALL_RETURN_ERR(env, napi_get_element(env, object, i, &item));
        SAF::RemoteAuthPackage pkg;
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, item, pkg));
        remoteAuthPackageVector.emplace_back(pkg);
    }
    return napi_ok;
}

napi_status NapiGetProperty(const napi_env env, napi_value object,
    SAF::RemoteAuthPackage &remoteAuthPackage)
{
    NAPI_RETURN_IF_VALUE_UNDEFINED(env, object);
    napi_value propValue;
    
    napi_status status = napi_get_named_property(env, object, "remoteMessage", &propValue);
    NAPI_THROW_RETURN_ERR(env, status != napi_ok, INVALID_PARAMETER, "remoteMessage is required");
    NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, remoteAuthPackage.remoteMessage));
    
    status = napi_get_named_property(env, object, "challenge", &propValue);
    NAPI_THROW_RETURN_ERR(env, status != napi_ok, INVALID_PARAMETER, "challenge is required");
    NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, remoteAuthPackage.challenge));
    
    status = napi_get_named_property(env, object, "ticket", &propValue);
    NAPI_THROW_RETURN_ERR(env, status != napi_ok, INVALID_PARAMETER, "ticket is required");
    NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, remoteAuthPackage.ticket));
    
    return napi_ok;
}

napi_status NapiGetProperty(const napi_env env, napi_value object,
    SAF::RemoteUserAuthItem &remoteUserAuthItem)
{
    NAPI_RETURN_IF_VALUE_UNDEFINED(env, object);
    napi_value propValue;
    
    napi_status status = napi_get_named_property(env, object, "permission", &propValue);
    NAPI_THROW_RETURN_ERR(env, status != napi_ok, INVALID_PARAMETER, "permission is required");
    NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, remoteUserAuthItem.permission));
    
    status = napi_get_named_property(env, object, "authResult", &propValue);
    NAPI_THROW_RETURN_ERR(env, status != napi_ok, INVALID_PARAMETER, "authResult is required");
    NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, remoteUserAuthItem.authResult));
    
    return napi_ok;
}

napi_status NapiGetProperty(const napi_env env, napi_value object,
    std::vector<SAF::RemoteUserAuthItem> &remoteUserAuthItemVector)
{
    NAPI_RETURN_IF_VALUE_UNDEFINED(env, object);
    uint32_t count = 0;
    NAPI_CALL_RETURN_ERR(env, napi_get_array_length(env, object, &count));
    NAPI_THROW_RETURN_ERR(env, count == 0, GENERAL_PARAMETER_ERROR, "RemoteUserAuthItem cannot be empty");
    for (uint32_t i = 0; i < count; ++i) {
        napi_value item = nullptr;
        NAPI_CALL_RETURN_ERR(env, napi_get_element(env, object, i, &item));
        SAF::RemoteUserAuthItem remoteUserAuthItem;
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, item, remoteUserAuthItem));
        remoteUserAuthItemVector.emplace_back(remoteUserAuthItem);
    }
    return napi_ok;
}

napi_status NapiGetProperty(const napi_env env, napi_value object,
    SAF::RemoteUserAuthResults &remoteUserAuthResults)
{
    NAPI_RETURN_IF_VALUE_UNDEFINED(env, object);
    napi_value propValue;
    
    napi_status status = napi_get_named_property(env, object, "results", &propValue);
    NAPI_THROW_RETURN_ERR(env, status != napi_ok, INVALID_PARAMETER, "results is required");
    NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, remoteUserAuthResults.results));

    status = napi_get_named_property(env, object, "permissionQuery", &propValue);
    NAPI_THROW_RETURN_ERR(env, status != napi_ok, INVALID_PARAMETER, "permissionQuery is required");
    NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, remoteUserAuthResults.permissionQuery));
    
    return napi_ok;
}

napi_status NapiGetProperty(const napi_env env, napi_value object,
    std::vector<SAF::RemoteUserAuthResults> &remoteUserAuthResultsVector)
{
    NAPI_RETURN_IF_VALUE_UNDEFINED(env, object);
    uint32_t count = 0;
    NAPI_CALL_RETURN_ERR(env, napi_get_array_length(env, object, &count));
    NAPI_THROW_RETURN_ERR(env, count == 0, GENERAL_PARAMETER_ERROR, "RemoteUserAuthResults cannot be empty");
    NAPI_THROW_RETURN_ERR(env, count > MAX_REMOTE_QUERY_ARRAY_SIZE, GENERAL_PARAMETER_ERROR,
        "RemoteUserAuthResults count exceeds limit 10");
    for (uint32_t i = 0; i < count; ++i) {
        napi_value item = nullptr;
        NAPI_CALL_RETURN_ERR(env, napi_get_element(env, object, i, &item));
        SAF::RemoteUserAuthResults remoteUserAuthResults;
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, item, remoteUserAuthResults));
        remoteUserAuthResultsVector.emplace_back(remoteUserAuthResults);
    }
    return napi_ok;
}

napi_status NapiSetProperty(const napi_env env, napi_value object, const char *propertyName,
    const SAF::RemoteAuthPackage &remoteAuthPackage)
{
    napi_value jsResult = nullptr;
    NAPI_CALL_RETURN_ERR(env, napi_create_object(env, &jsResult));
    NAPI_CALL_RETURN_ERR(env, NapiSetProperty(env, jsResult, "remoteMessage", remoteAuthPackage.remoteMessage));
    NAPI_CALL_RETURN_ERR(env, NapiSetProperty(env, jsResult, "challenge", remoteAuthPackage.challenge));
    NAPI_CALL_RETURN_ERR(env, NapiSetProperty(env, jsResult, "ticket", remoteAuthPackage.ticket));
    NAPI_CALL_RETURN_ERR(env, napi_set_named_property(env, object, propertyName, jsResult));
    return napi_ok;
}

napi_value CreateAsyncWork(napi_env env, napi_callback_info info, std::unique_ptr<SAF::AgentFenceAsyncContext> context,
    const char *resourceName)
{
    napi_value promise;
    NAPI_CALL(env, napi_create_promise(env, &context->deferred, &promise));
    if (context->parse != nullptr) {
        napi_status status = context->parse(env, info, context.get());
        if (status != napi_ok) {
            napi_value exception = nullptr;
            napi_get_and_clear_last_exception(env, &exception);
            napi_value error = (exception != nullptr) ? exception :
                NapiCreateError(env, context->result, GetErrorMessage(INVALID_PARAMETER).c_str());
            NAPI_CALL(env, napi_reject_deferred(env, context->deferred, error));
            return promise;
        }
    }
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

napi_status NapiGetProperty(const napi_env env, napi_value object, SAF::AppInfo &appInfo)
{
    NAPI_RETURN_IF_VALUE_UNDEFINED(env, object);
    napi_value propValue = nullptr;
    napi_status status = napi_get_named_property(env, object, "bundleName", &propValue);
    NAPI_THROW_RETURN_ERR(env, status != napi_ok, GENERAL_PARAMETER_ERROR, "bundleName is required");
    NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, appInfo.bundleName));
    status = napi_get_named_property(env, object, "index", &propValue);
    NAPI_THROW_RETURN_ERR(env, status != napi_ok, GENERAL_PARAMETER_ERROR, "index is required");
    NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, propValue, appInfo.index));
    return napi_ok;
}

napi_status NapiGetProperty(const napi_env env, napi_value object, std::vector<SAF::AppInfo> &appInfoList)
{
    NAPI_RETURN_IF_VALUE_UNDEFINED(env, object);
    uint32_t count = 0;
    NAPI_CALL_RETURN_ERR(env, napi_get_array_length(env, object, &count));
    NAPI_THROW_RETURN_ERR(env, count == 0, GENERAL_PARAMETER_ERROR, "AppInfo cannot be empty");
    for (uint32_t i = 0; i < count; ++i) {
        napi_value item = nullptr;
        NAPI_CALL_RETURN_ERR(env, napi_get_element(env, object, i, &item));
        SAF::AppInfo appInfo;
        NAPI_CALL_RETURN_ERR(env, NapiGetProperty(env, item, appInfo));
        appInfoList.emplace_back(appInfo);
    }
    return napi_ok;
}

napi_status NapiSetProperty(const napi_env env, napi_value object, const char *propertyName,
    const SAF::AppInfo &appInfo)
{
    napi_value jsResult = nullptr;
    NAPI_CALL_RETURN_ERR(env, napi_create_object(env, &jsResult));
    NAPI_CALL_RETURN_ERR(env, NapiSetProperty(env, jsResult, "bundleName", appInfo.bundleName));
    NAPI_CALL_RETURN_ERR(env, NapiSetProperty(env, jsResult, "index", appInfo.index));
    NAPI_CALL_RETURN_ERR(env, napi_set_named_property(env, object, propertyName, jsResult));
    return napi_ok;
}

napi_status NapiSetProperty(const napi_env env, napi_value object, const char *propertyName,
    const SAF::ControlAppStatusResult &result)
{
    napi_value jsResult = nullptr;
    NAPI_CALL_RETURN_ERR(env, napi_create_object(env, &jsResult));
    NAPI_CALL_RETURN_ERR(env, NapiSetProperty(env, jsResult, "appInfo", result.appInfo));
    NAPI_CALL_RETURN_ERR(env, NapiSetProperty(env, jsResult, "status",
        static_cast<int32_t>(result.status)));
    NAPI_CALL_RETURN_ERR(env, napi_set_named_property(env, object, propertyName, jsResult));
    return napi_ok;
}

napi_status NapiSetProperty(const napi_env env, napi_value object, const char *propertyName,
    const std::vector<SAF::ControlAppStatusResult> &resultList)
{
    napi_value jsResult = nullptr;
    NAPI_CALL_RETURN_ERR(env, napi_create_array(env, &jsResult));
    for (uint32_t i = 0; i < resultList.size(); ++i) {
        napi_value jsResultItem = nullptr;
        NAPI_CALL_RETURN_ERR(env, napi_create_object(env, &jsResultItem));
        NAPI_CALL_RETURN_ERR(env, NapiSetProperty(env, jsResultItem, "appInfo", resultList[i].appInfo));
        NAPI_CALL_RETURN_ERR(env, NapiSetProperty(env, jsResultItem, "status",
            static_cast<int32_t>(resultList[i].status)));
        NAPI_CALL_RETURN_ERR(env, napi_set_element(env, jsResult, i, jsResultItem));
    }
    NAPI_CALL_RETURN_ERR(env, napi_set_named_property(env, object, propertyName, jsResult));
    return napi_ok;
}

napi_value MapErrCodeToBusinessError(const napi_env env, int32_t resultCode)
{
    AgentFenceErrorCode errCode = MapErrorCode(resultCode);
    return NapiCreateError(env, static_cast<int32_t>(errCode), GetErrorMessage(errCode).c_str());
}
} // SAF_ASSET_COMMON
} // Security
} // OHOS
