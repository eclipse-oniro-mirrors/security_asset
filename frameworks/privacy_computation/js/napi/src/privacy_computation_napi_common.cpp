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
    if (const auto callStatus = CheckNapiStatus(env, napi_typeof(env, object, &type)); callStatus != napi_ok) {
        return callStatus;
    }
    if (type != napi_object) {
        ThrowJsError(env, PRIVACY_COMPUTATION_INVALID_ARGUMENT, "Expected an object parameter.");
        return napi_invalid_arg;
    }
    if (const auto callStatus = CheckNapiStatus(env, napi_get_named_property(env, object, name, &value));
        callStatus != napi_ok) {
        return callStatus;
    }
    if (const auto callStatus = CheckNapiStatus(env, napi_typeof(env, value, &type)); callStatus != napi_ok) {
        return callStatus;
    }
    if (type == napi_undefined) {
        ThrowJsError(env, PRIVACY_COMPUTATION_INVALID_ARGUMENT,
                     (std::string("Missing required property: ") + name).c_str());
        return napi_invalid_arg;
    }
    return napi_ok;
}

namespace {
constexpr size_t MAX_KEY_BYTES = 1024;
constexpr size_t SHA256_DIGEST_BYTES = 32;
constexpr size_t SHA384_DIGEST_BYTES = 48;
constexpr size_t SHA512_DIGEST_BYTES = 64;

napi_status GetOptionalProperty(napi_env env, napi_value object, const char *name, napi_value &value, bool &present)
{
    if (const auto callStatus = CheckNapiStatus(env, napi_get_named_property(env, object, name, &value));
        callStatus != napi_ok) {
        return callStatus;
    }
    napi_valuetype type = napi_undefined;
    if (const auto callStatus = CheckNapiStatus(env, napi_typeof(env, value, &type)); callStatus != napi_ok) {
        return callStatus;
    }
    present = type != napi_undefined;
    return napi_ok;
}

napi_status GetUint32Value(napi_env env, napi_value value, uint32_t &number, const char *message)
{
    napi_valuetype type = napi_undefined;
    if (const auto callStatus = CheckNapiStatus(env, napi_typeof(env, value, &type)); callStatus != napi_ok) {
        return callStatus;
    }
    if (type != napi_number) {
        ThrowJsError(env, PRIVACY_COMPUTATION_INVALID_ARGUMENT, message);
        return napi_invalid_arg;
    }
    double raw = 0;
    if (const auto callStatus = CheckNapiStatus(env, napi_get_value_double(env, value, &raw)); callStatus != napi_ok) {
        return callStatus;
    }
    if (!std::isfinite(raw) || raw < 0 || raw > std::numeric_limits<uint32_t>::max() || std::floor(raw) != raw) {
        ThrowJsError(env, PRIVACY_COMPUTATION_INVALID_ARGUMENT, message);
        return napi_invalid_arg;
    }
    number = static_cast<uint32_t>(raw);
    return napi_ok;
}

napi_status GetByteArrayValue(napi_env env, napi_value value, Bytes &bytes,
    const char *message, size_t maxBytes = MAX_PRIVACY_BYTES)
{
    bool typed = false;
    if (const auto callStatus = CheckNapiStatus(env, napi_is_typedarray(env, value, &typed)); callStatus != napi_ok) {
        return callStatus;
    }
    if (!typed) {
        ThrowJsError(env, PRIVACY_COMPUTATION_INVALID_ARGUMENT, message);
        return napi_invalid_arg;
    }
    napi_typedarray_type type;
    size_t count = 0;
    void *data = nullptr;
    if (const auto callStatus =
            CheckNapiStatus(env, napi_get_typedarray_info(env, value, &type, &count, &data, nullptr, nullptr));
        callStatus != napi_ok) {
        return callStatus;
    }
    if (type != napi_uint8_array || (count != 0 && data == nullptr)) {
        ThrowJsError(env, PRIVACY_COMPUTATION_INVALID_ARGUMENT, message);
        return napi_invalid_arg;
    }
    if (count > maxBytes) {
        ThrowJsError(env, PRIVACY_COMPUTATION_INVALID_ARGUMENT, "Privacy byte array is too large.");
        return napi_invalid_arg;
    }
    Bytes copy;
    if (count != 0) {
        const auto *begin = static_cast<const uint8_t *>(data);
        copy.assign(begin, begin + count);
    }
    bytes = std::move(copy);
    return napi_ok;
}

napi_status ParseJsHashAlg(napi_env env, napi_value object, HashAlg &hash)
{
    napi_value value = nullptr;
    bool present = false;
    if (const auto callStatus = GetOptionalProperty(env, object, "hashAlg", value, present); callStatus != napi_ok) {
        return callStatus;
    }
    uint32_t id = HASH_ALG_NONE;
    if (present) {
        if (const auto callStatus = GetUint32Value(env, value, id, "hashAlg must be a supported integer enum.");
            callStatus != napi_ok) {
            return callStatus;
        }
    }
    switch (id) {
        case HASH_ALG_NONE:
            hash = HashAlg::NONE;
            break;
        case HASH_ALG_SHA256:
            hash = HashAlg::SHA256;
            break;
        case HASH_ALG_SHA384:
            hash = HashAlg::SHA384;
            break;
        case HASH_ALG_SHA512:
            hash = HashAlg::SHA512;
            break;
        default:
            ThrowJsError(env, PRIVACY_COMPUTATION_INVALID_ARGUMENT, "hashAlg is outside the supported enum range.");
            return napi_invalid_arg;
    }
    return napi_ok;
}

napi_status ParseJsKey(napi_env env, napi_value object, const char *name, Bytes &bytes,
    HashAlg &hash)
{
    napi_value value = nullptr;
    if (const auto status = NapiGetNamedProperty(env, object, name, value); status != napi_ok) {
        return status;
    }
    if (const auto status = GetByteArrayValue(env, value, bytes, "Expected a Uint8Array.", MAX_KEY_BYTES);
        status != napi_ok) {
        return status;
    }
    if (const auto status = ParseJsHashAlg(env, object, hash); status != napi_ok) {
        return status;
    }
    const size_t size = bytes.size();
    if (size == 0) {
        ThrowJsError(env, PRIVACY_COMPUTATION_INVALID_ARGUMENT, "elemData and elemKey must not be empty.");
        return napi_invalid_arg;
    }
    if (hash == HashAlg::NONE &&
        size != SHA256_DIGEST_BYTES && size != SHA384_DIGEST_BYTES && size != SHA512_DIGEST_BYTES) {
        ThrowJsError(env, PRIVACY_COMPUTATION_INVALID_ARGUMENT, "A prehashed key must contain 32, 48 or 64 bytes.");
        return napi_invalid_arg;
    }
    return napi_ok;
}

napi_status GetArrayLength(napi_env env, napi_value value, uint32_t &count, const char *message)
{
    bool array = false;
    if (const auto callStatus = CheckNapiStatus(env, napi_is_array(env, value, &array)); callStatus != napi_ok) {
        return callStatus;
    }
    if (!array) {
        ThrowJsError(env, PRIVACY_COMPUTATION_INVALID_ARGUMENT, message);
        return napi_invalid_arg;
    }
    if (const auto callStatus = CheckNapiStatus(env, napi_get_array_length(env, value, &count));
        callStatus != napi_ok) {
        return callStatus;
    }
    return napi_ok;
}

napi_status ParseJsByteArray(napi_env env, napi_value value, std::vector<Bytes> &bytes,
                             size_t &total)
{
    uint32_t count = 0;
    if (const auto callStatus = GetArrayLength(env, value, count, "Ciphertexts must be an array of Uint8Array.");
        callStatus != napi_ok) {
        return callStatus;
    }
    if (count > MAX_PRIVACY_ITEMS || total > MAX_PRIVACY_BYTES - PRIVACY_LENGTH_BYTES) {
        ThrowJsError(env, PRIVACY_COMPUTATION_INVALID_ARGUMENT, "Privacy ciphertext list is too large.");
        return napi_invalid_arg;
    }
    total += PRIVACY_LENGTH_BYTES;
    for (uint32_t i = 0; i < count; ++i) {
        napi_value item = nullptr;
        if (const auto callStatus = CheckNapiStatus(env, napi_get_element(env, value, i, &item));
            callStatus != napi_ok) {
            return callStatus;
        }
        Bytes entry;
        if (const auto callStatus = GetByteArrayValue(env, item, entry, "Each ciphertext must be a Uint8Array.");
            callStatus != napi_ok) {
            return callStatus;
        }
        if (total > MAX_PRIVACY_BYTES - PRIVACY_LENGTH_BYTES ||
            entry.size() > MAX_PRIVACY_BYTES - total - PRIVACY_LENGTH_BYTES) {
            ThrowJsError(env, PRIVACY_COMPUTATION_INVALID_ARGUMENT, "Privacy request is too large.");
            return napi_invalid_arg;
        }
        total += PRIVACY_LENGTH_BYTES + entry.size();
        bytes.push_back(std::move(entry));
    }
    return napi_ok;
}

napi_status ParseJsElement(napi_env env, napi_value value, Element &element,
    ProtocolType protocolType)
{
    const auto status = ParseJsKey(env, value, "elemKey", element.elemKey, element.hashAlg);
    if (status != napi_ok || protocolType != ProtocolType::PIR_PROTOCOL) {
        return status;
    }
    element.elemValue.emplace();
    return NapiGetProperty(env, value, "elemValue", *element.elemValue);
}

} // namespace

napi_status ParseJsArgs(napi_env env, napi_callback_info info, napi_value *args, size_t expectedCount)
{
    constexpr size_t maxArgsCount = 4;
    napi_value values[maxArgsCount] = {nullptr};
    size_t count = maxArgsCount;
    if (const auto callStatus = CheckNapiStatus(env, napi_get_cb_info(env, info, &count, values, nullptr, nullptr));
        callStatus != napi_ok) {
        return callStatus;
    }
    if (count != expectedCount || expectedCount >= maxArgsCount) {
        ThrowJsError(env, PRIVACY_COMPUTATION_INVALID_ARGUMENT, "Unexpected number of arguments.");
        return napi_invalid_arg;
    }
    if (memcpy_s(args, expectedCount * sizeof(napi_value), values, expectedCount * sizeof(napi_value)) != EOK) {
        ThrowJsError(env, PRIVACY_COMPUTATION_SERVICE_ERROR, "Unable to copy NAPI arguments.");
        return napi_invalid_arg;
    }
    return napi_ok;
}

napi_status NapiGetProperty(napi_env env, napi_value value, Bytes &target)
{
    return GetByteArrayValue(env, value, target, "Expected a Uint8Array.");
}

napi_status NapiGetProperty(napi_env env, napi_value value, PrivacyProtocol &protocol)
{
    uint32_t sizeId = 0;
    uint32_t modeId = 0;
    if (const auto callStatus = NapiGetProperty(env, value, "dataSetSize", sizeId); callStatus != napi_ok) {
        return callStatus;
    }
    if (const auto callStatus = NapiGetProperty(env, value, "protocolType", modeId); callStatus != napi_ok) {
        return callStatus;
    }
    if (sizeId > static_cast<uint32_t>(DataSetSize::SIZE_512) ||
        modeId > static_cast<uint32_t>(ProtocolType::PIR_PROTOCOL)) {
        ThrowJsError(env, PRIVACY_COMPUTATION_INVALID_ARGUMENT, "Unsupported protocol enum value.");
        return napi_invalid_arg;
    }
    const auto size = static_cast<DataSetSize>(sizeId);
    const auto type = static_cast<ProtocolType>(modeId);
    const bool isIntersection = type == ProtocolType::PRIVATE_SET_INTERSECTION;
    if (isIntersection != (size == DataSetSize::SIZE_512)) {
        ThrowJsError(env, PRIVACY_COMPUTATION_INVALID_ARGUMENT, "Unsupported protocol and dataSetSize combination.");
        return napi_invalid_arg;
    }
    protocol.dataSetSize = size;
    protocol.protocolType = type;
    return napi_ok;
}

napi_status NapiGetProperty(napi_env env, napi_value value, TargetInput &target)
{
    return ParseJsKey(env, value, "elemData", target.elemData, target.hashAlg);
}

napi_status NapiGetProperty(napi_env env, napi_value value, std::vector<Element> &elements,
    ProtocolType protocolType)
{
    uint32_t count = 0;
    if (const auto callStatus = GetArrayLength(env, value, count, "elements must be an array.");
        callStatus != napi_ok) {
        return callStatus;
    }
    if (count == 0 || count > MAX_PRIVACY_ITEMS) {
        ThrowJsError(env, PRIVACY_COMPUTATION_INVALID_ARGUMENT, "Invalid privacy search element count.");
        return napi_invalid_arg;
    }
    size_t total = 0;
    for (uint32_t i = 0; i < count; ++i) {
        napi_value item = nullptr;
        if (const auto callStatus = CheckNapiStatus(env, napi_get_element(env, value, i, &item));
            callStatus != napi_ok) {
            return callStatus;
        }
        Element element;
        if (const auto callStatus = ParseJsElement(env, item, element, protocolType); callStatus != napi_ok) {
            return callStatus;
        }
        const size_t valueSize = element.elemValue ? element.elemValue->size() : 0;
        const size_t elementSize = element.elemKey.size() + valueSize;
        if (elementSize > MAX_PRIVACY_BYTES - total) {
            ThrowJsError(env, PRIVACY_COMPUTATION_INVALID_ARGUMENT, "Privacy search elements exceed the byte budget.");
            return napi_invalid_arg;
        }
        total += elementSize;
        elements.push_back(std::move(element));
    }
    return napi_ok;
}

napi_status NapiGetProperty(napi_env env, napi_value value, PrivacySearchResult &result,
    ProtocolType protocolType)
{
    napi_value ciphertext = nullptr;
    if (const auto callStatus = NapiGetNamedProperty(env, value, "resultCipherText", ciphertext);
        callStatus != napi_ok) {
        return callStatus;
    }
    size_t total = SEARCH_FIXED_BYTES;
    if (const auto callStatus = ParseJsByteArray(env, ciphertext, result.resultCipherText, total);
        callStatus != napi_ok) {
        return callStatus;
    }
    if (result.resultCipherText.empty()) {
        ThrowJsError(env, PRIVACY_COMPUTATION_INVALID_ARGUMENT, "resultCipherText must contain at least one batch.");
        return napi_invalid_arg;
    }
    if (protocolType != ProtocolType::PIR_PROTOCOL) {
        return napi_ok;
    }
    bool present = false;
    if (const auto callStatus = GetOptionalProperty(env, value, "valueCipherText", ciphertext, present);
        callStatus != napi_ok) {
        return callStatus;
    }
    if (present) {
        result.valueCipherText.emplace();
        if (const auto callStatus = ParseJsByteArray(env, ciphertext, *result.valueCipherText, total);
            callStatus != napi_ok) {
            return callStatus;
        }
    }
    return napi_ok;
}

napi_status NapiSetProperty(napi_env env, const Bytes &bytes, napi_value &output)
{
    void *data = nullptr;
    napi_value buffer = nullptr;
    if (const auto callStatus = CheckNapiStatus(env, napi_create_arraybuffer(env, bytes.size(), &data, &buffer));
        callStatus != napi_ok) {
        return callStatus;
    }
    if (!bytes.empty()) {
        if (data == nullptr) {
            ThrowJsError(env, PRIVACY_COMPUTATION_SERVICE_ERROR, "Unable to create output ArrayBuffer.");
            return napi_invalid_arg;
        }
        if (memcpy_s(data, bytes.size(), bytes.data(), bytes.size()) != EOK) {
            ThrowJsError(env, PRIVACY_COMPUTATION_SERVICE_ERROR, "Unable to copy output bytes.");
            return napi_invalid_arg;
        }
    }
    if (const auto callStatus =
            CheckNapiStatus(env, napi_create_typedarray(env, napi_uint8_array, bytes.size(), buffer, 0, &output));
        callStatus != napi_ok) {
        return callStatus;
    }
    return napi_ok;
}

napi_status NapiSetProperty(napi_env env, const PrivacySearchResult &result, napi_value &output)
{
    if (const auto callStatus = CheckNapiStatus(env, napi_create_object(env, &output)); callStatus != napi_ok) {
        return callStatus;
    }
    if (const auto callStatus = NapiSetProperty(env, output, "resultCipherText", result.resultCipherText);
        callStatus != napi_ok) {
        return callStatus;
    }
    if (result.valueCipherText) {
        if (const auto callStatus = NapiSetProperty(env, output, "valueCipherText", *result.valueCipherText);
            callStatus != napi_ok) {
            return callStatus;
        }
    }
    return napi_ok;
}

napi_status NapiSetProperty(napi_env env, const SearchResult &result, napi_value &output)
{
    if (const auto callStatus = CheckNapiStatus(env, napi_create_object(env, &output)); callStatus != napi_ok) {
        return callStatus;
    }
    if (const auto callStatus = NapiSetProperty(env, output, "matchedResult", result.matchedResult);
        callStatus != napi_ok) {
        return callStatus;
    }
    if (result.attachedValues) {
        if (const auto callStatus = NapiSetProperty(env, output, "attachedValues", *result.attachedValues);
            callStatus != napi_ok) {
            return callStatus;
        }
    }
    return napi_ok;
}

napi_status NapiSetProperty(napi_env env, const std::vector<Bytes> &bytes, napi_value &output)
{
    if (!(bytes.size() <= std::numeric_limits<uint32_t>::max())) {
        return napi_generic_failure;
    }
    if (const auto callStatus = CheckNapiStatus(env, napi_create_array_with_length(env, bytes.size(), &output));
        callStatus != napi_ok) {
        return callStatus;
    }
    for (uint32_t i = 0; i < bytes.size(); ++i) {
        napi_value item = nullptr;
        if (const auto callStatus = NapiSetProperty(env, bytes[i], item); callStatus != napi_ok) {
            return callStatus;
        }
        if (const auto callStatus = CheckNapiStatus(env, napi_set_element(env, output, i, item));
            callStatus != napi_ok) {
            return callStatus;
        }
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
        case PRIVACY_PROTOCOL_SUCCESS:
            return PRIVACY_COMPUTATION_SUCCESS;
        case PRIVACY_PROTOCOL_INVALID_ARGUMENT:
            return PRIVACY_COMPUTATION_INVALID_ARGUMENT;
        case PRIVACY_PROTOCOL_CODEC_ERROR:
            return PRIVACY_COMPUTATION_DATA_CORRUPTED;
        case PRIVACY_PROTOCOL_HASH_ERROR:
        case PRIVACY_PROTOCOL_RANDOM_ERROR:
        case PRIVACY_PROTOCOL_CRYPTO_ERROR:
            return PRIVACY_COMPUTATION_CRYPTO_ERROR;
        case PRIVACY_PROTOCOL_UNSUPPORTED_PROTOCOL:
        case PRIVACY_PROTOCOL_INCOMPATIBLE_HE_PARAMETERS:
        case PRIVACY_PROTOCOL_HE_ERROR:
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
    if (!(napi_create_string_utf8(env, message, NAPI_AUTO_LENGTH, &text) == napi_ok)) {
        return nullptr;
    }
    if (!(napi_create_error(env, nullptr, text, &error) == napi_ok)) {
        return nullptr;
    }
    if (!(napi_create_int32(env, code, &number) == napi_ok)) {
        return nullptr;
    }
    if (!(napi_set_named_property(env, error, "code", number) == napi_ok)) {
        return nullptr;
    }
    return error;
}

napi_status ThrowJsError(napi_env env, int32_t code, const char *message)
{
    LOGE("NAPI operation failed, code=%{public}d", code);
    bool pending = false;
    if (const auto callStatus = napi_is_exception_pending(env, &pending); callStatus != napi_ok) {
        return callStatus;
    }
    if (!(!pending)) {
        return napi_pending_exception;
    }
    napi_value error = CreateJsError(env, code, message);
    if (error == nullptr) {
        LOGE("Failed to create JS error, code=%{public}d", code);
    }
    if (!(error != nullptr)) {
        return napi_generic_failure;
    }
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
    if (const auto callStatus = napi_is_exception_pending(env, &pending); callStatus != napi_ok) {
        return callStatus;
    }
    napi_value error = nullptr;
    if (pending) {
        if (const auto callStatus = napi_get_and_clear_last_exception(env, &error); callStatus != napi_ok) {
            return callStatus;
        }
    } else {
        error = CreateJsError(env, code, message);
    }
    if (error == nullptr) {
        LOGE("Failed to obtain Promise rejection error, code=%{public}d", code);
    }
    if (!(error != nullptr)) {
        return napi_generic_failure;
    }
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
    if (!(CheckNapiStatus(env, napi_create_promise(env, &deferred, &promise)) == napi_ok)) {
        return nullptr;
    }
    if (!(RejectPromise(env, deferred, code, message) == napi_ok)) {
        return nullptr;
    }
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

    if (CheckNapiStatus(env, napi_resolve_deferred(env, context->deferred, output)) != napi_ok) {
        return;
    }
    LOGI("NAPI %{public}s success, promise resolved", context->operationName.c_str());
}

void CompleteAsyncWork(napi_env env, napi_status status, void *data)
{
    std::unique_ptr<PrivacyComputationBaseContext> context(static_cast<PrivacyComputationBaseContext *>(data));
    if (context == nullptr) {
        LOGE("Async completion context is null");
    }
    if (!(context != nullptr)) {
        return;
    }
    if (status != napi_ok) {
        LOGE("Async work completion failed, status=%{public}d", static_cast<int32_t>(status));
    }
    context->result = status == napi_ok ? context->result : PRIVACY_COMPUTATION_SERVICE_ERROR;
    ResolvePromise(env, context.get());
}

napi_status PrepareAsyncWork(napi_env env, napi_callback_info info, PrivacyComputationBaseContext *context,
                             const char *resourceName)
{
    if (const auto callStatus = context->parse(env, info, context); callStatus != napi_ok) {
        return callStatus;
    }
    LOGI("NAPI %{public}s arguments parsed", resourceName);
    context->result = PRIVACY_COMPUTATION_SERVICE_ERROR;
    napi_value resource = nullptr;
    if (const auto callStatus =
            CheckNapiStatus(env, napi_create_string_utf8(env, resourceName, NAPI_AUTO_LENGTH, &resource));
        callStatus != napi_ok) {
        return callStatus;
    }
    if (const auto callStatus =
            CheckNapiStatus(env, napi_create_async_work(env, nullptr, resource, context->execute, CompleteAsyncWork,
                                                        context, &context->work));
        callStatus != napi_ok) {
        return callStatus;
    }
    if (const auto callStatus = CheckNapiStatus(env, napi_queue_async_work(env, context->work));
        callStatus != napi_ok) {
        return callStatus;
    }
    return napi_ok;
}
} // namespace

napi_value CreateAsyncWork(napi_env env, napi_callback_info info,
                           std::unique_ptr<PrivacyComputationBaseContext> context, const char *resourceName)
{
    if (context == nullptr) {
        ThrowJsError(env, PRIVACY_COMPUTATION_OUT_OF_MEMORY, GetErrorMessage(PRIVACY_COMPUTATION_OUT_OF_MEMORY));
        return nullptr;
    }
    if (context->parse == nullptr || context->execute == nullptr || context->resolve == nullptr) {
        ThrowJsError(env, PRIVACY_COMPUTATION_SERVICE_ERROR, "Invalid privacy computation async context.");
        return nullptr;
    }
    context->env = env;
    context->operationName = resourceName;
    napi_value promise = nullptr;
    if (!(CheckNapiStatus(env, napi_create_promise(env, &context->deferred, &promise)) == napi_ok)) {
        return nullptr;
    }
    const napi_status status = PrepareAsyncWork(env, info, context.get(), resourceName);
    if (status != napi_ok) {
        LOGE("Failed to prepare %{public}s, status=%{public}d", resourceName, static_cast<int32_t>(status));
        // Once a Promise exists, failure must reject it, not just return from a NAPI_CALL macro.
        if (!(RejectPromise(env, context->deferred, context->result, GetErrorMessage(context->result)) == napi_ok)) {
            return nullptr;
        }
        return promise; // Context destructor deletes any work created before queue failure.
    }
    LOGI("NAPI %{public}s async work queued", resourceName);
    context.release(); // Ownership transfers to completion only after successful queueing.
    return promise;
}

} // namespace OHOS::Security::PrivacyComputation
