/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef HPIC_NAPI_COMMON_H
#define HPIC_NAPI_COMMON_H

#include "hpic_log.h"
#include "hpic_napi_context.h"
#include <memory>

namespace OHOS::Security::PrivacyComputation {
// Match asset's macro roles. Calls and conditions are evaluated once.
#define IF_ERR_RETURN(call)                                                                                            \
    do {                                                                                                               \
        const napi_status callStatus = (call);                                                                         \
        if (callStatus != napi_ok) {                                                                                   \
            return callStatus;                                                                                         \
        }                                                                                                              \
    } while (false)

#define IF_FALSE_RETURN(condition, returnValue)                                                                        \
    do {                                                                                                               \
        if (!(condition)) {                                                                                            \
            return returnValue;                                                                                        \
        }                                                                                                              \
    } while (false)

#define NAPI_CALL_RETURN_ERR(env, call)                                                                                \
    IF_ERR_RETURN(OHOS::Security::PrivacyComputation::CheckNapiStatus((env), (call)))

#define NAPI_CALL_RETURN_VOID(env, call)                                                                               \
    IF_FALSE_RETURN(OHOS::Security::PrivacyComputation::CheckNapiStatus((env), (call)) == napi_ok, )

#define NAPI_THROW_BASE(env, condition, returnValue, code, message)                                                    \
    do {                                                                                                               \
        if (condition) {                                                                                               \
            LOGE("NAPI validation failed: %{public}s", #condition);                                                    \
            OHOS::Security::PrivacyComputation::ThrowJsError((env), (code), (message));                                \
            return returnValue;                                                                                        \
        }                                                                                                              \
    } while (false)

#define NAPI_THROW(env, condition, code, message) NAPI_THROW_BASE(env, condition, nullptr, code, message)

#define NAPI_THROW_RETURN_ERR(env, condition, code, message)                                                           \
    NAPI_THROW_BASE(env, condition, napi_invalid_arg, code, message)

napi_status CheckNapiStatus(napi_env env, napi_status status);
napi_status ThrowJsError(napi_env env, int32_t code, const char *message);
napi_value CreateJsError(napi_env env, int32_t code, const char *message);
napi_status RejectPromise(napi_env env, napi_deferred deferred, int32_t code, const char *message);
napi_value CreateRejectedPromise(napi_env env, int32_t code, const char *message);
napi_value CreateAsyncWork(napi_env env, napi_callback_info info, std::unique_ptr<HpicBaseContext> context,
                           const char *resourceName);

} // namespace OHOS::Security::PrivacyComputation

#endif // HPIC_NAPI_COMMON_H
