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

#include "hpic_napi_common.h"
#include "psi_pir_error_code.h"

namespace OHOS::Security::PrivacyComputation {
int32_t MapProtocolError(int32_t result) noexcept
{
    switch (result) {
    case psi_pir::PSI_PIR_SUCCESS:
        return HPIC_SUCCESS;
    case psi_pir::PSI_PIR_INVALID_ARGUMENT:
        return HPIC_INVALID_ARGUMENT;
    case psi_pir::PSI_PIR_CODEC_ERROR:
        return HPIC_DATA_CORRUPTED;
    case psi_pir::PSI_PIR_HASH_ERROR:
    case psi_pir::PSI_PIR_RANDOM_ERROR:
    case psi_pir::PSI_PIR_CRYPTO_ERROR:
        return HPIC_CRYPTO_ERROR;
    case psi_pir::PSI_PIR_UNSUPPORTED_PROTOCOL:
    case psi_pir::PSI_PIR_INCOMPATIBLE_HE_PARAMETERS:
    case psi_pir::PSI_PIR_HE_ERROR: // M1 backend/bridge is not available.
        return HPIC_UNAVAILABLE;
    default:
        return HPIC_SERVICE_ERROR;
    }
}

const char *GetErrorMessage(int32_t code) noexcept
{
    switch (code) {
    case HPIC_INVALID_ARGUMENT:
        return "Invalid privacy computation parameters.";
    case HPIC_OUT_OF_MEMORY:
        return "Unable to allocate memory for Context.";
    case HPIC_DATA_CORRUPTED:
        return "Invalid privacy computation data format.";
    case HPIC_CRYPTO_ERROR:
        return "Privacy cryptography operation failed.";
    case HPIC_ACCOUNT_ERROR:
        return "Unable to capture the current OS account identity.";
    case HPIC_UNAVAILABLE:
        return "Privacy computation backend is unavailable.";
    default:
        return "Privacy computation operation failed.";
    }
}

napi_value CreateJsError(napi_env env, int32_t code, const char *message)
{
    napi_value text = nullptr;
    napi_value error = nullptr;
    napi_value number = nullptr;
    // Keep this primitive below ThrowJsError: do not recurse through CheckNapiStatus.
    IF_FALSE_RETURN(napi_create_string_utf8(env, message, NAPI_AUTO_LENGTH, &text) == napi_ok, nullptr);
    IF_FALSE_RETURN(napi_create_error(env, nullptr, text, &error) == napi_ok, nullptr);
    IF_FALSE_RETURN(napi_create_int32(env, code, &number) == napi_ok, nullptr);
    IF_FALSE_RETURN(napi_set_named_property(env, error, "code", number) == napi_ok, nullptr);
    return error;
}

napi_status ThrowJsError(napi_env env, int32_t code, const char *message)
{
    LOGE("NAPI operation failed, code=%{public}d", code);
    bool pending = false;
    IF_ERR_RETURN(napi_is_exception_pending(env, &pending));
    IF_FALSE_RETURN(!pending, napi_pending_exception);
    napi_value error = CreateJsError(env, code, message);
    if (error == nullptr) {
        LOGE("Failed to create JS error, code=%{public}d", code);
    }
    IF_FALSE_RETURN(error != nullptr, napi_generic_failure);
    return napi_throw(env, error);
}

napi_status CheckNapiStatus(napi_env env, napi_status status)
{
    if (status == napi_ok) {
        return napi_ok;
    }
    LOGE("NAPI call failed, status=%{public}d", static_cast<int32_t>(status));
    // Like asset's GET_AND_THROW_LAST_ERROR, preserve a getter/runtime exception.
    bool pending = false;
    if (napi_is_exception_pending(env, &pending) == napi_ok && !pending) {
        const int32_t code = status == napi_invalid_arg ? HPIC_INVALID_ARGUMENT : HPIC_SERVICE_ERROR;
        ThrowJsError(env, code, GetErrorMessage(code));
    }
    return status;
}

napi_status RejectPromise(napi_env env, napi_deferred deferred, int32_t code, const char *message)
{
    bool pending = false;
    IF_ERR_RETURN(napi_is_exception_pending(env, &pending));
    napi_value error = nullptr;
    if (pending) {
        IF_ERR_RETURN(napi_get_and_clear_last_exception(env, &error));
    } else {
        error = CreateJsError(env, code, message);
    }
    if (error == nullptr) {
        LOGE("Failed to obtain Promise rejection error, code=%{public}d", code);
    }
    IF_FALSE_RETURN(error != nullptr, napi_generic_failure);
    const napi_status status = napi_reject_deferred(env, deferred, error);
    if (status != napi_ok) {
        LOGE("Failed to reject Promise, status=%{public}d", static_cast<int32_t>(status));
    }
    return status;
}

napi_value CreateRejectedPromise(napi_env env, int32_t code, const char *message)
{
    napi_deferred deferred = nullptr;
    napi_value promise = nullptr;
    IF_FALSE_RETURN(CheckNapiStatus(env, napi_create_promise(env, &deferred, &promise)) == napi_ok, nullptr);
    IF_FALSE_RETURN(RejectPromise(env, deferred, code, message) == napi_ok, nullptr);
    return promise;
}

namespace {
void ResolvePromise(napi_env env, HpicBaseContext *context)
{
    if (context->result != HPIC_SUCCESS) {
        LOGE("Async operation failed, code=%{public}d", context->result);
        RejectPromise(env, context->deferred, context->result, GetErrorMessage(context->result));
        return;
    }

    napi_value output = nullptr;
    if (context->resolve(env, context, output) != napi_ok || output == nullptr) {
        LOGE("Failed to convert async result to JS output");
        context->result = HPIC_SERVICE_ERROR;
        RejectPromise(env, context->deferred, context->result, GetErrorMessage(context->result));
        return;
    }

    NAPI_CALL_RETURN_VOID(env, napi_resolve_deferred(env, context->deferred, output));
}

void CompleteAsyncWork(napi_env env, napi_status status, void *data)
{
    std::unique_ptr<HpicBaseContext> context(static_cast<HpicBaseContext *>(data));
    if (context == nullptr) {
        LOGE("Async completion context is null");
    }
    IF_FALSE_RETURN(context != nullptr, );
    if (status != napi_ok) {
        LOGE("Async work completion failed, status=%{public}d", static_cast<int32_t>(status));
    }
    context->result = status == napi_ok ? context->result : HPIC_SERVICE_ERROR;
    ResolvePromise(env, context.get());
}

napi_status PrepareAsyncWork(napi_env env, napi_callback_info info, HpicBaseContext *context, const char *resourceName)
{
    IF_ERR_RETURN(context->parse(env, info, context));
    context->result = HPIC_SERVICE_ERROR;
    napi_value resource = nullptr;
    NAPI_CALL_RETURN_ERR(env, napi_create_string_utf8(env, resourceName, NAPI_AUTO_LENGTH, &resource));
    NAPI_CALL_RETURN_ERR(env, napi_create_async_work(env, nullptr, resource, context->execute, CompleteAsyncWork,
                                                     context, &context->work));
    NAPI_CALL_RETURN_ERR(env, napi_queue_async_work(env, context->work));
    return napi_ok;
}
} // namespace

napi_value CreateAsyncWork(napi_env env, napi_callback_info info, std::unique_ptr<HpicBaseContext> context,
                           const char *resourceName)
{
    NAPI_THROW(env, context == nullptr, HPIC_OUT_OF_MEMORY, GetErrorMessage(HPIC_OUT_OF_MEMORY));
    NAPI_THROW(env, context->parse == nullptr || context->execute == nullptr || context->resolve == nullptr,
               HPIC_SERVICE_ERROR, "Invalid privacy computation async context.");
    context->env = env;
    napi_value promise = nullptr;
    IF_FALSE_RETURN(CheckNapiStatus(env, napi_create_promise(env, &context->deferred, &promise)) == napi_ok, nullptr);
    const napi_status status = PrepareAsyncWork(env, info, context.get(), resourceName);
    if (status != napi_ok) {
        LOGE("Failed to prepare %{public}s, status=%{public}d", resourceName, static_cast<int32_t>(status));
        // Once a Promise exists, failure must reject it, not just return from a NAPI_CALL macro.
        IF_FALSE_RETURN(RejectPromise(env, context->deferred, context->result, GetErrorMessage(context->result)) ==
                            napi_ok,
                        nullptr);
        return promise; // Context destructor deletes any work created before queue failure.
    }
    context.release(); // Ownership transfers to completion only after successful queueing.
    return promise;
}
} // namespace OHOS::Security::PrivacyComputation
