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

#include "hpic_napi_utils.h"
#include "hpic_napi_common.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <utility>

namespace OHOS::Security::PrivacyComputation {
namespace {
napi_status GetRequiredProperty(napi_env env, napi_value object, const char *name, napi_value &value)
{
    napi_valuetype type = napi_undefined;
    NAPI_CALL_RETURN_ERR(env, napi_typeof(env, object, &type));
    NAPI_THROW_RETURN_ERR(env, type != napi_object, HPIC_INVALID_ARGUMENT, "Expected an object parameter.");
    NAPI_CALL_RETURN_ERR(env, napi_get_named_property(env, object, name, &value));
    NAPI_CALL_RETURN_ERR(env, napi_typeof(env, value, &type));
    NAPI_THROW_RETURN_ERR(env, type == napi_undefined, HPIC_INVALID_ARGUMENT,
                          (std::string("Missing required property: ") + name).c_str());
    return napi_ok;
}

napi_status GetOptionalProperty(napi_env env, napi_value object, const char *name, napi_value &value, bool &present)
{
    NAPI_CALL_RETURN_ERR(env, napi_get_named_property(env, object, name, &value));
    napi_valuetype type = napi_undefined;
    NAPI_CALL_RETURN_ERR(env, napi_typeof(env, value, &type));
    present = type != napi_undefined;
    return napi_ok;
}

napi_status ParseJsUint32(napi_env env, napi_value value, uint32_t &number, const char *message)
{
    napi_valuetype type = napi_undefined;
    NAPI_CALL_RETURN_ERR(env, napi_typeof(env, value, &type));
    NAPI_THROW_RETURN_ERR(env, type != napi_number, HPIC_INVALID_ARGUMENT, message);
    double raw = 0;
    NAPI_CALL_RETURN_ERR(env, napi_get_value_double(env, value, &raw));
    NAPI_THROW_RETURN_ERR(
        env, !std::isfinite(raw) || raw < 0 || raw > std::numeric_limits<uint32_t>::max() || std::floor(raw) != raw,
        HPIC_INVALID_ARGUMENT, message);
    number = static_cast<uint32_t>(raw);
    return napi_ok;
}

napi_status ParseJsUint8Array(napi_env env, napi_value value, psi_pir::Bytes &bytes, const char *message)
{
    bool typed = false;
    NAPI_CALL_RETURN_ERR(env, napi_is_typedarray(env, value, &typed));
    NAPI_THROW_RETURN_ERR(env, !typed, HPIC_INVALID_ARGUMENT, message);
    napi_typedarray_type type;
    size_t count = 0;
    void *data = nullptr;
    NAPI_CALL_RETURN_ERR(env, napi_get_typedarray_info(env, value, &type, &count, &data, nullptr, nullptr));
    NAPI_THROW_RETURN_ERR(env, type != napi_uint8_array || (count != 0 && data == nullptr), HPIC_INVALID_ARGUMENT,
                          message);
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
        IF_ERR_RETURN(ParseJsUint32(env, value, id, "hashAlg must be a supported integer enum."));
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
        NAPI_THROW_RETURN_ERR(env, true, HPIC_INVALID_ARGUMENT, "hashAlg is outside the supported enum range.");
    }
    return napi_ok;
}

napi_status GetArrayLength(napi_env env, napi_value value, uint32_t &count, const char *message)
{
    bool array = false;
    NAPI_CALL_RETURN_ERR(env, napi_is_array(env, value, &array));
    NAPI_THROW_RETURN_ERR(env, !array, HPIC_INVALID_ARGUMENT, message);
    NAPI_CALL_RETURN_ERR(env, napi_get_array_length(env, value, &count));
    return napi_ok;
}

napi_status ParseJsByteArray(napi_env env, napi_value value, std::vector<psi_pir::Bytes> &bytes)
{
    uint32_t count = 0;
    IF_ERR_RETURN(GetArrayLength(env, value, count, "Ciphertexts must be an array of Uint8Array."));
    for (uint32_t i = 0; i < count; ++i) {
        napi_value item = nullptr;
        NAPI_CALL_RETURN_ERR(env, napi_get_element(env, value, i, &item));
        psi_pir::Bytes entry;
        IF_ERR_RETURN(ParseJsUint8Array(env, item, entry, "Each ciphertext must be a Uint8Array."));
        bytes.push_back(std::move(entry));
    }
    return napi_ok;
}

napi_status ParseJsElement(napi_env env, napi_value value, psi_pir::Element &element)
{
    napi_value key = nullptr;
    IF_ERR_RETURN(GetRequiredProperty(env, value, "elemKey", key));
    IF_ERR_RETURN(ParseJsUint8Array(env, key, element.elemKey, "elemKey must be a Uint8Array."));
    IF_ERR_RETURN(ParseJsHashAlg(env, value, element.hashAlg));
    napi_value attached = nullptr;
    bool present = false;
    IF_ERR_RETURN(GetOptionalProperty(env, value, "elemValue", attached, present));
    if (present) {
        element.elemValue.emplace();
        IF_ERR_RETURN(
            ParseJsUint8Array(env, attached, *element.elemValue, "elemValue must be a Uint8Array when provided."));
    }
    return napi_ok;
}

napi_status CreateJsByteArray(napi_env env, const std::vector<psi_pir::Bytes> &bytes, napi_value &output)
{
    IF_FALSE_RETURN(bytes.size() <= std::numeric_limits<uint32_t>::max(), napi_generic_failure);
    NAPI_CALL_RETURN_ERR(env, napi_create_array_with_length(env, bytes.size(), &output));
    for (uint32_t i = 0; i < bytes.size(); ++i) {
        napi_value item = nullptr;
        IF_ERR_RETURN(CreateJsUint8Array(env, bytes[i], item));
        NAPI_CALL_RETURN_ERR(env, napi_set_element(env, output, i, item));
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
    NAPI_THROW_RETURN_ERR(env, count != expectedCount || expectedCount >= MAX_ARGS_COUNT, HPIC_INVALID_ARGUMENT,
                          "Unexpected number of arguments.");
    std::copy_n(values, expectedCount, args);
    return napi_ok;
}

napi_status ParseJsPrivacyTarget(napi_env env, napi_value value, psi_pir::Bytes &target)
{
    return ParseJsUint8Array(env, value, target, "privacyTarget must be a Uint8Array.");
}

napi_status ParseJsProtocol(napi_env env, napi_value value, psi_pir::PrivacyProtocol &protocol)
{
    napi_value size = nullptr;
    napi_value mode = nullptr;
    uint32_t sizeId = 0;
    uint32_t modeId = 0;
    IF_ERR_RETURN(GetRequiredProperty(env, value, "dataSetSize", size));
    IF_ERR_RETURN(GetRequiredProperty(env, value, "protocolType", mode));
    IF_ERR_RETURN(ParseJsUint32(env, size, sizeId, "dataSetSize must be an integer enum."));
    IF_ERR_RETURN(ParseJsUint32(env, mode, modeId, "protocolType must be an integer enum."));
    NAPI_THROW_RETURN_ERR(env,
                          sizeId > static_cast<uint32_t>(psi_pir::DataSetSize::SIZE_512) ||
                              modeId > static_cast<uint32_t>(psi_pir::ProtocolType::PIR_PROTOCOL),
                          HPIC_INVALID_ARGUMENT, "Unsupported protocol enum value.");
    protocol.dataSetSize = static_cast<psi_pir::DataSetSize>(sizeId);
    protocol.protocolType = static_cast<psi_pir::ProtocolType>(modeId);
    return napi_ok;
}

napi_status ParseJsTarget(napi_env env, napi_value value, psi_pir::TargetInput &target)
{
    napi_value data = nullptr;
    IF_ERR_RETURN(GetRequiredProperty(env, value, "elemData", data));
    IF_ERR_RETURN(ParseJsUint8Array(env, data, target.elemData, "elemData must be a Uint8Array."));
    IF_ERR_RETURN(ParseJsHashAlg(env, value, target.hashAlg));
    return napi_ok;
}

napi_status ParseJsElements(napi_env env, napi_value value, std::vector<psi_pir::Element> &elements)
{
    uint32_t count = 0;
    IF_ERR_RETURN(GetArrayLength(env, value, count, "elements must be an array."));
    for (uint32_t i = 0; i < count; ++i) {
        napi_value item = nullptr;
        NAPI_CALL_RETURN_ERR(env, napi_get_element(env, value, i, &item));
        psi_pir::Element element;
        IF_ERR_RETURN(ParseJsElement(env, item, element));
        elements.push_back(std::move(element));
    }
    return napi_ok;
}

napi_status ParseJsSearchResult(napi_env env, napi_value value, psi_pir::PrivacySearchResult &result)
{
    napi_value ciphertext = nullptr;
    IF_ERR_RETURN(GetRequiredProperty(env, value, "fheCipherText", ciphertext));
    IF_ERR_RETURN(ParseJsByteArray(env, ciphertext, result.fheCipherText));
    bool present = false;
    IF_ERR_RETURN(GetOptionalProperty(env, value, "symCipherText", ciphertext, present));
    if (present) {
        result.symCipherText.emplace();
        IF_ERR_RETURN(ParseJsByteArray(env, ciphertext, *result.symCipherText));
    }
    return napi_ok;
}

napi_status CreateJsUint8Array(napi_env env, const psi_pir::Bytes &bytes, napi_value &output)
{
    void *data = nullptr;
    napi_value buffer = nullptr;
    NAPI_CALL_RETURN_ERR(env, napi_create_arraybuffer(env, bytes.size(), &data, &buffer));
    if (!bytes.empty()) {
        NAPI_THROW_RETURN_ERR(env, data == nullptr, HPIC_SERVICE_ERROR, "Unable to create output ArrayBuffer.");
        std::copy(bytes.begin(), bytes.end(), static_cast<uint8_t *>(data));
    }
    NAPI_CALL_RETURN_ERR(env, napi_create_typedarray(env, napi_uint8_array, bytes.size(), buffer, 0, &output));
    return napi_ok;
}

napi_status CreateJsPrivacySearchResult(napi_env env, const psi_pir::PrivacySearchResult &result, napi_value &output)
{
    NAPI_CALL_RETURN_ERR(env, napi_create_object(env, &output));
    napi_value ciphertext = nullptr;
    IF_ERR_RETURN(CreateJsByteArray(env, result.fheCipherText, ciphertext));
    NAPI_CALL_RETURN_ERR(env, napi_set_named_property(env, output, "fheCipherText", ciphertext));
    if (result.symCipherText) {
        IF_ERR_RETURN(CreateJsByteArray(env, *result.symCipherText, ciphertext));
        NAPI_CALL_RETURN_ERR(env, napi_set_named_property(env, output, "symCipherText", ciphertext));
    }
    return napi_ok;
}

napi_status CreateJsSearchResult(napi_env env, const psi_pir::SearchResult &result, napi_value &output)
{
    NAPI_CALL_RETURN_ERR(env, napi_create_object(env, &output));
    napi_value matched = nullptr;
    NAPI_CALL_RETURN_ERR(env, napi_get_boolean(env, result.matchedResult, &matched));
    NAPI_CALL_RETURN_ERR(env, napi_set_named_property(env, output, "matchedResult", matched));
    if (result.attachedValues) {
        napi_value values = nullptr;
        IF_ERR_RETURN(CreateJsByteArray(env, *result.attachedValues, values));
        NAPI_CALL_RETURN_ERR(env, napi_set_named_property(env, output, "attachedValues", values));
    }
    return napi_ok;
}
} // namespace OHOS::Security::PrivacyComputation
