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

#include "asset_napi_common.h"

#include <vector>

#include "securec.h"

#include "asset_log.h"
#include "asset_mem.h"
#include "asset_system_type.h"

#include "asset_napi_context.h"
#include "asset_napi_common.h"
#include "asset_api_error_code.h"

namespace OHOS {
namespace Security {
namespace Asset {
namespace {
#define MAX_BUFFER_LEN 2048

#define NAPI_THROW_RETURN_ERR(env, condition, code, message)            \
    NAPI_THROW_BASE(env, condition, napi_generic_failure, code, message)

#define NAPI_CALL_BREAK(env, theCall)   \
if ((theCall) != napi_ok) {             \
    GET_AND_THROW_LAST_ERROR((env));    \
    break;                              \
}

#define NAPI_CALL_RETURN_ERR(env, theCall)  \
if ((theCall) != napi_ok) {                 \
    GET_AND_THROW_LAST_ERROR((env));        \
    return napi_generic_failure;            \
}

#define CHECK_ASSET_TAG(env, condition, tag, message)                                   \
if ((condition)) {                                                                      \
    char msg[MAX_MESSAGE_LEN] = { 0 };                                                  \
    (void)sprintf_s(msg, MAX_MESSAGE_LEN, "AssetTag(0x%08x) " message, tag);            \
    LOGE("[FATAL][NAPI]%{public}s", (msg));                                             \
    napi_throw((env), CreateJsError((env), SEC_ASSET_INVALID_ARGUMENT, (msg)));         \
    return napi_invalid_arg;                                                            \
}

bool IsBlobValid(const AssetBlob &blob)
{
    return blob.data != nullptr && blob.size != 0;
}

napi_status ParseByteArray(const napi_env env, napi_value value, uint32_t tag, AssetBlob &blob)
{
    napi_typedarray_type arrayType;
    size_t length = 0;
    void *rawData = nullptr;

    bool result = false;
    NAPI_CALL_RETURN_ERR(env, napi_is_typedarray(env, value, &result));
    CHECK_ASSET_TAG(env, !result, tag, "Expect type napi_typedarray.");
    NAPI_CALL_RETURN_ERR(env, napi_get_typedarray_info(env, value, &arrayType, &length, &rawData, nullptr, nullptr));
    CHECK_ASSET_TAG(env, arrayType != napi_uint8_array, tag, "Expect type napi_uint8_array.");
    CHECK_ASSET_TAG(env, length == 0 || length > MAX_BUFFER_LEN, tag, "Invalid array length.");

    blob.data = static_cast<uint8_t *>(AssetMalloc(length));
    NAPI_THROW_RETURN_ERR(
        env, blob.data == nullptr, SEC_ASSET_OUT_OF_MEMORY, "Unable to allocate memory for AssetBlob.");

    (void)memcpy_s(blob.data, length, rawData, length);
    blob.size = static_cast<uint32_t>(length);
    return napi_ok;
}

napi_status ParseAssetAttribute(const napi_env env, napi_value tag, napi_value value, AssetAttr &attr)
{
    // parse tag
    napi_valuetype type = napi_undefined;
    NAPI_CALL_RETURN_ERR(env, napi_typeof(env, tag, &type));
    NAPI_THROW_RETURN_ERR(
        env, type != napi_number, SEC_ASSET_INVALID_ARGUMENT, "The tag type of map should be number.");
    NAPI_CALL_RETURN_ERR(env, napi_get_value_uint32(env, tag, &attr.tag));

    // parse value
    NAPI_CALL_RETURN_ERR(env, napi_typeof(env, value, &type));
    switch (attr.tag & SEC_ASSET_TAG_TYPE_MASK) {
        case SEC_ASSET_TYPE_BOOL:
            CHECK_ASSET_TAG(env, type != napi_boolean, attr.tag, "Expect type napi_boolean.");
            NAPI_CALL_RETURN_ERR(env, napi_get_value_bool(env, value, &attr.value.boolean));
            break;
        case SEC_ASSET_TYPE_NUMBER:
            CHECK_ASSET_TAG(env, type != napi_number, attr.tag, "Expect type napi_number.");
            NAPI_CALL_RETURN_ERR(env, napi_get_value_uint32(env, value, &attr.value.u32));
            break;
        case SEC_ASSET_TYPE_BYTES:
            CHECK_ASSET_TAG(env, type != napi_object, attr.tag, "Expect type napi_object.");
            NAPI_CALL_RETURN_ERR(env, ParseByteArray(env, value, attr.tag, attr.value.blob));
            break;
        default:
            CHECK_ASSET_TAG(env, true, attr.tag, "Invalid tag argument.");
    }
    return napi_ok;
}

napi_value GetIteratorNext(const napi_env env, napi_value iterator, napi_value func, bool *done)
{
    napi_value next = nullptr;
    NAPI_CALL(env, napi_call_function(env, iterator, func, 0, nullptr, &next));

    napi_value doneValue = nullptr;
    NAPI_CALL(env, napi_get_named_property(env, next, "done", &doneValue));
    NAPI_CALL(env, napi_get_value_bool(env, doneValue, done));
    return next;
}

napi_value CreateJsMap(const napi_env env, const AssetResult &result)
{
    napi_value global = nullptr;
    napi_value mapFunc = nullptr;
    napi_value map = nullptr;
    NAPI_CALL(env, napi_get_global(env, &global));
    NAPI_CALL(env, napi_get_named_property(env, global, "Map", &mapFunc));
    NAPI_CALL(env, napi_new_instance(env, mapFunc, 0, nullptr, &map));
    napi_value setFunc = nullptr;
    NAPI_CALL(env, napi_get_named_property(env, map, "set", &setFunc));
    for (uint32_t i = 0; i < result.count; i++) {
        napi_value key = nullptr;
        napi_value value = nullptr;
        NAPI_CALL(env, napi_create_uint32(env, result.attrs[i].tag, &key));
        switch (result.attrs[i].tag & SEC_ASSET_TAG_TYPE_MASK) {
            case SEC_ASSET_TYPE_BOOL:
                NAPI_CALL(env, napi_get_boolean(env, result.attrs[i].value.boolean, &value));
                break;
            case SEC_ASSET_TYPE_NUMBER:
                NAPI_CALL(env, napi_create_uint32(env, result.attrs[i].value.u32, &value));
                break;
            case SEC_ASSET_TYPE_BYTES:
                value = CreateJsUint8Array(env, result.attrs[i].value.blob);
                break;
            default:
                return nullptr;
        }

        napi_value setArgs[] = { key, value };
        NAPI_CALL(env, napi_call_function(env, map, setFunc, sizeof(setArgs) / sizeof(setArgs[0]), setArgs, nullptr));
    }
    return map;
}

void ResolvePromise(const napi_env env, BaseContext *context)
{
    if (context->result == SEC_ASSET_SUCCESS) {
        napi_value result = context->resolve(env, context);
        NAPI_CALL_RETURN_VOID(env, napi_resolve_deferred(env, context->deferred, result));
    } else {
        napi_value result = CreateJsError(env, context->result);
        NAPI_CALL_RETURN_VOID(env, napi_reject_deferred(env, context->deferred, result));
    }
}
} // anonymous namespace

napi_status ParseJsArgs(const napi_env env, napi_callback_info info, napi_value *value, size_t valueSize)
{
    size_t argc = valueSize;
    NAPI_CALL_RETURN_ERR(env, napi_get_cb_info(env, info, &argc, value, nullptr, nullptr));
    NAPI_THROW_RETURN_ERR(env, argc < valueSize, SEC_ASSET_INVALID_ARGUMENT,
        "Mandatory parameters are left unspecified.");
    return napi_ok;
}

napi_status ParseJsMap(const napi_env env, napi_value arg, std::vector<AssetAttr> &attrs)
{
    // check map type
    bool isMap = false;
    NAPI_CALL_RETURN_ERR(env, napi_is_map(env, arg, &isMap));
    NAPI_THROW_RETURN_ERR(env, !isMap, SEC_ASSET_INVALID_ARGUMENT, "Expect Map type.");

    // parse map object
    napi_value entriesFunc = nullptr;
    napi_value iterator = nullptr;
    napi_value nextFunc = nullptr;
    NAPI_CALL_RETURN_ERR(env, napi_get_named_property(env, arg, "entries", &entriesFunc));
    NAPI_CALL_RETURN_ERR(env, napi_call_function(env, arg, entriesFunc, 0, nullptr, &iterator));
    NAPI_CALL_RETURN_ERR(env, napi_get_named_property(env, iterator, "next", &nextFunc));

    bool done = false;
    napi_value next = nullptr;
    while ((next = GetIteratorNext(env, iterator, nextFunc, &done)) != nullptr && !done) {
        napi_value entry = nullptr;
        napi_value key = nullptr;
        napi_value value = nullptr;
        NAPI_CALL_BREAK(env, napi_get_named_property(env, next, "value", &entry));
        NAPI_CALL_BREAK(env, napi_get_element(env, entry, 0, &key));
        NAPI_CALL_BREAK(env, napi_get_element(env, entry, 1, &value));

        AssetAttr param = { 0 };
        NAPI_CALL_BREAK(env, ParseAssetAttribute(env, key, value, param));
        attrs.push_back(param);
    }

    NAPI_THROW_RETURN_ERR(env, !done, SEC_ASSET_INVALID_ARGUMENT, "Parse entry of map failed.");
    return napi_ok;
}

napi_status ParseJsUserId(const napi_env env, napi_value arg, std::vector<AssetAttr> &attrs)
{
    napi_valuetype type = napi_undefined;
    NAPI_CALL_RETURN_ERR(env, napi_typeof(env, arg, &type));
    NAPI_THROW_RETURN_ERR(env, type != napi_number, SEC_ASSET_INVALID_ARGUMENT, "The type of userId should be number.");

    AssetAttr param = { 0 };
    param.tag = SEC_ASSET_TAG_USER_ID;
    NAPI_CALL_RETURN_ERR(env, napi_get_value_uint32(env, arg, &param.value.u32));
    attrs.push_back(param);
    return napi_ok;
}

napi_value CreateJsError(const napi_env env, int32_t errCode)
{
    return CreateJsError(env, errCode, GetErrorMessage(errCode));
}

napi_value CreateJsError(const napi_env env, int32_t errCode, const char *errorMsg)
{
    napi_value code = nullptr;
    NAPI_CALL(env, napi_create_int32(env, errCode, &code));

    napi_value message = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, errorMsg, strlen(errorMsg), &message));

    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_error(env, code, message, &result));
    return result;
}

napi_value CreateJsUint8Array(const napi_env env, const AssetBlob &blob)
{
    if (!IsBlobValid(blob) || blob.size > MAX_BUFFER_LEN) {
        return nullptr;
    }

    void *data = nullptr;
    napi_value buffer = nullptr;
    NAPI_CALL(env, napi_create_arraybuffer(env, blob.size, &data, &buffer));
    (void)memcpy_s(data, blob.size, blob.data, blob.size);

    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_typedarray(env, napi_uint8_array, blob.size, buffer, 0, &result));
    return result;
}

napi_value CreateJsMapArray(const napi_env env, const AssetResultSet &resultSet)
{
    napi_value array = nullptr;
    NAPI_CALL(env, napi_create_array(env, &array));
    for (uint32_t i = 0; i < resultSet.count; i++) {
        if (resultSet.results[i].attrs == nullptr || resultSet.results[i].count == 0) {
            return nullptr;
        }
        napi_value map = CreateJsMap(env, resultSet.results[i]);
        NAPI_CALL(env, napi_set_element(env, array, i, map));
    }
    return array;
}

std::function<void(char *, uint32_t)> NapiThrowError(const napi_env env)
{
    return [env](char *errorMsg, uint32_t errorCode) {
        napi_throw((env), CreateJsError((env), static_cast<int32_t>(errorCode), (errorMsg)));
    };
}

napi_value CreateJsUndefined(const napi_env env)
{
    napi_value value = nullptr;
    NAPI_CALL(env, napi_get_undefined(env, &value));
    return value;
}

napi_status NapiSetProperty(const napi_env env, napi_value object, const char *propertyName, uint32_t propertyValue)
{
    napi_value property = nullptr;
    NAPI_CALL_RETURN_ERR(env, napi_create_uint32(env, propertyValue, &property));
    NAPI_CALL_RETURN_ERR(env, napi_set_named_property(env, object, propertyName, property));
    return napi_ok;
}

napi_value CreateAsyncWork(const napi_env env, napi_callback_info info, std::unique_ptr<BaseContext> context,
    const char *resourceName)
{
    if (context->parse != nullptr) {
        NAPI_CALL(env, context->parse(env, info, context.get()));
    }

    napi_value promise;
    NAPI_CALL(env, napi_create_promise(env, &context->deferred, &promise));
    if (context->check != nullptr) {
        int32_t errorCode = context->check(context->attrs, NapiThrowError(env));
        if (errorCode != SEC_ASSET_SUCCESS) {
            NAPI_CALL(env, napi_reject_deferred(env, context->deferred, CreateJsError(env, errorCode)));
            return promise;
        }
    }
    napi_value resource = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, resourceName, NAPI_AUTO_LENGTH, &resource));
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, context->execute,
        [](napi_env env, napi_status status, void *data) {
            BaseContext *context = static_cast<BaseContext *>(data);
            ResolvePromise(env, context);
            delete context;
        }, static_cast<void *>(context.get()), &context->work));
    context->env = env;
    NAPI_CALL(env, napi_queue_async_work(env, context->work));
    context.release();
    return promise;
}

napi_value CreateSyncWork(const napi_env env, napi_callback_info info, BaseContext *context)
{
    if (context->parse != nullptr) {
        NAPI_CALL(env, context->parse(env, info, context));
    }

    context->execute(env, context);
    if (context->result != SEC_ASSET_SUCCESS) {
        napi_throw(env, CreateJsError(env, context->result));
        return nullptr;
    }
    return context->resolve(env, context);
}

} // Asset
} // Security
} // OHOS