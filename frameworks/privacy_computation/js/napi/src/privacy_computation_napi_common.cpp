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

#include "privacy_computation_napi_common.h"
#include "privacy_limits.h"
#include "securec.h"
#include <cmath>
#include <limits>
#include <utility>

namespace OHOS::Security::PrivacyComputation {
napi_status NapiGetNamedProperty(napi_env env, napi_value object, const char *name, napi_value &value)
{
    napi_valuetype type = napi_undefined;
    NAPI_CALL_RETURN_ERR(env, napi_typeof(env, object, &type));
    NAPI_THROW_RETURN_ERR(env, type != napi_object, PRIVACY_COMPUTATION_INVALID_ARGUMENT,
                          "Expected an object parameter.");
    NAPI_CALL_RETURN_ERR(env, napi_get_named_property(env, object, name, &value));
    NAPI_CALL_RETURN_ERR(env, napi_typeof(env, value, &type));
    NAPI_THROW_RETURN_ERR(env, type == napi_undefined, PRIVACY_COMPUTATION_INVALID_ARGUMENT,
                          (std::string("Missing required property: ") + name).c_str());
    return napi_ok;
}

namespace {
napi_status GetOptionalProperty(napi_env env, napi_value object, const char *name, napi_value &value, bool &present)
{
    NAPI_CALL_RETURN_ERR(env, napi_get_named_property(env, object, name, &value));
    napi_valuetype type = napi_undefined;
    NAPI_CALL_RETURN_ERR(env, napi_typeof(env, value, &type));
    present = type != napi_undefined;
    return napi_ok;
}

napi_status GetUint32Value(napi_env env, napi_value value, uint32_t &number, const char *message)
{
    napi_valuetype type = napi_undefined;
    NAPI_CALL_RETURN_ERR(env, napi_typeof(env, value, &type));
    NAPI_THROW_RETURN_ERR(env, type != napi_number, PRIVACY_COMPUTATION_INVALID_ARGUMENT, message);
    double raw = 0;
    NAPI_CALL_RETURN_ERR(env, napi_get_value_double(env, value, &raw));
    NAPI_THROW_RETURN_ERR(
        env, !std::isfinite(raw) || raw < 0 || raw > std::numeric_limits<uint32_t>::max() || std::floor(raw) != raw,
        PRIVACY_COMPUTATION_INVALID_ARGUMENT, message);
    number = static_cast<uint32_t>(raw);
    return napi_ok;
}

napi_status GetByteArrayValue(napi_env env, napi_value value, psi_pir::Bytes &bytes, const char *message)
{
    bool typed = false;
    NAPI_CALL_RETURN_ERR(env, napi_is_typedarray(env, value, &typed));
    NAPI_THROW_RETURN_ERR(env, !typed, PRIVACY_COMPUTATION_INVALID_ARGUMENT, message);
    napi_typedarray_type type;
    size_t count = 0;
    void *data = nullptr;
    NAPI_CALL_RETURN_ERR(env, napi_get_typedarray_info(env, value, &type, &count, &data, nullptr, nullptr));
    NAPI_THROW_RETURN_ERR(env, type != napi_uint8_array || (count != 0 && data == nullptr),
                          PRIVACY_COMPUTATION_INVALID_ARGUMENT, message);
    NAPI_THROW_RETURN_ERR(env, count > MAX_PRIVACY_BYTES, PRIVACY_COMPUTATION_INVALID_ARGUMENT,
                          "Privacy byte array is too large.");
    psi_pir::Bytes copy;
    if (count != 0) {
        const auto *begin = static_cast<const uint8_t *>(data);
        copy.assign(begin, begin + count);
    }
    bytes = std::move(copy);
    return napi_ok;
}

napi_status ParseJsHashAlg(napi_env env, napi_value object, psi_pir::HashAlg &hash)
{
    napi_value value = nullptr;
    bool present = false;
    IF_ERR_RETURN(GetOptionalProperty(env, object, "hashAlg", value, present));
    uint32_t id = HASH_ALG_NONE;
    if (present) {
        IF_ERR_RETURN(GetUint32Value(env, value, id, "hashAlg must be a supported integer enum."));
    }
    switch (id) {
        case HASH_ALG_NONE:
            hash = psi_pir::HashAlg::NONE;
            break;
        case HASH_ALG_SHA256:
            hash = psi_pir::HashAlg::SHA256;
            break;
        case HASH_ALG_SHA384:
            hash = psi_pir::HashAlg::SHA384;
            break;
        case HASH_ALG_SHA512:
            hash = psi_pir::HashAlg::SHA512;
            break;
        default:
            NAPI_THROW_RETURN_ERR(env, true, PRIVACY_COMPUTATION_INVALID_ARGUMENT,
                                  "hashAlg is outside the supported enum range.");
    }
    return napi_ok;
}

napi_status GetArrayLength(napi_env env, napi_value value, uint32_t &count, const char *message)
{
    bool array = false;
    NAPI_CALL_RETURN_ERR(env, napi_is_array(env, value, &array));
    NAPI_THROW_RETURN_ERR(env, !array, PRIVACY_COMPUTATION_INVALID_ARGUMENT, message);
    NAPI_CALL_RETURN_ERR(env, napi_get_array_length(env, value, &count));
    return napi_ok;
}

napi_status ParseJsByteArray(napi_env env, napi_value value, std::vector<psi_pir::Bytes> &bytes, size_t &total)
{
    uint32_t count = 0;
    IF_ERR_RETURN(GetArrayLength(env, value, count, "Ciphertexts must be an array of Uint8Array."));
    NAPI_THROW_RETURN_ERR(env, count > MAX_PRIVACY_ITEMS || total > MAX_PRIVACY_BYTES - PRIVACY_LENGTH_BYTES,
                          PRIVACY_COMPUTATION_INVALID_ARGUMENT, "Privacy ciphertext list is too large.");
    total += PRIVACY_LENGTH_BYTES;
    for (uint32_t i = 0; i < count; ++i) {
        napi_value item = nullptr;
        NAPI_CALL_RETURN_ERR(env, napi_get_element(env, value, i, &item));
        psi_pir::Bytes entry;
        IF_ERR_RETURN(GetByteArrayValue(env, item, entry, "Each ciphertext must be a Uint8Array."));
        NAPI_THROW_RETURN_ERR(env,
                              total > MAX_PRIVACY_BYTES - PRIVACY_LENGTH_BYTES ||
                                  entry.size() > MAX_PRIVACY_BYTES - total - PRIVACY_LENGTH_BYTES,
                              PRIVACY_COMPUTATION_INVALID_ARGUMENT, "Privacy request is too large.");
        total += PRIVACY_LENGTH_BYTES + entry.size();
        bytes.push_back(std::move(entry));
    }
    return napi_ok;
}

napi_status ParseJsElement(napi_env env, napi_value value, psi_pir::Element &element)
{
    IF_ERR_RETURN(NapiGetProperty(env, value, "elemKey", element.elemKey));
    IF_ERR_RETURN(ParseJsHashAlg(env, value, element.hashAlg));
    napi_value attached = nullptr;
    bool present = false;
    IF_ERR_RETURN(GetOptionalProperty(env, value, "elemValue", attached, present));
    if (present) {
        element.elemValue.emplace();
        IF_ERR_RETURN(
            GetByteArrayValue(env, attached, *element.elemValue, "elemValue must be a Uint8Array when provided."));
    }
    return napi_ok;
}

} // namespace

napi_status ParseJsArgs(napi_env env, napi_callback_info info, napi_value *args, size_t expectedCount)
{
    constexpr size_t MAX_ARGS_COUNT = 4;
    napi_value values[MAX_ARGS_COUNT] = {nullptr};
    size_t count = MAX_ARGS_COUNT;
    NAPI_CALL_RETURN_ERR(env, napi_get_cb_info(env, info, &count, values, nullptr, nullptr));
    NAPI_THROW_RETURN_ERR(env, count != expectedCount || expectedCount >= MAX_ARGS_COUNT,
                          PRIVACY_COMPUTATION_INVALID_ARGUMENT, "Unexpected number of arguments.");
    NAPI_THROW_RETURN_ERR(
        env, memcpy_s(args, expectedCount * sizeof(napi_value), values, expectedCount * sizeof(napi_value)) != EOK,
        PRIVACY_COMPUTATION_SERVICE_ERROR, "Unable to copy NAPI arguments.");
    return napi_ok;
}

napi_status NapiGetProperty(napi_env env, napi_value value, psi_pir::Bytes &target)
{
    return GetByteArrayValue(env, value, target, "Expected a Uint8Array.");
}

napi_status NapiGetProperty(napi_env env, napi_value value, psi_pir::PrivacyProtocol &protocol)
{
    uint32_t sizeId = 0;
    uint32_t modeId = 0;
    IF_ERR_RETURN(NapiGetProperty(env, value, "dataSetSize", sizeId));
    IF_ERR_RETURN(NapiGetProperty(env, value, "protocolType", modeId));
    NAPI_THROW_RETURN_ERR(env,
                          sizeId > static_cast<uint32_t>(psi_pir::DataSetSize::SIZE_512) ||
                              modeId > static_cast<uint32_t>(psi_pir::ProtocolType::PIR_PROTOCOL),
                          PRIVACY_COMPUTATION_INVALID_ARGUMENT, "Unsupported protocol enum value.");
    protocol.dataSetSize = static_cast<psi_pir::DataSetSize>(sizeId);
    protocol.protocolType = static_cast<psi_pir::ProtocolType>(modeId);
    return napi_ok;
}

napi_status NapiGetProperty(napi_env env, napi_value value, psi_pir::TargetInput &target)
{
    IF_ERR_RETURN(NapiGetProperty(env, value, "elemData", target.elemData));
    IF_ERR_RETURN(ParseJsHashAlg(env, value, target.hashAlg));
    return napi_ok;
}

napi_status NapiGetProperty(napi_env env, napi_value value, std::vector<psi_pir::Element> &elements)
{
    uint32_t count = 0;
    IF_ERR_RETURN(GetArrayLength(env, value, count, "elements must be an array."));
    NAPI_THROW_RETURN_ERR(env, count > MAX_PRIVACY_ITEMS, PRIVACY_COMPUTATION_INVALID_ARGUMENT,
                          "Too many privacy search elements.");
    size_t total = 0;
    for (uint32_t i = 0; i < count; ++i) {
        napi_value item = nullptr;
        NAPI_CALL_RETURN_ERR(env, napi_get_element(env, value, i, &item));
        psi_pir::Element element;
        IF_ERR_RETURN(ParseJsElement(env, item, element));
        const size_t valueSize = element.elemValue ? element.elemValue->size() : 0;
        const size_t elementSize = element.elemKey.size() + valueSize;
        NAPI_THROW_RETURN_ERR(env, elementSize > MAX_PRIVACY_BYTES - total, PRIVACY_COMPUTATION_INVALID_ARGUMENT,
                              "Privacy search elements exceed the byte budget.");
        total += elementSize;
        elements.push_back(std::move(element));
    }
    return napi_ok;
}

napi_status NapiGetProperty(napi_env env, napi_value value, psi_pir::PrivacySearchResult &result)
{
    napi_value ciphertext = nullptr;
    IF_ERR_RETURN(NapiGetNamedProperty(env, value, "resultCipherText", ciphertext));
    size_t total = SEARCH_FIXED_BYTES;
    IF_ERR_RETURN(ParseJsByteArray(env, ciphertext, result.resultCipherText, total));
    bool present = false;
    IF_ERR_RETURN(GetOptionalProperty(env, value, "valueCipherText", ciphertext, present));
    if (present) {
        result.valueCipherText.emplace();
        IF_ERR_RETURN(ParseJsByteArray(env, ciphertext, *result.valueCipherText, total));
    }
    return napi_ok;
}

napi_status NapiSetProperty(napi_env env, const psi_pir::Bytes &bytes, napi_value &output)
{
    void *data = nullptr;
    napi_value buffer = nullptr;
    NAPI_CALL_RETURN_ERR(env, napi_create_arraybuffer(env, bytes.size(), &data, &buffer));
    if (!bytes.empty()) {
        NAPI_THROW_RETURN_ERR(env, data == nullptr, PRIVACY_COMPUTATION_SERVICE_ERROR,
                              "Unable to create output ArrayBuffer.");
        NAPI_THROW_RETURN_ERR(env, memcpy_s(data, bytes.size(), bytes.data(), bytes.size()) != EOK,
                              PRIVACY_COMPUTATION_SERVICE_ERROR, "Unable to copy output bytes.");
    }
    NAPI_CALL_RETURN_ERR(env, napi_create_typedarray(env, napi_uint8_array, bytes.size(), buffer, 0, &output));
    return napi_ok;
}

napi_status NapiSetProperty(napi_env env, const psi_pir::PrivacySearchResult &result, napi_value &output)
{
    NAPI_CALL_RETURN_ERR(env, napi_create_object(env, &output));
    IF_ERR_RETURN(NapiSetProperty(env, output, "resultCipherText", result.resultCipherText));
    if (result.valueCipherText) {
        IF_ERR_RETURN(NapiSetProperty(env, output, "valueCipherText", *result.valueCipherText));
    }
    return napi_ok;
}

napi_status NapiSetProperty(napi_env env, const psi_pir::SearchResult &result, napi_value &output)
{
    NAPI_CALL_RETURN_ERR(env, napi_create_object(env, &output));
    IF_ERR_RETURN(NapiSetProperty(env, output, "matchedResult", result.matchedResult));
    if (result.attachedValues) {
        IF_ERR_RETURN(NapiSetProperty(env, output, "attachedValues", *result.attachedValues));
    }
    return napi_ok;
}

napi_status NapiSetProperty(napi_env env, const std::vector<psi_pir::Bytes> &bytes, napi_value &output)
{
    IF_FALSE_RETURN(bytes.size() <= std::numeric_limits<uint32_t>::max(), napi_generic_failure);
    NAPI_CALL_RETURN_ERR(env, napi_create_array_with_length(env, bytes.size(), &output));
    for (uint32_t i = 0; i < bytes.size(); ++i) {
        napi_value item = nullptr;
        IF_ERR_RETURN(NapiSetProperty(env, bytes[i], item));
        NAPI_CALL_RETURN_ERR(env, napi_set_element(env, output, i, item));
    }
    return napi_ok;
}

napi_status NapiGetProperty(napi_env env, napi_value value, uint32_t &number)
{
    return GetUint32Value(env, value, number, "Expected an unsigned integer.");
}

napi_status NapiSetProperty(napi_env env, uint32_t value, napi_value &output)
{
    return CheckNapiStatus(env, napi_create_uint32(env, value, &output));
}

napi_status NapiSetProperty(napi_env env, bool value, napi_value &output)
{
    return CheckNapiStatus(env, napi_get_boolean(env, value, &output));
}

PrivacyComputationBaseContext::~PrivacyComputationBaseContext()
{
    if (work != nullptr && env != nullptr) {
        const napi_status status = napi_delete_async_work(env, work);
        if (status != napi_ok) {
            LOGE("Failed to delete async work, status=%{public}d", static_cast<int32_t>(status));
        }
        work = nullptr;
    }
}

int32_t MapProtocolError(int32_t result) noexcept
{
    switch (result) {
        case psi_pir::PSI_PIR_SUCCESS:
            return PRIVACY_COMPUTATION_SUCCESS;
        case psi_pir::PSI_PIR_INVALID_ARGUMENT:
            return PRIVACY_COMPUTATION_INVALID_ARGUMENT;
        case psi_pir::PSI_PIR_CODEC_ERROR:
            return PRIVACY_COMPUTATION_DATA_CORRUPTED;
        case psi_pir::PSI_PIR_HASH_ERROR:
        case psi_pir::PSI_PIR_RANDOM_ERROR:
        case psi_pir::PSI_PIR_CRYPTO_ERROR:
            return PRIVACY_COMPUTATION_CRYPTO_ERROR;
        case psi_pir::PSI_PIR_UNSUPPORTED_PROTOCOL:
        case psi_pir::PSI_PIR_INCOMPATIBLE_HE_PARAMETERS:
        case psi_pir::PSI_PIR_HE_ERROR:
            return PRIVACY_COMPUTATION_UNAVAILABLE;
        default:
            return PRIVACY_COMPUTATION_SERVICE_ERROR;
    }
}

const char *GetErrorMessage(int32_t code) noexcept
{
    switch (code) {
        case PRIVACY_COMPUTATION_INVALID_ARGUMENT:
            return "Invalid privacy computation parameters.";
        case PRIVACY_COMPUTATION_OUT_OF_MEMORY:
            return "Unable to allocate memory for Context.";
        case PRIVACY_COMPUTATION_DATA_CORRUPTED:
            return "Invalid privacy computation data format.";
        case PRIVACY_COMPUTATION_CRYPTO_ERROR:
            return "Privacy cryptography operation failed.";
        case PRIVACY_COMPUTATION_ACCOUNT_ERROR:
            return "Unable to capture the current OS account identity.";
        case PRIVACY_COMPUTATION_UNAVAILABLE:
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
        const int32_t code =
            status == napi_invalid_arg ? PRIVACY_COMPUTATION_INVALID_ARGUMENT : PRIVACY_COMPUTATION_SERVICE_ERROR;
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
void ResolvePromise(napi_env env, PrivacyComputationBaseContext *context)
{
    if (context->result != PRIVACY_COMPUTATION_SUCCESS) {
        LOGE("NAPI %{public}s failed, code=%{public}d", context->operationName.c_str(), context->result);
        RejectPromise(env, context->deferred, context->result, GetErrorMessage(context->result));
        return;
    }

    napi_value output = nullptr;
    if (context->resolve(env, context, output) != napi_ok || output == nullptr) {
        LOGE("Failed to convert async result to JS output");
        context->result = PRIVACY_COMPUTATION_SERVICE_ERROR;
        RejectPromise(env, context->deferred, context->result, GetErrorMessage(context->result));
        return;
    }

    PRIVACY_NAPI_CALL_RETURN_VOID(env, napi_resolve_deferred(env, context->deferred, output));
    LOGI("NAPI %{public}s success, promise resolved", context->operationName.c_str());
}

void CompleteAsyncWork(napi_env env, napi_status status, void *data)
{
    std::unique_ptr<PrivacyComputationBaseContext> context(static_cast<PrivacyComputationBaseContext *>(data));
    if (context == nullptr) {
        LOGE("Async completion context is null");
    }
    IF_FALSE_RETURN(context != nullptr, );
    if (status != napi_ok) {
        LOGE("Async work completion failed, status=%{public}d", static_cast<int32_t>(status));
    }
    context->result = status == napi_ok ? context->result : PRIVACY_COMPUTATION_SERVICE_ERROR;
    ResolvePromise(env, context.get());
}

napi_status PrepareAsyncWork(napi_env env, napi_callback_info info, PrivacyComputationBaseContext *context,
                             const char *resourceName)
{
    IF_ERR_RETURN(context->parse(env, info, context));
    LOGI("NAPI %{public}s arguments parsed", resourceName);
    context->result = PRIVACY_COMPUTATION_SERVICE_ERROR;
    napi_value resource = nullptr;
    NAPI_CALL_RETURN_ERR(env, napi_create_string_utf8(env, resourceName, NAPI_AUTO_LENGTH, &resource));
    NAPI_CALL_RETURN_ERR(env, napi_create_async_work(env, nullptr, resource, context->execute, CompleteAsyncWork,
                                                     context, &context->work));
    NAPI_CALL_RETURN_ERR(env, napi_queue_async_work(env, context->work));
    return napi_ok;
}
} // namespace

napi_value CreateAsyncWork(napi_env env, napi_callback_info info,
                           std::unique_ptr<PrivacyComputationBaseContext> context, const char *resourceName)
{
    NAPI_THROW(env, context == nullptr, PRIVACY_COMPUTATION_OUT_OF_MEMORY,
               GetErrorMessage(PRIVACY_COMPUTATION_OUT_OF_MEMORY));
    NAPI_THROW(env, context->parse == nullptr || context->execute == nullptr || context->resolve == nullptr,
               PRIVACY_COMPUTATION_SERVICE_ERROR, "Invalid privacy computation async context.");
    context->env = env;
    context->operationName = resourceName;
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
    LOGI("NAPI %{public}s async work queued", resourceName);
    context.release(); // Ownership transfers to completion only after successful queueing.
    return promise;
}

} // namespace OHOS::Security::PrivacyComputation
