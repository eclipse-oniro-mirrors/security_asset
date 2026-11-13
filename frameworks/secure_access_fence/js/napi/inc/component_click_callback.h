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

#ifndef COMPONENT_CLICK_CALLBACK_H
#define COMPONENT_CLICK_CALLBACK_H

#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace OHOS::Security::SAF {

/**
 * @brief Register a JS callback to receive each simulated click result.
 *
 * Implements the public API abilityToolAccessCtrl.registerControlComponentResultCallback.
 * The click executor reports each result to the SAF service with the caller
 * uid as the session id; the service routes it back to this process through
 * the IGuiCallback IPC stub, and the result is delivered to the JS callback on
 * the JS thread via a threadsafe function.
 *
 * A repeated register call overwrites the previously registered callback.
 *
 * @param env The napi environment.
 * @param info The napi callback info; argv[0] must be a JS function.
 * @return undefined, or nullptr with a pending BusinessError thrown:
 *         24010000 invalid parameter, 24010001 service abnormal,
 *         24010002 common internal error, 201 permission denied.
 */
napi_value NapiRegisterControlComponentResultCallback(const napi_env env, napi_callback_info info);

/**
 * @brief Unregister the JS callback registered by
 *        NapiRegisterControlComponentResultCallback.
 *
 * Implements the public API
 * abilityToolAccessCtrl.unregisterControlComponentResultCallback. Calling it
 * without a prior successful register is an idempotent no-op.
 *
 * @param env The napi environment.
 * @param info The napi callback info (no arguments).
 * @return undefined, or nullptr with a pending BusinessError thrown:
 *         24010001 service abnormal, 24010002 common internal error.
 */
napi_value NapiUnregisterControlComponentResultCallback(const napi_env env, napi_callback_info info);

} // namespace OHOS::Security::SAF

#endif // COMPONENT_CLICK_CALLBACK_H
