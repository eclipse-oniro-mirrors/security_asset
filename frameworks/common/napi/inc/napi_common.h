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

#ifndef SAF_ASSET_NAPI_COMMON_H
#define SAF_ASSET_NAPI_COMMON_H

#include <vector>

#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "secure_access_fence_type.h"
#include "agent_fence_napi_context.h"

namespace OHOS {
namespace Security {
namespace SAF_ASSET_COMMON {

#define NORMAL_ARGS_NUM 1
#define AS_USER_ARGS_NUM 2

#define MAX_MESSAGE_LEN 256
#define MAX_ARGS_NUM 5

#define NAPI_THROW_BASE(env, condition, ret, code, message)             \
if ((condition)) {                                                      \
    LOGE("[FATAL][NAPI]%{public}s", (message));                         \
    napi_throw((env), NapiCreateError((env), (code), (message)));         \
    return (ret);                                                       \
}
 
#define NAPI_THROW(env, condition, code, message)                       \
    NAPI_THROW_BASE(env, condition, nullptr, code, message)

#define IF_ERROR_THROW_RETURN(env, result)                 \
if ((result) != nullptr) {                                 \
    napi_throw((env), (result));                           \
    return napi_invalid_arg;                               \
}

#define IF_FALSE_RETURN(result, returnValue)    \
if (!(result)) {                                \
    return (returnValue);                        \
}

#define IF_ERR_RETURN(result)                   \
if ((result) != napi_ok) {                      \
    return (result);                            \
}

#define NAPI_CALL_RETURN_ERR(env, ret) \
if ((ret) != napi_ok) {                      \
    GET_AND_THROW_LAST_ERROR((env));           \
    return (ret);                            \
}

#define NAPI_THROW_RETURN_ERR(env, condition, code, message)            \
    NAPI_THROW_BASE(env, condition, napi_generic_failure, code, message)


#define NAPI_RETURN_IF_VALUE_UNDEFINED(env, object) \
    napi_valuetype type; \
    NAPI_CALL_RETURN_ERR(env, napi_typeof(env, object, &type)); \
    if (type == napi_undefined || type == napi_null) { \
        return napi_ok; \
    }

napi_value NapiCreateError(const napi_env env, int32_t code, const char *message);

napi_status NapiGetProperty(const napi_env env, napi_value object, SAF::PermissionQuery &permissionQuery);

napi_status NapiGetProperty(const napi_env env, napi_value object, bool &value);

napi_status NapiGetProperty(const napi_env env, napi_value object, int32_t &value);

napi_status NapiGetProperty(const napi_env env, napi_value object, std::string &value);

napi_status NapiGetProperty(const napi_env env, napi_value object, std::vector<SAF::OperationInfo> &operationInfo);

napi_status NapiGetProperty(const napi_env env, napi_value object, SAF::OperationInfo &operationInfo);

napi_status NapiGetProperty(const napi_env env, napi_value object, SAF::CommandInfo &commandInfo);

napi_status NapiGetProperty(const napi_env env, napi_value object, std::vector<SAF::UserAuthResult> &userAuthResult);

napi_status NapiGetProperty(const napi_env env, napi_value object, SAF::UserAuthResult &userAuthResult);

napi_status NapiGetProperty(const napi_env env, napi_value object, std::vector<SAF::PermissionInfo> &permissionInfo);

napi_status NapiGetProperty(const napi_env env, napi_value object, SAF::PermissionInfo &permissionInfo);

napi_status NapiSetProperty(const napi_env env, napi_value object, const char *propertyName, const bool value);

napi_status NapiSetProperty(const napi_env env, napi_value object, const char *propertyName, const int32_t value);

napi_status NapiSetProperty(const napi_env env, napi_value object, const char *propertyName,
    const SAF::VerifyTicketInfo ticket);

napi_status NapiSetProperty(const napi_env env, napi_value object, const char *propertyName, const std::string value);

napi_status NapiSetPropertyUndefined(const napi_env env, napi_value object, const char *propertyName);

napi_status NapiSetProperty(const napi_env env, napi_value object, const char *propertyName,
    const std::vector<SAF::PermissionInfo> &permissionResults);

napi_status NapiSetProperty(const napi_env env, napi_value object, const char *propertyName,
    const SAF::PermissionInfo permissionInfo);

napi_value CreateAsyncWork(napi_env env, napi_callback_info info, std::unique_ptr<SAF::AgentFenceAsyncContext> context,
    const char *resourceName);

void AddUint32Property(const napi_env env, napi_value object, const char *name, uint32_t value);

} // SAF_ASSET_COMMON
} // Security
} // OHOS

#endif // SAF_ASSET_NAPI_COMMON_H