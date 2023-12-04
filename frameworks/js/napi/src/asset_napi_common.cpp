/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "asset_api.h"
#include "asset_log.h"
#include "asset_mem.h"
#include "asset_napi_error_code.h"
#include "asset_type.h"

namespace OHOS {
namespace Security {
namespace Asset {
namespace {
#define MAX_BUFFER_LEN 1024
#define MAX_MESSAGE_LEN 128
#define MAX_ARGS_NUM 5

#define NAPI_THROW_BASE(env, condition, ret, code, message)             \
if ((condition)) {                                                      \
    LOGE("[FATAL][NAPI]%{public}s", (message));                         \
    napi_throw_error((env), std::to_string((code)).c_str(), message);   \
    return (ret);                                                       \
}

#define NAPI_THROW(env, condition, code, message)                       \
    NAPI_THROW_BASE(env, condition, nullptr, code, message)

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
    napi_throw_error((env), std::to_string(ASSET_INVALID_ARGUMENT).c_str(), msg);       \
    return napi_invalid_arg;                                                            \
}

void FreeAssetAttrs(std::vector<Asset_Attr> &attrs)
{
    for (auto attr : attrs) {
        if ((attr.tag & ASSET_TAG_TYPE_MASK) == ASSET_TYPE_BYTES) {
            OH_Asset_FreeBlob(&attr.value.blob);
        }
    }
    attrs.clear();
}

AsyncContext *CreateAsyncContext()
{
    return static_cast<AsyncContext *>(AssetMalloc(sizeof(AsyncContext)));
}

void DestroyAsyncContext(napi_env env, AsyncContext *context)
{
    if (context == nullptr) {
        return;
    }
    if (context->work != nullptr) {
        napi_delete_async_work(env, context->work);
        context->work = nullptr;
    }

    OH_Asset_FreeResultSet(&context->resultSet);
    OH_Asset_FreeBlob(&context->challenge);
    FreeAssetAttrs(context->updateAttrs);
    FreeAssetAttrs(context->attrs);
    AssetFree(context);
}

napi_status ParseByteArray(napi_env env, napi_value value, uint32_t tag, Asset_Blob &blob)
{
    napi_typedarray_type arrayType;
    size_t length = 0;
    void *rawData = nullptr;

    NAPI_CALL_RETURN_ERR(env, napi_get_typedarray_info(env, value, &arrayType, &length, &rawData, nullptr, nullptr));
    CHECK_ASSET_TAG(env, arrayType != napi_uint8_array, tag, "expect type napi_uint8_array");
    CHECK_ASSET_TAG(env, length == 0 || length > MAX_BUFFER_LEN, tag, "invalid array length.");

    blob.data = static_cast<uint8_t *>(AssetMalloc(length));
    NAPI_THROW_RETURN_ERR(env, blob.data == nullptr, ASSET_OUT_OF_MEMRORY, "Unable to allocate memory for Asset_Blob.");

    (void)memcpy_s(blob.data, length, rawData, length);
    blob.size = static_cast<uint32_t>(length);
    return napi_ok;
}

napi_status ParseAssetAttribute(napi_env env, napi_value tag, napi_value value, Asset_Attr &attr)
{
    // parse tag
    napi_valuetype type = napi_undefined;
    NAPI_CALL_RETURN_ERR(env, napi_typeof(env, tag, &type));
    NAPI_THROW_RETURN_ERR(env, type != napi_number, ASSET_INVALID_ARGUMENT, "The tag type of map should be number.");
    NAPI_CALL_RETURN_ERR(env, napi_get_value_uint32(env, tag, &attr.tag));

    // parse value
    NAPI_CALL_RETURN_ERR(env, napi_typeof(env, value, &type));
    switch (attr.tag & ASSET_TAG_TYPE_MASK) {
        case ASSET_TYPE_BOOL:
            CHECK_ASSET_TAG(env, type != napi_boolean, attr.tag, "Expect type napi_boolean.");
            NAPI_CALL_RETURN_ERR(env, napi_get_value_bool(env, value, &attr.value.boolean));
            break;
        case ASSET_TYPE_NUMBER:
            CHECK_ASSET_TAG(env, type != napi_number, attr.tag, "expect type napi_number");
            NAPI_CALL_RETURN_ERR(env, napi_get_value_uint32(env, value, &attr.value.u32));
            break;
        case ASSET_TYPE_BYTES:
            CHECK_ASSET_TAG(env, type != napi_object, attr.tag, "expect type napi_object");
            NAPI_CALL_RETURN_ERR(env, ParseByteArray(env, value, attr.tag, attr.value.blob));
            break;
        default:
            CHECK_ASSET_TAG(env, true, attr.tag, "Invalid tag argument.");
    }
    return napi_ok;
}

napi_value GetIteratorNext(napi_env env, napi_value iterator, napi_value func, bool *done)
{
    napi_value next = nullptr;
    NAPI_CALL(env, napi_call_function(env, iterator, func, 0, nullptr, &next));

    napi_value doneValue = nullptr;
    NAPI_CALL(env, napi_get_named_property(env, next, "done", &doneValue));
    NAPI_CALL(env, napi_get_value_bool(env, doneValue, done));
    return next;
}

napi_status ParseMapParam(napi_env env, napi_value arg, std::vector<Asset_Attr> &attrs)
{
    // check map type
    bool isMap = false;
    NAPI_CALL_RETURN_ERR(env, napi_is_map(env, arg, &isMap));
    NAPI_THROW_RETURN_ERR(env, !isMap, ASSET_INVALID_ARGUMENT, "Expect Map type.");

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

        Asset_Attr param = { 0 };
        NAPI_CALL_BREAK(env, ParseAssetAttribute(env, key, value, param));
        attrs.push_back(param);
    }

    NAPI_THROW_RETURN_ERR(env, !done, ASSET_INVALID_ARGUMENT, "Parse entry of map failed.");
    return napi_ok;
}

napi_value GetUndefinedValue(napi_env env)
{
    napi_value value = nullptr;
    NAPI_CALL(env, napi_get_undefined(env, &value));
    return value;
}

napi_value GetUint8Array(napi_env env, const Asset_Blob *blob)
{
    if (blob->data == nullptr || blob->size == 0 || blob->size > MAX_BUFFER_LEN) {
        return nullptr;
    }

    void *data = nullptr;
    napi_value buffer = nullptr;
    NAPI_CALL(env, napi_create_arraybuffer(env, blob->size, &data, &buffer));
    (void)memcpy_s(data, blob->size, blob->data, blob->size);

    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_typedarray(env, napi_uint8_array, blob->size, buffer, 0, &result));
    return result;
}

napi_value GetMapObject(napi_env env, Asset_Result *result)
{
    napi_value global = nullptr;
    napi_value mapFunc = nullptr;
    napi_value map = nullptr;
    NAPI_CALL(env, napi_get_global(env, &global));
    NAPI_CALL(env, napi_get_named_property(env, global, "Map", &mapFunc));
    NAPI_CALL(env, napi_new_instance(env, mapFunc, 0, nullptr, &map));
    napi_value setFunc = nullptr;
    NAPI_CALL(env, napi_get_named_property(env, map, "set", &setFunc));
    for (uint32_t i = 0; i < result->count; i++) {
        napi_value key = nullptr;
        napi_value value = nullptr;
        NAPI_CALL(env, napi_create_uint32(env, result->attrs[i].tag, &key));
        switch (result->attrs[i].tag & ASSET_TAG_TYPE_MASK) {
            case ASSET_TYPE_BOOL:
                NAPI_CALL(env, napi_get_boolean(env, result->attrs[i].value.boolean, &value));
                break;
            case ASSET_TYPE_NUMBER:
                NAPI_CALL(env, napi_create_uint32(env, result->attrs[i].value.u32, &value));
                break;
            case ASSET_TYPE_BYTES:
                value = GetUint8Array(env, &result->attrs[i].value.blob);
                break;
            default:
                return nullptr;
        }

        napi_value setArgs[] = { key, value };
        NAPI_CALL(env, napi_call_function(env, map, setFunc, sizeof(setArgs) / sizeof(setArgs[0]), setArgs, nullptr));
    }
    return map;
}

napi_value GetMapArray(napi_env env, Asset_ResultSet *resultSet)
{
    napi_value array = nullptr;
    NAPI_CALL(env, napi_create_array(env, &array));
    for (uint32_t i = 0; i < resultSet->count; i++) {
        if (resultSet->results[i].attrs == nullptr || resultSet->results[i].count == 0) {
            return nullptr;
        }
        napi_value map = GetMapObject(env, &resultSet->results[i]);
        NAPI_CALL(env, napi_set_element(env, array, i, map));
    }
    return array;
}

napi_value GetBusinessValue(napi_env env, AsyncContext *context)
{
    // Processing the return value of the PreQueryAsset function.
    if (context->challenge.data != nullptr && context->challenge.size != 0) {
        return GetUint8Array(env, &context->challenge);
    }

    // Processing the return value of the QueryAsset function.
    if (context->resultSet.results != nullptr && context->resultSet.count != 0) {
        return GetMapArray(env, &context->resultSet);
    }

    return GetUndefinedValue(env);
}

napi_value GetBusinessError(napi_env env, int32_t errCode)
{
    napi_value code = nullptr;
    NAPI_CALL(env, napi_create_int32(env, errCode, &code));

    napi_value message = nullptr;
    const char *errorMsg = GetErrorMessage(errCode);
    NAPI_CALL(env, napi_create_string_utf8(env, errorMsg, strlen(errorMsg), &message));

    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_error(env, code, message, &result));
    return result;
}

void ResolvePromise(napi_env env, AsyncContext *context)
{
    napi_value result = nullptr;
    if (context->result == ASSET_SUCCESS) {
        result = GetBusinessValue(env, context);
        NAPI_CALL_RETURN_VOID(env, napi_resolve_deferred(env, context->deferred, result));
    } else {
        result = GetBusinessError(env, context->result);
        NAPI_CALL_RETURN_VOID(env, napi_reject_deferred(env, context->deferred, result));
    }
}

napi_value CreateAsyncWork(napi_env env, AsyncContext *context, const char *funcName,
    napi_async_execute_callback execute)
{
    napi_value result;
    NAPI_CALL(env, napi_create_promise(env, &context->deferred, &result));

    napi_value resource = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, funcName, NAPI_AUTO_LENGTH, &resource));
    NAPI_CALL(env, napi_create_async_work(
        env, nullptr, resource, execute,
        [](napi_env env, napi_status status, void *data) {
            AsyncContext *asyncContext = static_cast<AsyncContext *>(data);
            ResolvePromise(env, asyncContext);
            DestroyAsyncContext(env, asyncContext);
        },
        static_cast<void *>(context), &context->work));
    NAPI_CALL(env, napi_queue_async_work(env, context->work));
    return result;
}

} // anonymous namespace

napi_value NapiEntry(napi_env env, napi_callback_info info, const char *funcName, napi_async_execute_callback execute,
    size_t expectArgNum)
{
    size_t argc = expectArgNum;
    napi_value argv[MAX_ARGS_NUM] = { 0 };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_THROW(env, argc < expectArgNum, ASSET_INVALID_ARGUMENT, "The number of arguments is insufficient.");

    AsyncContext *context = CreateAsyncContext();
    NAPI_THROW(env, context == nullptr, ASSET_OUT_OF_MEMRORY, "Unable to allocate memory for AsyncContext.");

    do {
        size_t index = 0;
        if (ParseMapParam(env, argv[index++], context->attrs) != napi_ok) {
            LOGE("Parse first map parameter failed.");
            break;
        }

        if (expectArgNum == UPDATE_ARGS_NUM &&
            ParseMapParam(env, argv[index++], context->updateAttrs) != napi_ok) {
            LOGE("Parse second map parameter failed.");
            break;
        }

        napi_value promise = CreateAsyncWork(env, context, funcName, execute);
        if (promise == nullptr) {
            LOGE("Create async work failed.");
            break;
        }
        return promise;
    } while (0);
    DestroyAsyncContext(env, context);
    return nullptr;
}

} // Asset
} // Security
} // OHOS